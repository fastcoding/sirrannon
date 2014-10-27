/* cr-canvas.c 
 * Copyright (C) 2006 Robert Gibbs <bgibbs@users.sourceforge.net> 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <math.h>
#include <gtk/gtk.h>
#include "cr-marshal.h"
#include "cr-canvas.h"

#define DEBUG_UPDATE 0
#define DEBUG_INVALIDATE 0
#define DEBUG_PROFILE 0

#if DEBUG_PROFILE
#include <time.h>
static clock_t overall_ticks = 0;
#endif

/**
 * SECTION:cr-canvas
 * @title: CrCanvas
 * @short_description: A #GtkWidget used to paint a tree of #CrItem objects.
 *
 * A #GtkWidget that is used as a drawing device for a tree for #CrItem objects.
 * The CrCanvas widget has comprehensive zooming, scaling, and viewable region
 * methods and properties.  All region transformations are stored in the
 * canvas's root #CrItem.
 */

static GObjectClass *parent_class = NULL;

enum {
        ARG_0,
        PROP_HADJUSTMENT,
        PROP_VADJUSTMENT,
        PROP_MAINTAIN_CENTER,
        PROP_MAINTAIN_ASPECT,
        PROP_AUTO_SCALE,
        PROP_SHOW_LESS,
        PROP_REPAINT_MODE,
        PROP_ROOT,
        PROP_PICK_ITEM,
        PROP_REPAINT_ON_SCROLL
};

enum {
        SCROLL_REGION_CHANGED,
        BEFORE_PAINT,
        EVENT_ROOT,
        LAST_SIGNAL
};

static guint cr_canvas_signals[LAST_SIGNAL] = { 0 };

static void
update_adjustment_factor(CrCanvas *canvas, GtkAdjustment *adjustment, gdouble
                scroll_factor, gdouble viewport_length)
{
        gdouble region;

        region = MAX(1, viewport_length * scroll_factor);

        if (adjustment->upper != region) {

                adjustment->lower = 0;
                adjustment->upper = region;

                adjustment->page_size = viewport_length;
                adjustment->page_increment = viewport_length;
                adjustment->step_increment = viewport_length/10;
                gtk_adjustment_changed(adjustment);
        }
}

static void
update_adjustment_world(CrCanvas *canvas, GtkAdjustment *adjustment,
                gdouble value, gdouble upper, gdouble viewport_length)
{
        double length;
        /* want to preserve the upper and lower limits and bound the value such
         * that it is inside the limits if possible. */

        length = MIN(viewport_length, upper);

        if (length != adjustment->page_size || 
                        upper != adjustment->upper) {
                adjustment->page_size = length;
                adjustment->page_increment = length;
                adjustment->step_increment = length/10;
                adjustment->upper = upper;
                adjustment->lower = 0;
                gtk_adjustment_changed(adjustment);
        }

        value = CLAMP(value, 0, adjustment->upper - length);

        if (value != adjustment->value) {
                adjustment->value = value;
                g_signal_handlers_block_matched(adjustment, G_SIGNAL_MATCH_DATA,
                                0, 0, NULL, NULL, canvas);
                gtk_adjustment_value_changed(adjustment);
                g_signal_handlers_unblock_matched(adjustment, 
                                G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, canvas);
        }
}

static void
update_adjustments(CrCanvas *canvas)
{
        gdouble width, height, x1, y1, x2, y2, z, vx, vy, dx, dy;
        gdouble x, y, old_x, old_y;
        cairo_matrix_t *matrix, *matrix_i;
        CrItem *item;

        item = canvas->root;

        width = GTK_WIDGET(canvas)->allocation.width;
        height = GTK_WIDGET(canvas)->allocation.height;

        if (canvas->flags & CR_CANVAS_SCROLL_WORLD) {

                x1 = canvas->scroll_x1;
                y1 = canvas->scroll_y1;
                x2 = canvas->scroll_x2;
                y2 = canvas->scroll_y2;

                matrix = cr_item_get_matrix(item);
                matrix_i = cr_item_get_inverse_matrix(item);

                /* In order to handle rotation I need to transform the
                 * point about the rotational axis.
                 * assuming the axis is zero for now.
                 */
                z = 0;
                cairo_matrix_transform_point(matrix, &x1, &z);
                z = 0;
                cairo_matrix_transform_point(matrix, &x2, &z);
                z = 0;
                cairo_matrix_transform_point(matrix, &z, &y1);
                z = 0;
                cairo_matrix_transform_point(matrix, &z, &y2);

                dx = fabs(x2 - x1);
                dy = fabs(y2 - y1);
                vx = fabs(MIN(x1,x2));
                vy = fabs(MIN(y1,y2));

                canvas->value_x = vx;
                canvas->value_y = vy;
                update_adjustment_world(canvas, canvas->hadjustment,
                                vx, dx, width);

                update_adjustment_world(canvas, canvas->vadjustment,
                                vy, dy, height);

                if (width > dx+1 ||  height > dy+1) {
                        /* the viewport is bigger than the region.  Apply a
                         * translation to keep it at a sensible location. 
                         * Since it may be rotated - place it at the center of
                         * the device area.
                         */
                        x = (canvas->scroll_x2 - canvas->scroll_x1) / 2 +
                                canvas->scroll_x1;
                        y = (canvas->scroll_y2 - canvas->scroll_y1) / 2 +
                                canvas->scroll_y1;
                        old_x = width/2;
                        old_y = height/2;
                        cairo_matrix_transform_point(matrix_i, &old_x, &old_y);

                        if (old_x - x != 0 || old_y - y != 0) {
                                cairo_matrix_translate(matrix, old_x - x, 
                                                old_y - y);
                        }
                }
        }
        else {

                /* FIXME: Decide if this should only be done on a size alloc.*/

                update_adjustment_factor(canvas, canvas->hadjustment,
                                canvas->scroll_factor_x, width);

                update_adjustment_factor(canvas, canvas->vadjustment,
                                canvas->scroll_factor_y, height);
        }
        /*
                g_print ("lower = %g, upper = %g, value = %g\n",
                                canvas->hadjustment->lower,
                                canvas->hadjustment->upper,
                                canvas->hadjustment->value);
                                */
}

/**
 * recenter_adjustment:
 *
 * Puts scrollbar back into the center of the current range.
 **/
static void
recenter_adjustment(GtkAdjustment *adjustment, gdouble *newval_ref)
{

        *newval_ref = (adjustment->upper - adjustment->page_size -
                        adjustment->lower) / 2;

        if (*newval_ref != adjustment->value) {
                adjustment->value = *newval_ref;
                gtk_adjustment_value_changed(adjustment);
        }
}

/**
 * recenter_adjustments:
 *
 * Puts both scroll bars back to the center. Calling this only makes sense in
 * the infinite world model.
 **/
static void
recenter_adjustments(CrCanvas *canvas)
{
        if (!(canvas->flags & CR_CANVAS_SCROLL_WORLD)) {
                recenter_adjustment(canvas->hadjustment, &canvas->value_x);
                recenter_adjustment(canvas->vadjustment, &canvas->value_y);
        }
        g_signal_emit(canvas, cr_canvas_signals[SCROLL_REGION_CHANGED], 0);
}

static gboolean
on_update_idle(CrCanvas *canvas)
{
        (*CR_CANVAS_GET_CLASS(canvas)->update)(canvas);
        canvas->update_idle_id = 0;
        return FALSE;
}

static gboolean
repaint_revert(CrCanvas *canvas) 
{
        if (canvas->flags & CR_CANVAS_REPAINT_REVERT) {
                (*CR_CANVAS_GET_CLASS(canvas)->quick_update)(canvas);
                canvas->flags &= ~(CR_CANVAS_REPAINT_MODE |
                                CR_CANVAS_REPAINT_REVERT);
        }
        return FALSE;
}

static gboolean
on_repaint_revert_idle(CrCanvas *canvas)
{
        cairo_t *cr;
        /* This is a high priority update that gets called
         * before the expose event.
         * We set another idle to be called after the expose event to run
         * the item update.
         */
        canvas->update_idle_id = g_idle_add_full(
                        G_PRIORITY_LOW,
                        (GSourceFunc) repaint_revert,
                        canvas, NULL);

        cr = gdk_cairo_create (GTK_WIDGET(canvas)->window);

        g_signal_emit(canvas, cr_canvas_signals[BEFORE_PAINT], 0, cr,
                        (canvas->flags & CR_CANVAS_VIEWPORT_CHANGED) != 0);

        canvas->flags &= ~CR_CANVAS_VIEWPORT_CHANGED;

        cairo_destroy(cr);

        return FALSE;
}

/**
 * update:
 * @canvas:
 *
 * This is the normal update sequence.  The canvas first requests that all that
 * changed report their old boundaries.  Then it requests that the same items
 * report their new boundaries.  All the boundaries are are sent to gdk for
 * determination of what portion of the window has changed.
 */
static void
update(CrCanvas *canvas)
{
        cairo_t *cr;
        GdkRegion *region;
        GdkRectangle rect;

        //I am trying to call this from realize.  If this is not feasible,
        // then it will be necessary to override map
        if (!GTK_WIDGET_MAPPED(canvas)) return;

        /*
         * FIXME: This logic does not work in cairo 1.3.x series - was ok in 1.2
        cr = canvas->cairo;
        if (!cr) {
                cr = gdk_cairo_create(GTK_WIDGET(canvas)->window);
                canvas->cairo = cr;
        }
        */

        /* This logic does not work during the clip event from geoc.
         * The cairo matrix translations are shifted by the amount of the 
         * expose event as if the matrix gets modified even though
         * cairo_identity_matrix is being called.
         *
         *cr = gdk_cairo_create(GTK_WIDGET(canvas)->window);
         *
         */
        /*
        cairo_surface_t *surface;
        surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 0, 0);
        cr = cairo_create(surface);
        cairo_surface_destroy(surface);
        */
        cr = gdk_cairo_create(GTK_WIDGET(canvas)->window);

        if (canvas->update_idle_id) {
                g_source_remove(canvas->update_idle_id);
                canvas->update_idle_id = 0;
        }

#if DEBUG_UPDATE
        g_print("do update\n");
#endif

        canvas->flags &= ~CR_CANVAS_NEED_UPDATE;

        cairo_identity_matrix(cr);
        cr_item_report_old_bounds(canvas->root, cr, FALSE);
        cairo_identity_matrix(cr);
        cr_item_report_new_bounds(canvas->root, cr, FALSE);

        if (canvas->flags & CR_CANVAS_NEED_UPDATE) {

                g_warning("Canvas item requested update during update loop.");
                (*CR_CANVAS_GET_CLASS(canvas)->update)(canvas);
        }

#if 0
        region = gdk_window_get_update_area(GTK_WIDGET(canvas)->window);
        if (region) {
                gdk_region_get_clipbox(region, &rect);
                printf("ready to expose %ix%i at %i,%i\n", 
                                rect.width, rect.height,
                                rect.x, rect.y);
                gdk_window_invalidate_region(GTK_WIDGET(canvas)->window, 
                                region, FALSE);
                gdk_region_destroy(region);
        }
#endif

        g_signal_emit(canvas, cr_canvas_signals[BEFORE_PAINT], 0, cr,
                        (canvas->flags & CR_CANVAS_VIEWPORT_CHANGED) != 0);

        canvas->flags &= ~CR_CANVAS_VIEWPORT_CHANGED;
        cairo_destroy(cr);

        if (!(canvas->flags & CR_CANVAS_REPAINT_MODE) &&
                        canvas->flags & CR_CANVAS_NEED_UPDATE) {
                /* This catches the case where a request_update would get made
                 * as a result of the expose signal being emitted. An example of
                 * this would be an item that depends on the position of other 
                 * items.  
                 * This logic prevents the paint from happening twice by
                 * triggering a second update.  In repaint mode this is not
                 * necessary because everything will be repainted anyway.*/

                if (canvas->flags & CR_CANVAS_INSIDE_UPDATE) {
                        g_warning("before-paint signal triggers update twice"
                                        " further recursion has been stopped.");
                }
                else {
                        canvas->flags |= CR_CANVAS_INSIDE_UPDATE;
                        (*CR_CANVAS_GET_CLASS(canvas)->update)(canvas);
                        canvas->flags &= ~CR_CANVAS_INSIDE_UPDATE;
                }
        }
}

/**
 * quick_update:
 * @canvas:
 *
 * This performs a quicker update.  It updates all of the canvas items and their
 * bounds without triggering an invalidate on the widget.   It is used in
 * conjunction with repaint mode.
 */
static void
quick_update(CrCanvas *canvas) 
{
        cairo_t *cr;

#if DEBUG_UPDATE
        g_print("quick update\n");
#endif
        if (!GTK_WIDGET_MAPPED(canvas)) return;

        /*
        cr = canvas->cairo;
        if (!cr) {
                cr = gdk_cairo_create(GTK_WIDGET(canvas)->window);
                canvas->cairo = cr;
        }
        */
        cr = gdk_cairo_create(GTK_WIDGET(canvas)->window);

        if (canvas->update_idle_id) {
                g_source_remove(canvas->update_idle_id);
                canvas->update_idle_id = 0;
        }

        canvas->flags |= CR_CANVAS_NEED_UPDATE;
        canvas->flags |= CR_CANVAS_IGNORE_INVALIDATE;

        cairo_identity_matrix(cr);
        cr_item_report_new_bounds(canvas->root, cr, FALSE);

        canvas->flags &= ~CR_CANVAS_NEED_UPDATE;
        canvas->flags &= ~CR_CANVAS_IGNORE_INVALIDATE;
        cairo_destroy(cr);
}

static void
on_item_request_update(CrCanvas *canvas)
{
        if (GTK_WIDGET_REALIZED(canvas) && !(canvas->flags & 
                        (CR_CANVAS_IN_EXPOSE | CR_CANVAS_NEED_UPDATE))) {

#if DEBUG_PROFILE
                overall_ticks = clock();
#endif

                if (canvas->update_idle_id)
                        g_source_remove(canvas->update_idle_id);

                if (canvas->flags & CR_CANVAS_REPAINT_MODE) {
                        canvas->update_idle_id = g_idle_add_full(
                                        GDK_PRIORITY_REDRAW - 20,
                                        (GSourceFunc) on_repaint_revert_idle,
                                        canvas, NULL);
                        gtk_widget_queue_draw(GTK_WIDGET(canvas));
                }
                else {
                        canvas->update_idle_id = g_idle_add_full(
                                        GDK_PRIORITY_REDRAW - 20,
                                        (GSourceFunc) on_update_idle,
                                        canvas, NULL);
                }
        }
        canvas->flags |= CR_CANVAS_NEED_UPDATE;
}

static void
on_item_invalidated(CrCanvas *canvas, int mask,
                double x1, double y1, double x2, double y2,
                CrDeviceBounds *device)
{
        GdkRectangle rect;

        if (canvas->flags & CR_CANVAS_IGNORE_INVALIDATE) return;

        if (device) {
                switch (device->anchor) {
                        case GTK_ANCHOR_N:
                        case GTK_ANCHOR_CENTER:
                        case GTK_ANCHOR_S:
                                rect.x = x1 + device->x1 - 1;
                                rect.width = x2 + device->x2 - rect.x + 2;
                                break;
                        case GTK_ANCHOR_NE:
                        case GTK_ANCHOR_E:
                        case GTK_ANCHOR_SE:
                                rect.x = x1 + device->x1 - 1;
                                rect.width = device->x2 - device->x1 + 2;
                                break;
                        case GTK_ANCHOR_NW:
                        case GTK_ANCHOR_W:
                        case GTK_ANCHOR_SW:
                                rect.x = x2 + device->x1 - 1;
                                rect.width = device->x2 - device->x1 + 2;
                                break;
                }
                switch (device->anchor) {
                        case GTK_ANCHOR_W:
                        case GTK_ANCHOR_CENTER:
                        case GTK_ANCHOR_E:
                                rect.y = y1 + device->y1 - 1;
                                rect.height = y2 + device->y2 - rect.y + 2;
                                break;
                        case GTK_ANCHOR_NW:
                        case GTK_ANCHOR_N:
                        case GTK_ANCHOR_NE:
                                rect.y = y1 + device->y1 - 1;
                                rect.height = device->y2 - device->y1 + 2;
                                break;
                        case GTK_ANCHOR_SW:
                        case GTK_ANCHOR_S:
                        case GTK_ANCHOR_SE:
                                rect.y = y2 + device->y1 - 1;
                                rect.height = device->y2 - device->y1 + 2;
                                break;
                }
#if DEBUG_INVALIDATE
                g_print("device invalidate %ix%i at %i %i\n", 
                                rect.width,rect.height,
                                rect.x,rect.y);
#endif
                gdk_window_invalidate_rect(GTK_WIDGET(canvas)->window, 
                                &rect, FALSE);

                if (device->anchor == GTK_ANCHOR_CENTER) return;

        }
        if (x2 - x1 <= 0 || y2 - y1 <= 0) return;

        /* Because the (int) casting rounds down, we need to add 1 to the 
         * width and the height.  For some reason it is actually necessary to 
         * add 2.  Possibly there is a compound rounding happening somewhere in
         * Cairo. */
        rect.x = (int) x1;
        rect.y = (int) y1;
        rect.width = (int) (x2 - x1 + 2);
        rect.height = (int) (y2 - y1 + 2);

#if DEBUG_INVALIDATE
        g_print("invalidate %ix%i at %i %i\n", rect.width,rect.height,
                        rect.x,rect.y);
#endif
        gdk_window_invalidate_rect(GTK_WIDGET(canvas)->window, 
                        &rect, FALSE);
}

static void
do_item_added(CrItem *newitem, CrCanvas *canvas)
{
        g_signal_connect_swapped(newitem, "request_update",
                        (GCallback)on_item_request_update, canvas);

        g_list_foreach(newitem->items, (GFunc) do_item_added, canvas);
}

static void
on_item_added(CrCanvas *canvas, CrItem *newitem, CrItem *root)
{
        do_item_added(newitem, canvas);
}

static void
do_item_removed(CrItem *olditem, CrCanvas *canvas)
{
        g_signal_handlers_disconnect_by_func(olditem, 
                        (GCallback)on_item_request_update, canvas);

        g_list_foreach(olditem->items, (GFunc) do_item_removed, canvas);
}

static void
on_item_removed(CrCanvas *canvas, CrItem *olditem, CrItem *root)
{
        do_item_removed(olditem, canvas);
}


static void
set_root(CrCanvas *canvas, CrItem *item)
{
        if (canvas->root) {
                g_signal_handlers_disconnect_by_func(canvas->root, 
                                (GCallback)on_item_added, canvas);
                g_signal_handlers_disconnect_by_func(canvas->root, 
                                (GCallback)on_item_removed, canvas);
                g_signal_handlers_disconnect_by_func(canvas->root, 
                                (GCallback)on_item_invalidated, canvas);
                g_object_unref(canvas->root);
                canvas->root = NULL;
        }
        if (item) {
                canvas->root = item;
                g_object_ref(item);
                g_signal_connect_swapped(canvas->root, "added",
                                (GCallback)on_item_added, canvas);
                g_signal_connect_swapped(canvas->root, "removed",
                                (GCallback)on_item_removed, canvas);
                g_signal_connect_swapped(canvas->root, "invalidate",
                                (GCallback)on_item_invalidated, canvas);
                on_item_added(canvas, canvas->root, NULL);
        }
}

/***********************/

static void
cr_canvas_dispose(GObject *object)
{
        CrCanvas *canvas;

        canvas = CR_CANVAS(object);

        if (canvas->update_idle_id) g_source_remove(canvas->update_idle_id);

        (*CR_CANVAS_GET_CLASS(canvas)->set_root)(canvas, NULL);
        
        if (canvas->hadjustment) g_object_unref(canvas->hadjustment);
        if (canvas->vadjustment) g_object_unref(canvas->vadjustment);
        canvas->hadjustment = canvas->vadjustment = NULL;

        parent_class->dispose(object);
}

static void
cr_canvas_finalize(GObject *object)
{
        parent_class->finalize(object);
}

static void
cr_canvas_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        CrCanvas *canvas = (CrCanvas*) object;
        gboolean bval, maintain_flag;
        guint32 flag;

        flag = 0;

        switch (property_id) {
                case PROP_HADJUSTMENT:
                        cr_canvas_set_hadjustment (canvas, 
				  (GtkAdjustment*) g_value_get_object (value));
                        break;
                case PROP_VADJUSTMENT:
                        cr_canvas_set_vadjustment (canvas, 
				  (GtkAdjustment*) g_value_get_object (value));
                        break;
                case PROP_MAINTAIN_CENTER:
                        flag = CR_CANVAS_MAINTAIN_CENTER;
                        break;
                case PROP_MAINTAIN_ASPECT:
                        flag = CR_CANVAS_MAINTAIN_ASPECT;
                        break;
                case PROP_AUTO_SCALE:
                        flag = CR_CANVAS_AUTO_SCALE;
                        break;
                case PROP_SHOW_LESS:
                        flag = CR_CANVAS_SHOW_LESS;
                        break;
                case PROP_REPAINT_MODE:
                        cr_canvas_set_repaint_mode(canvas, 
                                        g_value_get_boolean(value));
                        break;
                case PROP_ROOT:
                        (*CR_CANVAS_GET_CLASS(canvas)->set_root)(canvas,
                                        g_value_get_object(value));
                        break;
                case PROP_REPAINT_ON_SCROLL:
                        flag = CR_CANVAS_REPAINT_ON_SCROLL;
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
        if (flag) {
                bval = g_value_get_boolean(value);
                if (bval)
                        canvas->flags |= flag;
                else
                        canvas->flags &= ~flag;
        }
}

static void
cr_canvas_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrCanvas *canvas = (CrCanvas*) object;
        switch (property_id) {
                case PROP_HADJUSTMENT:
                        g_value_set_object (value, canvas->hadjustment);
                        break;
                case PROP_VADJUSTMENT:
                        g_value_set_object (value, canvas->vadjustment);
                        break;
                case PROP_MAINTAIN_CENTER:
                        g_value_set_boolean(value, canvas->flags &
                                        CR_CANVAS_MAINTAIN_CENTER);
                        break;
                case PROP_MAINTAIN_ASPECT:
                        g_value_set_boolean(value, canvas->flags &
                                        CR_CANVAS_MAINTAIN_ASPECT);
                        break;
                case PROP_AUTO_SCALE:
                        g_value_set_boolean(value, canvas->flags &
                                        CR_CANVAS_AUTO_SCALE);
                        break;
                case PROP_SHOW_LESS:
                        g_value_set_boolean(value, canvas->flags &
                                        CR_CANVAS_SHOW_LESS);
                        break;
                case PROP_REPAINT_MODE:
                        g_value_set_boolean(value, canvas->flags &
                                        CR_CANVAS_REPAINT_MODE);
                        break;
                case PROP_ROOT:
                        g_value_set_object (value, canvas->root);
                        break;
                case PROP_PICK_ITEM:
                        g_value_set_object (value, canvas->pick_item);
                        break;
                case PROP_REPAINT_ON_SCROLL:
                        g_value_set_boolean(value, canvas->flags &
                                        CR_CANVAS_REPAINT_ON_SCROLL);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_canvas_init(CrCanvas *canvas)
{
        CrItem *item;

        /* needed for key events.*/
        GTK_WIDGET_SET_FLAGS (canvas, GTK_CAN_FOCUS);

        item = g_object_new(CR_TYPE_ITEM, NULL);
        (*CR_CANVAS_GET_CLASS(canvas)->set_root)(canvas, item);
        g_object_unref(item);

        GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET(canvas), GTK_NO_WINDOW);

        canvas->flags |= CR_CANVAS_MAINTAIN_ASPECT;
        canvas->scroll_factor_x = canvas->scroll_factor_y = 1.0;
        cr_canvas_set_vadjustment(canvas, g_object_new(GTK_TYPE_ADJUSTMENT,
                                NULL));
        cr_canvas_set_hadjustment(canvas, g_object_new(GTK_TYPE_ADJUSTMENT,
                                NULL));
}

static gint
on_expose(GtkWidget *widget, GdkEventExpose *event)
{
        CrCanvas *canvas;
        cairo_t *cr;
        CrItem *item;
        gdouble dx, dy;

        if (GTK_WIDGET_CLASS(parent_class)->expose_event)
                GTK_WIDGET_CLASS(parent_class)->expose_event(widget, event);

        canvas = CR_CANVAS(widget);
        item = CR_ITEM(canvas->root);

        canvas->flags |= CR_CANVAS_IN_EXPOSE;

#if DEBUG_UPDATE
        g_print("expose %ix%i at %i,%i\n", event->area.width, 
                        event->area.height, event->area.x, event->area.y);
#endif

        /* The root group scaling may can be changed independently from the
         * canvas widget.  When using the scroll region (finite world) model,
         * this update ensures the GtkAdjustments are sync'd to the root group
         * scale . When using the scroll factor (infinite world) model, it
         * resets the scroll limits to conform to the window size.
         */
        update_adjustments(canvas);

        cr = gdk_cairo_create (widget->window);

        /* the GDK window pointer is attached here to the cairo context
         * to support an item implementation that wishes to draw directly to
         * the window without using cairo.  This is done in geoc-lines-faster
         * to improve performance
         */
        g_dataset_set_data(cr, "window", widget->window);

        cr_item_invoke_paint(item, cr, 
                        (canvas->flags & CR_CANVAS_REPAINT_MODE),
                        event->area.x, event->area.y,
                        event->area.x + event->area.width,
                        event->area.y + event->area.height);
        g_dataset_destroy(cr);
        cairo_destroy(cr);

        if (canvas->flags & CR_CANVAS_REPAINT_MODE)
                canvas->flags &= ~CR_CANVAS_NEED_UPDATE;

        canvas->flags &= ~CR_CANVAS_IN_EXPOSE;
        
#if DEBUG_PROFILE
        cr_item_print_stats();
        /*cr_path_print_stats();*/
        g_print("overall time = %lf secs\n", (double) (clock() - overall_ticks)/
                        CLOCKS_PER_SEC);
        overall_ticks = 0;
        g_print("-----------------------------------\n");
#endif
        return FALSE;
}

static void
maintain_center(CrCanvas *canvas, gdouble neww, gdouble newh, gdouble oldw,
                gdouble oldh)
{
        CrItem *item;
        gdouble dx, dy;
        int wx, wy;

        item = CR_ITEM(canvas->root);

        dx = (neww - oldw)/2;
        dy = (newh - oldh)/2;
        wx = (int) dx;
        wy = (int) dy;

        cairo_matrix_transform_distance(cr_item_get_inverse_matrix(item), 
                        &dx, &dy);

        cairo_matrix_translate(cr_item_get_matrix(item), dx, dy);

        /* this avoids the overhead from calling request_update() */
        *item->matrix_p = *item->matrix;

        if (GTK_WIDGET(canvas)->window)
                gdk_window_scroll(GTK_WIDGET(canvas)->window, wx, wy);
}

static void
calculate_scale_factors(CrCanvas *canvas, gdouble width, gdouble height, gdouble
                *factor_x, gdouble *factor_y)
{
        *factor_x = width / canvas->init_w;
        *factor_y = height / canvas->init_h;
        gboolean show_less, less_than;

        if (canvas->flags & CR_CANVAS_MAINTAIN_ASPECT) {

                /* If the aspect ratio of the window changes due to a resize, 
                 * should we show less area or more area ?
                 * Below a ">" will show less area.  A "<" will show more area.
                 */
                less_than = (fabs(1. - *factor_x) < fabs(1. - *factor_y));
                show_less = ((canvas->flags & CR_CANVAS_SHOW_LESS) != 0);
                if (less_than ^ show_less)
                        *factor_y = *factor_x;
                else
                        *factor_x = *factor_y;
        }
}

static void
auto_scale(CrCanvas *canvas, gdouble neww, gdouble newh, gdouble
                oldw, gdouble oldh)
{
        CrItem *item;
        gdouble cx, cy, ncx, ncy;
        gdouble old_fx, old_fy, new_fx, new_fy, factor_x, factor_y;
        cairo_matrix_t *matrix, *matrix_i;

        item = canvas->root;
        matrix = cr_item_get_matrix(item);
        matrix_i = cr_item_get_inverse_matrix(item);

        calculate_scale_factors(canvas, neww, newh, &new_fx, &new_fy);
        calculate_scale_factors(canvas, oldw, oldh, &old_fx, &old_fy);

        factor_x = new_fx / old_fx;
        factor_y = new_fy / old_fy;

        if (canvas->flags & CR_CANVAS_MAINTAIN_CENTER) {

                cx = oldw/2;
                cy = oldh/2;

        }
        else
                cx = cy = 0;

        cairo_matrix_transform_point(matrix_i, &cx, &cy);

        cairo_matrix_scale(matrix, factor_x, factor_y);
        matrix_i = cr_item_get_inverse_matrix(item);

        if (canvas->flags & CR_CANVAS_MAINTAIN_CENTER) {

                ncx = neww/2;
                ncy = newh/2;
        }
        else
                ncx = ncy = 0;

        cairo_matrix_transform_point(matrix_i, &ncx, &ncy);

        cairo_matrix_translate(matrix, ncx - cx, ncy - cy);

        cr_canvas_queue_repaint(canvas);
}

/* Used to prevent the user from zooming out larger than the
 * pre-defined world.
 */
static void
apply_min_scale_factor(CrCanvas *canvas, double width, double height)
{
        double x_factor, y_factor;

        x_factor = width / (canvas->scroll_x2-canvas->scroll_x1);
        y_factor = height / (canvas->scroll_y2-canvas->scroll_y1);

        if (canvas->flags & CR_CANVAS_MAINTAIN_ASPECT)
                x_factor = y_factor = MAX(x_factor, y_factor);

        cr_canvas_set_min_scale_factor(canvas, x_factor, y_factor);
}

static void 
size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
        CrCanvas *canvas;
        gdouble neww, newh, oldw, oldh;
        gdouble x_factor, y_factor;

        neww = allocation->width;
        newh = allocation->height;

        if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
                (* GTK_WIDGET_CLASS (parent_class)->size_allocate)(widget,
                                                                   allocation);

        canvas = CR_CANVAS(widget);

        /* We cannot use widget->allocation to determine previous width and 
         * height because it gets reset during an unparent operation.  This
         * widget may need to perform scaling after a reparent and therefore 
         * needs to know its previous allocation before it was unparented.
         */
        oldw = canvas->last_w;
        oldh = canvas->last_h;

        if (oldw > 1 && neww > 1) {

                if (canvas->flags & CR_CANVAS_SCROLL_WORLD)
                        apply_min_scale_factor(canvas, neww, newh);

                if (canvas->flags & CR_CANVAS_AUTO_SCALE)
                        auto_scale(canvas, neww, newh, oldw, oldh);

                else if (canvas->flags & CR_CANVAS_MAINTAIN_CENTER)
                        maintain_center(canvas, neww, newh, oldw, oldh);

        }
        else {
                /* initialization just pick any initial size and maintains
                 * scaling from that reference point. */
                canvas->init_w = neww;
                canvas->init_h = newh;
        }
        update_adjustments(canvas);
        canvas->last_w = neww;
        canvas->last_h = newh;
}

static void
set_scroll_adjustments(CrCanvas *canvas, GtkAdjustment *hadjustment,
                GtkAdjustment *vadjustment)
{
        if (hadjustment) cr_canvas_set_hadjustment (canvas, hadjustment);
        if (vadjustment) cr_canvas_set_vadjustment (canvas, vadjustment);
}

static void
realize(GtkWidget *widget)
{
        GdkWindowAttr attributes;
        gint      attributes_mask;
        CrCanvas *canvas;

        /* during a reparant these could be set from the previous parent
         * and need to be cleared. */
        canvas = CR_CANVAS(widget);
        if (canvas->pick_item) {
                g_object_remove_weak_pointer(G_OBJECT(canvas->pick_item),
                                (gpointer)&canvas->pick_item);
                canvas->pick_item = NULL;
        }
        canvas->pick_button = 0;

        GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
        attributes.x = widget->allocation.x;
        attributes.y = widget->allocation.y;
        attributes.width = widget->allocation.width;
        attributes.height = widget->allocation.height;
        attributes.wclass = GDK_INPUT_OUTPUT;
        attributes.window_type = GDK_WINDOW_CHILD;
        attributes.event_mask = GDK_EXPOSURE_MASK |
                gtk_widget_get_events (widget);
        attributes.visual = gtk_widget_get_visual (widget);

        attributes.colormap = gtk_widget_get_colormap (widget);
        attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;        widget->window = gdk_window_new (widget->parent->window,
                &attributes, attributes_mask);

        gdk_window_set_events (widget->window, 
                        (gdk_window_get_events (widget->window )
                                | GDK_EXPOSURE_MASK
                                | GDK_BUTTON_PRESS_MASK
                                | GDK_BUTTON_RELEASE_MASK
                                | GDK_POINTER_MOTION_MASK
                                | GDK_KEY_PRESS_MASK
                                | GDK_KEY_RELEASE_MASK
                                | GDK_ENTER_NOTIFY_MASK
                                | GDK_LEAVE_NOTIFY_MASK
                                | GDK_FOCUS_CHANGE_MASK));

        widget->style = gtk_style_attach (widget->style, widget->window);
        gtk_style_set_background (widget->style, widget->window, 
                GTK_STATE_NORMAL);
        gdk_window_set_user_data (widget->window, widget);

        recenter_adjustments(CR_CANVAS(widget));
}

static void
map(GtkWidget *widget)
{
        CrCanvas *canvas;
        if (GTK_WIDGET_CLASS(parent_class)->map)
                GTK_WIDGET_CLASS(parent_class)->map(widget);

        /* guarantee the item tree is fully updated before the first expose
         * event. */
        canvas = CR_CANVAS(widget);
        if (canvas->flags & CR_CANVAS_NEED_UPDATE)
                (*CR_CANVAS_GET_CLASS(canvas)->update)(canvas);
}

static gboolean
motion_event(GtkWidget *widget, GdkEventMotion *event)
{
        /*To transform between device space (events produced by GTK+) and
         * cairo’s user space you can use the function cairo inverse transform
         * point (cairo t *cr, double *x, double *y) which transforms the values
         * stored at the ad-dresses pointed to by the arguments.*/

        CrItem *root_item, *new_item;
        CrCanvas *canvas;
        cairo_t *cr;
        GdkEvent event_copy;
        double x, y;
        gboolean state;
        cairo_matrix_t pick_matrix;

        canvas = CR_CANVAS(widget);
        root_item = CR_ITEM(canvas->root);
        state = FALSE;
        event_copy = *((GdkEvent *) event);
        new_item = NULL;

        cr = gdk_cairo_create (widget->window);

        cairo_matrix_init_identity(&pick_matrix);
        if (canvas->pick_item)
                cr_item_find_child(root_item, &pick_matrix, canvas->pick_item);

        if (canvas->pick_button && canvas->pick_item) {

                state = cr_item_invoke_event(canvas->pick_item,
                                (GdkEvent*) event, &pick_matrix,
                                canvas->pick_item);

        }
        else {
                canvas->pick_button = 0;
                cairo_identity_matrix(cr);

                new_item = cr_item_invoke_test(root_item,
                                cr,
                                event->x,
                                event->y);

                if (new_item == canvas->pick_item) {
                        cairo_destroy(cr);
                        return state;
                }
                /* should i send motion events to the item?*/

                event_copy.crossing.x = event->x;
                event_copy.crossing.y = event->y;
                event_copy.crossing.x_root = event->x_root;
                event_copy.crossing.y_root = event->y_root;

                if (canvas->pick_item) {
                        event_copy.type = GDK_LEAVE_NOTIFY;
                        state |= cr_item_invoke_event(
                                        canvas->pick_item,
                                        &event_copy,
                                        &pick_matrix,
                                        canvas->pick_item);
                }
                if (new_item) {
                        cairo_get_matrix(cr, &pick_matrix);

                        event_copy.type = GDK_ENTER_NOTIFY;
                        canvas->pick_item = new_item;
                        /* this handles case where item deleted
                         * between events */
                        g_object_add_weak_pointer(
                                G_OBJECT(new_item),
                                (gpointer)&canvas->pick_item);
                        state |= cr_item_invoke_event(new_item,
                                        &event_copy,
                                        &pick_matrix,
                                        canvas->pick_item);
                }
                else {
                        g_object_remove_weak_pointer(
                                G_OBJECT(canvas->pick_item),
                                (gpointer)&canvas->pick_item);
                        canvas->pick_item = NULL;
                }
        }
        cairo_destroy(cr);
        return state;
}

static gboolean
button_event(GtkWidget *widget, GdkEventButton *event)
{
        CrCanvas *canvas;
        CrItem *root_item;
        gboolean state;
        cairo_matrix_t pick_matrix;

        canvas = CR_CANVAS(widget);
        root_item = CR_ITEM(canvas->root);
        state = FALSE;

        cairo_matrix_init_identity(&pick_matrix);
        if (canvas->pick_item)
                cr_item_find_child(root_item, &pick_matrix, canvas->pick_item);

        switch (event->type) {
                case GDK_BUTTON_PRESS:
                case GDK_2BUTTON_PRESS:
                case GDK_3BUTTON_PRESS:
                        canvas->pick_button = event->button;
                        if (canvas->pick_item) {
                                state = cr_item_invoke_event(canvas->pick_item,
                                        (GdkEvent *)event, &pick_matrix,
                                        canvas->pick_item);
                        }
                        break;

                case GDK_BUTTON_RELEASE:
                        canvas->pick_button = 0;
                        if (canvas->pick_item) {
                                state = cr_item_invoke_event(canvas->pick_item,
                                        (GdkEvent*) event, &pick_matrix,
                                        canvas->pick_item);
                        }
                        break;
        }
        return state;
}

static gboolean
key_event(GtkWidget *widget, GdkEventKey *event)
{
        CrCanvas *canvas;
        CrItem *root_item;
        gboolean state;
        cairo_matrix_t pick_matrix;

        canvas = CR_CANVAS(widget);
        root_item = CR_ITEM(canvas->root);
        state = FALSE;

        if (canvas->pick_item) {

                cairo_matrix_init_identity(&pick_matrix);
                cr_item_find_child(root_item, &pick_matrix, canvas->pick_item);

                state = cr_item_invoke_event(canvas->pick_item,
                                (GdkEvent*) event, &pick_matrix,
                                canvas->pick_item);
        }
        return state;
}

static gint
focus_in(GtkWidget *widget, GdkEventFocus *event)
{
        GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
        return FALSE;
}

static gint
focus_out(GtkWidget *widget, GdkEventFocus *event)
{
        CrCanvas *canvas;
        CrItem *root_item;
        GdkEvent event_copy;
        cairo_matrix_t pick_matrix;

        canvas = CR_CANVAS(widget);
        root_item = CR_ITEM(canvas->root);

        GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

        if (canvas->pick_item) {

                cairo_matrix_init_identity(&pick_matrix);
                cr_item_find_child(root_item, &pick_matrix, canvas->pick_item);

                event_copy = *((GdkEvent *) event);

                event_copy.type = GDK_LEAVE_NOTIFY;

                cr_item_invoke_event(
                                canvas->pick_item,
                                &event_copy,
                                &pick_matrix,
                                canvas->pick_item);

                g_object_remove_weak_pointer(G_OBJECT(canvas->pick_item),
                                (gpointer)&canvas->pick_item);
                canvas->pick_item = NULL;
        }

        return FALSE;
}

static gint
enter_notify(GtkWidget *widget, GdkEventCrossing *event)
{
        /* This is used to guarantee that canvas items will always get 
         * a leave notify event to correspond to every enter notify
         * when the window goes out of focus.
         */
        gtk_widget_grab_focus(widget);
        return FALSE;
}

static void
cr_canvas_class_init(CrCanvasClass *klass)
{
        GObjectClass *object_class;
        GtkWidgetClass *widget_class;

        object_class = (GObjectClass *) klass;
        widget_class = (GtkWidgetClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->get_property = cr_canvas_get_property;
        object_class->set_property = cr_canvas_set_property;
        object_class->dispose = cr_canvas_dispose;
        object_class->finalize = cr_canvas_finalize;

        widget_class->expose_event = on_expose;

        klass->set_scroll_adjustments = set_scroll_adjustments;
        klass->set_root = set_root;
        klass->update = update;
        klass->quick_update = quick_update;

        widget_class->size_allocate = size_allocate;
        widget_class->realize = realize;
        widget_class->map = map;

        /* The general "event" signal is not used in this widget so that a user
         * connecting to the specific signal can block the this widget by
         * returning TRUE.*/
        widget_class->button_press_event = button_event;
        widget_class->button_release_event = button_event;
        widget_class->motion_notify_event = motion_event;
        widget_class->key_press_event = key_event;
        widget_class->key_release_event = key_event;
        widget_class->focus_in_event = focus_in;
        widget_class->focus_out_event = focus_out;
        widget_class->enter_notify_event = enter_notify;

        /*
        widget_class->expose_event =
        widget_class->enter_notify_event
        widget_class->leave_notify_event
        */

        widget_class->set_scroll_adjustments_signal = g_signal_new
                ("set_scroll_adjustments",
                 G_OBJECT_CLASS_TYPE (object_class),
                 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                 G_STRUCT_OFFSET(CrCanvasClass, set_scroll_adjustments),
                 NULL, NULL,
                 cr_marshal_VOID__OBJECT_OBJECT,
                 G_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

        cr_canvas_signals[SCROLL_REGION_CHANGED] = g_signal_new(
                        "scroll_region_changed", CR_TYPE_CANVAS,
                        G_SIGNAL_RUN_FIRST,
                        G_STRUCT_OFFSET(CrCanvasClass, scroll_region_changed),
                        NULL, NULL,
                        cr_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);

        cr_canvas_signals[BEFORE_PAINT] = g_signal_new(
                        "before_paint", CR_TYPE_CANVAS,
                        G_SIGNAL_RUN_FIRST,
                        G_STRUCT_OFFSET(CrCanvasClass, before_paint),
                        NULL, NULL,
                        cr_marshal_VOID__BOXED_BOOLEAN,
                        G_TYPE_NONE, 2, CR_TYPE_CONTEXT, G_TYPE_BOOLEAN);

        g_object_class_install_property (object_class, PROP_HADJUSTMENT,
                        g_param_spec_object ("hadjustment", 
                               "Horizontal adjustment",
                               "The GtkAdjustment for the horizontal position",
                                GTK_TYPE_ADJUSTMENT, G_PARAM_READWRITE));
        g_object_class_install_property (object_class, PROP_VADJUSTMENT,
                        g_param_spec_object ("vadjustment", 
                               "Vertical adjustment",
                               "The GtkAdjustment for the vertical position",
                                GTK_TYPE_ADJUSTMENT, G_PARAM_READWRITE));
        g_object_class_install_property (object_class, PROP_MAINTAIN_CENTER,
                        g_param_spec_boolean("maintain_center",
                                "Maintain Center",
                                "World center point remains on resize.",
                                FALSE,
                                G_PARAM_READWRITE));
        g_object_class_install_property (object_class, PROP_MAINTAIN_ASPECT,
                        g_param_spec_boolean("maintain_aspect",
                                "Maintain Aspect",
                                "Aspect ratio is maintained on resize and"
                                "zooming events.",
                                TRUE,
                                G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                        PROP_AUTO_SCALE,
                        g_param_spec_boolean("auto_scale",
                                "Auto Scale",
                                "Present viewport contents are retained on"
                                " resize. This means items will be zoomed"
                                " in or out depending on how the window "
                                "changes.",
                                FALSE,
                                G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                        PROP_SHOW_LESS,
                        g_param_spec_boolean("show_less",
                                "Show Less",
                                "Whether to show more area or less area on "
                                "a viewport resize when the aspect ratio "
                                "changes.  This only has an effect when both "
                                "auto-scale and maintain-aspect are set.",
                                FALSE,
                                G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                        PROP_REPAINT_MODE,
                        g_param_spec_boolean("repaint_mode",
                                "Repaint Mode",
                                "Canvas repaints all items on each update.",
                                FALSE,
                                G_PARAM_READWRITE));
        g_object_class_install_property (object_class, PROP_ROOT,
                        g_param_spec_object ("root", 
                               "Root Canvas Item",
                               "The root canvas item for this widget",
                                CR_TYPE_ITEM, G_PARAM_READWRITE));
        g_object_class_install_property (object_class, PROP_PICK_ITEM,
                        g_param_spec_object ("pick-item", 
                               "Cursor Picked Item",
                               "The lowest item in the tree presently "
                               "receiving cursor events.",
                                CR_TYPE_ITEM, G_PARAM_READABLE));
        g_object_class_install_property (object_class, PROP_REPAINT_ON_SCROLL,
                g_param_spec_boolean("repaint-on-scroll",
                        "Repaint on Scroll",
                        "Repaint the whole canvas when the scrollbar is moved."
                        " Default behavior is to move the previously painted "
                        "area,  This changes the default behavior to repaint "
                        " the whole canvas instead. This is needed for the "
                        "use case where something on the canvas will be changed"
                        " immediately as a result of the scroll action.",
                        FALSE,
                        G_PARAM_READWRITE));
}

GType
cr_canvas_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrCanvasClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_canvas_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrCanvas),
                0,
                (GInstanceInitFunc) cr_canvas_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(GTK_TYPE_WIDGET,
                        "CrCanvas", &info, 0);
        }
        return type;
}

/* This function is used to preset a dummy size in the event the widget has not
 * been realized.  It is necessary to allow the user to make scale request prior
 * to rendering for the first time. The first call to size_allocate after
 * realizing the widget will fix any scaling problems.
 */
static void
get_size(CrCanvas *canvas, double *width, double *height)
{
        GtkWidget *widget;

        widget = GTK_WIDGET(canvas);

        if (widget->allocation.width <= 1 || widget->allocation.height <= 1) {

                /* could possible take the request value here if any */

                canvas->init_w = canvas->init_h = canvas->last_w = 
                        canvas->last_h = widget->allocation.width =
                        widget->allocation.height = 100;
        }

        *width = (double) widget->allocation.width;
        *height = (double) widget->allocation.height;
}

/* This function validates any given zoom scales to ensure they are
 * within the min/max scale factors.
 */
static void
limit_scale(CrCanvas *canvas, double *x_scale, double *y_scale)
{
        cairo_matrix_t matrix;
        CrItem *item;
        double current_x_scale, current_y_scale;

        item = CR_ITEM(canvas->root);
        matrix = *(cr_item_get_matrix(item));

        /* remove rotation - scale is limited as if there were no rotation */
        cairo_matrix_rotate(&matrix, -atan2(matrix.yx, matrix.yy));

        current_x_scale = current_y_scale = 1;
        cairo_matrix_transform_distance(&matrix, &current_x_scale, 
                        &current_y_scale);

        if (canvas->flags & CR_CANVAS_MAX_SCALE_FACTOR) {

                if (*x_scale * current_x_scale > canvas->max_x_scale_factor)
                        *x_scale = canvas->max_x_scale_factor / current_x_scale;
                 
                if (*y_scale * current_y_scale > canvas->max_y_scale_factor)
                        *y_scale = canvas->max_y_scale_factor / current_y_scale;

        }
        if (canvas->flags & CR_CANVAS_MIN_SCALE_FACTOR) {

                if (*x_scale * current_x_scale < canvas->min_x_scale_factor)
                        *x_scale = canvas->min_x_scale_factor / current_x_scale;
                 
                if (*y_scale * current_y_scale < canvas->min_y_scale_factor)
                        *y_scale = canvas->min_y_scale_factor / current_y_scale;
        }
}

static void
on_adjustment_value_changed (GtkAdjustment *adjustment,
                        CrCanvas     *canvas)
{
        CrItem *item;
        double dx, dy;
        int wx, wy;

        item = canvas->root;

        dx = wx = -(canvas->hadjustment->value - canvas->value_x);
        dy = wy = -(canvas->vadjustment->value - canvas->value_y);

        cairo_matrix_transform_distance(cr_item_get_inverse_matrix(item), 
                        &dx, &dy);

        double tmpx = canvas->hadjustment->value;
        double tmpy = canvas->vadjustment->value;
        canvas->value_x = canvas->hadjustment->value;
        canvas->value_y = canvas->vadjustment->value;

        if (dx != 0 || dy != 0) {
                cairo_matrix_translate(cr_item_get_matrix(item), dx, dy);

                /* this avoids the overhead from calling request_update() */
                *item->matrix_p = *item->matrix;

                canvas->flags |= CR_CANVAS_VIEWPORT_CHANGED;

                if (canvas->flags & CR_CANVAS_REPAINT_ON_SCROLL)
                        cr_canvas_queue_repaint(canvas);
                else
                        /* In this case it is safe to scroll the window without
                         * updating the item tree since so we don't request an
                         * update of the root item */
                        gdk_window_scroll(GTK_WIDGET(canvas)->window, wx, wy);
        }
}

static void
setup_adjustment(CrCanvas *canvas, GtkAdjustment *old, GtkAdjustment *new)
{
        double value;

        if (old) {
                new->upper = old->upper;
                new->lower = old->lower;
                new->page_size = old->page_size;
                new->page_increment = old->page_increment;
                new->step_increment = old->step_increment;
                new->value = old->value;

                g_signal_handlers_disconnect_by_func(old,
                                on_adjustment_value_changed, canvas);
                g_object_unref(old);
        }
        g_object_ref (new);
        gtk_object_sink (GTK_OBJECT (new));
        g_signal_connect (new, "value_changed",
                        G_CALLBACK (on_adjustment_value_changed),
                        canvas);
}

/**
 * cr_canvas_set_hadjustment:
 * @canvas: a #CrCanvas
 * @adjustment: new scroll adjustment
 *
 * Sets the horizontal scroll adjustment for the layout.
 *
 * See #GtkScrolledWindow, #GtkScrollbar, #GtkAdjustment for details.
 * 
 **/
void           
cr_canvas_set_hadjustment (CrCanvas     *canvas,
                        GtkAdjustment *adjustment)
{
        setup_adjustment(canvas, canvas->hadjustment, adjustment);
        canvas->value_x = adjustment->value;
        canvas->hadjustment = adjustment;
        g_object_notify (G_OBJECT (canvas), "hadjustment");
}
 
/**
 * cr_canvas_set_vadjustment:
 * @canvas: a #CrCanvas
 * @adjustment: new scroll adjustment
 *
 * Sets the vertical scroll adjustment for the layout.
 *
 * See #GtkScrolledWindow, #GtkScrollbar, #GtkAdjustment for details.
 * 
 **/
void           
cr_canvas_set_vadjustment (CrCanvas     *canvas,
                        GtkAdjustment *adjustment)
{
        setup_adjustment(canvas, canvas->vadjustment, adjustment);
        canvas->value_y = adjustment->value;
        canvas->vadjustment = adjustment;
        g_object_notify (G_OBJECT (canvas), "vadjustment");
}

/**
 * cr_canvas_set_scroll_region:
 * @canvas:
 * @scroll_x1: The leftmost world boundary as defined by the root group matrix.
 * @scroll_y1: The topmost world boundary as defined by the root group matrix.
 * @scroll_x2: The rightmost world boundary as defined by the root group matrix.
 * @scroll_y2: The bottomost world boundary as defined by the root matrix.
 * 
 * Sets the canvas to a finite "world".  The scrollbars or adjustments will not
 * allow the canvas to scroll outside the input boundaries.  The world to screen
 * coordinate conversions are defined by the matrix held within the root group
 * item.
 **/
void
cr_canvas_set_scroll_region(CrCanvas *canvas, gdouble scroll_x1,
                gdouble scroll_y1, gdouble scroll_x2, gdouble scroll_y2)
{
        double width, height;

        canvas->scroll_x1 = scroll_x1;
        canvas->scroll_y1 = scroll_y1;
        canvas->scroll_x2 = scroll_x2;
        canvas->scroll_y2 = scroll_y2;

        canvas->flags |= CR_CANVAS_SCROLL_WORLD;

        get_size(canvas, &width, &height);

        apply_min_scale_factor(canvas, width, height);

        update_adjustments(canvas);
        g_signal_emit(canvas, cr_canvas_signals[SCROLL_REGION_CHANGED], 0);
}

/**
 * cr_canvas_get_scroll_region:
 * @canvas:
 * @scroll_x1: The leftmost world boundary as defined by the root group matrix.
 * @scroll_y1: The topmost world boundary as defined by the root group matrix.
 * @scroll_x2: The rightmost world boundary as defined by the root group matrix.
 * @scroll_y2: The bottomost world boundary as defined by the root matrix.
 * 
 * Gets the current scrolling area of the canvas.  If the canvas is in 'infinite
 * world' mode, this region will change often.
 **/
void
cr_canvas_get_scroll_region(CrCanvas *canvas, gdouble *scroll_x1, gdouble
                *scroll_y1, gdouble *scroll_x2, gdouble *scroll_y2)
{
        CrItem *item;

        if (canvas->flags & CR_CANVAS_SCROLL_WORLD) {
                *scroll_x1 = canvas->scroll_x1;
                *scroll_y1 = canvas->scroll_y1;
                *scroll_x2 = canvas->scroll_x2;
                *scroll_y2 = canvas->scroll_y2;
        }
        else {
                *scroll_x1 = canvas->hadjustment->lower - 
                        canvas->hadjustment->value;
                *scroll_y1 = canvas->vadjustment->lower - 
                        canvas->vadjustment->value;
                *scroll_x2 = canvas->hadjustment->upper - 
                        canvas->hadjustment->value;
                *scroll_y2 = canvas->vadjustment->upper - 
                        canvas->vadjustment->value;

                item = CR_ITEM(canvas->root);

                cairo_matrix_transform_point(cr_item_get_inverse_matrix(item),
                                scroll_x1, scroll_y1);
                cairo_matrix_transform_point(cr_item_get_inverse_matrix(item),
                                scroll_x2, scroll_y2);
        }
}

/**
 * cr_canvas_set_scroll_factor:
 * @canvas:
 * @scroll_factor_x: A multiplier for how much additional width scrolling.
 * @scroll_factor_y: A multiplier for how much additional height scrolling.
 *
 * This tells the canvas to assume that the world boundaries are infinite, but
 * that scrolling should be arbitrarily limited to a factor of the displayed
 * viewport size. i.e. a factor of 2 would allow a viewport showing a 100 unit
 * range to be scrolled over a 200 unit range. This call causes the canvas to be
 * in no way dependent on any setting in the root group item.
 **/
void 
cr_canvas_set_scroll_factor(CrCanvas *canvas, double scroll_factor_x, double
                scroll_factor_y)
{
        canvas->scroll_factor_x = scroll_factor_x;
        canvas->scroll_factor_y = scroll_factor_y;
        canvas->flags &= ~CR_CANVAS_SCROLL_WORLD;
        update_adjustments(canvas);
        recenter_adjustments(canvas);
}

/**
 * cr_canvas_scroll_to:
 * @canvas:
 * @x: The x coordinate for the left border
 * @y: The y coordinate for the upper border
 *
 * Places the given coordinate in the upper left corner of the current viewport.
 * Coordinates are expected to be in user space and are translated to device
 * space using the root group matrix.  If finite world scrolling is used, it may
 * be impossible to set the center to the requested coordinate.  If infinite
 * world scrolling is used the scroll area will be adjusted such that the
 * requested center is also the center of the scroll region. This function is
 * typically used with a document model when the 'auto_scale' and
 * 'maintain_center' properties are disabled.
 */
void
cr_canvas_scroll_to(CrCanvas *canvas, gdouble x, gdouble y)
{
        CrItem *item;
        double sx, sy, w, h, dx, dy;

        item = CR_ITEM(canvas->root);

        if  (canvas->flags & CR_CANVAS_SCROLL_WORLD) {
                get_size(canvas, &w, &h);
                cairo_matrix_transform_distance(cr_item_get_inverse_matrix(
                                        item), &w, &h);
                x = CLAMP(x, canvas->scroll_x1, canvas->scroll_x2 - w);
                y = CLAMP(y, canvas->scroll_y1, canvas->scroll_y2 - h);
        }

        sx = sy = 0;

        cairo_matrix_transform_point(cr_item_get_inverse_matrix(item), 
                        &sx, &sy);

        dx = sx - x;
        dy = sy - y;

        cairo_matrix_translate(cr_item_get_matrix(item), dx, dy);

        cairo_matrix_transform_distance(cr_item_get_matrix(item), &dx, &dy);

        /* this is smoother?*/
        cr_item_request_update(item);
        /*
        gdk_window_scroll(GTK_WIDGET(canvas)->window, (int)dx, (int)dy);
        */

        canvas->flags |= CR_CANVAS_VIEWPORT_CHANGED;
}

/**
 * cr_canvas_get_scroll_offsets:
 * @canvas:
 * @x: The x coordinate of the left border
 * @y: The y coordinate of the upper border
 *
 * A convenience function to get the upper leftmost canvas coordinate in root
 * group units.
 */
void
cr_canvas_get_scroll_offsets(CrCanvas *canvas, gdouble *x, gdouble *y)
{
        CrItem *item;

        item = CR_ITEM(canvas->root);

        *x = *y = 0;

        cairo_matrix_transform_point(cr_item_get_inverse_matrix(item), x, y);
}


/**
 * cr_canvas_center_on:
 * @canvas:
 * @x: The x coordinate to set center to.
 * @y: The y coordinate to set center to.
 *
 * Places the given coordinate in the center of the current viewport.
 * Coordinates are expected to be in user space and are translated to device
 * space using the root group matrix.  If finite world scrolling is used, it may
 * be impossible to set the center to the requested coordinate.  If infinite
 * world scrolling is used the scroll area will be adjusted such that the
 * requested center is also the center of the scroll region. This function is
 * typically used when the 'auto_scale' or 'maintain_center' properties are set.
 */
void
cr_canvas_center_on(CrCanvas *canvas, gdouble x, gdouble y)
{
        gdouble w, h, l, old_x, old_y, half_w, half_h, dx, dy;
        CrItem *item;

        item = CR_ITEM(canvas->root);

        get_size(canvas, &w, &h);

        old_x = half_w = w / 2.;
        old_y = half_h = h / 2.;

        cairo_matrix_transform_point(cr_item_get_inverse_matrix(item), 
                        &old_x, &old_y);

        if (canvas->flags & CR_CANVAS_SCROLL_WORLD) {
                cairo_matrix_transform_distance(cr_item_get_inverse_matrix(
                                        item), &half_w, &half_h);
                x = CLAMP(x, canvas->scroll_x1 + half_w, 
                                canvas->scroll_x2 - half_w);
                y = CLAMP(y, canvas->scroll_y1 + half_h, 
                                canvas->scroll_y2 - half_h);
        }

        dx = old_x - x;
        dy = old_y - y;

        cairo_matrix_translate(cr_item_get_matrix(item), dx, dy);

        recenter_adjustments(canvas);

        cairo_matrix_transform_distance(cr_item_get_matrix(item), &dx, &dy);

        /* this is smoother?*/
        cr_item_request_update(item);
        /*
        gdk_window_scroll(GTK_WIDGET(canvas)->window, (int)dx, (int)dy);
        */

        canvas->flags |= CR_CANVAS_VIEWPORT_CHANGED;
}

/**
 * cr_canvas_get_center:
 * @canvas:
 * @x: The x coordinate of the center.
 * @y: The y coordinate of the center.
 *
 * Convenience function to get the center point of the viewport in root group
 * coordinates.
 */
void
cr_canvas_get_center(CrCanvas *canvas, gdouble *x, gdouble *y)
{
        CrItem *item;
        double w, h;

        item = CR_ITEM(canvas->root);

        get_size(canvas, &w, &h);

        *x = w / 2;
        *y = h / 2;

        cairo_matrix_transform_point(cr_item_get_inverse_matrix(item), x, y);
}


/**
 * cr_canvas_center_scale:
 * @canvas:
 * @x: The center x coordinate in user space.
 * @y: The center y coordinate in user space.
 * @w: The requested width in user space.
 * @h: The requested height in user space.
 *
 * Centers and scales the canvas in one call.  If a finite world scrolling is
 * used, it may be impossible to honor this request.  If infinite world
 * scrolling is used the scrollable area will be adjusted such that the
 * requested center is now the center of the scroll area. If the maintain_aspect
 * property is TRUE, the scale request will be honored such that an equal to or
 * larger area will be shown in the viewport.
 */
void
cr_canvas_center_scale(CrCanvas *canvas,
                gdouble x, gdouble y, gdouble w, gdouble h)
{
        gdouble width, height, cx, cy, sx, sy, hval, vval, z;
        CrItem *item;
        cairo_matrix_t *matrix, *matrix_i;

        item = CR_ITEM(canvas->root);
        matrix = cr_item_get_matrix(item);
        matrix_i = cr_item_get_inverse_matrix(item);

        get_size(canvas, &width, &height);

        cx = width/2;
        cy = height/2;


        /* using 1 coord as zero finds distance as if there was no
         * rotation.
         */
        z = 0;
        cairo_matrix_transform_distance(matrix_i, &width, &z);
        z = 0;
        cairo_matrix_transform_distance(matrix_i, &z, &height);

        sx = width/w;
        sy = height/h;

        if (canvas->flags & CR_CANVAS_MAINTAIN_ASPECT)
                if (fabs(1. - sx) < fabs(1. - sy))
                        sy = sx;
                else
                        sx = sy;

        limit_scale(canvas, &sx, &sy);

        cairo_matrix_scale(matrix, sx, sy);
        matrix_i = cr_item_get_inverse_matrix(item);

        cairo_matrix_transform_point(matrix_i, &cx, &cy);

        cairo_matrix_translate(matrix, cx - x, cy - y); 

        /* auto scale saved parameters should be reset here as a new viewport
         * has been requested. 
         */
        get_size(canvas, &canvas->init_w, &canvas->init_h);

        canvas->flags |= CR_CANVAS_VIEWPORT_CHANGED;

        cr_item_request_update(item);
}

/**
 * cr_canvas_get_center_scale:
 * @canvas:
 * @cx: The center x coordinate in user space.
 * @cy: The center y coordinate in user space.
 * @w: The width in user space.
 * @h: The height in user space.
 *
 * Passes back the center and width and height in user space.
 */
void
cr_canvas_get_center_scale(CrCanvas *canvas,
                gdouble *cx, gdouble *cy, gdouble *w, gdouble *h)
{
        CrItem *item;
        cairo_matrix_t *matrix_i;

        get_size(canvas, w, h);

        *cx = *w/2;
        *cy = *h/2;

        item = CR_ITEM(canvas->root);
        matrix_i = cr_item_get_inverse_matrix(item);
        cairo_matrix_transform_distance(matrix_i, w, h);
        cairo_matrix_transform_point(matrix_i, cx, cy);
}

/**
 * cr_canvas_set_viewport:
 * @canvas:
 * @x1: The leftmost coordinate in user space.
 * @y1: The topmost coordinate in user space.
 * @x2: The rightmost coordinate in user space.
 * @y2: The bottommost coordinate in user space.
 *
 * A simple wrapper around cr_canvas_center_scale.  This procedure attempts to
 * set the canvas viewport to honor the coordinates provided.  If necessary, the
 * canvas will be re-centeredand re-scaled.  If a finite world scrolling is
 * used, it may be impossible to honor this request.  If infinite world
 * scrolling is used the scrollable area will be adjusted such that the
 * requested center is now the center of the scroll area. If the maintain_aspect
 * property is TRUE, the scale request will be honored such that an equal to or
 * larger area will be shown in the viewport.
 */
void
cr_canvas_set_viewport(CrCanvas *canvas,
                gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
        cr_canvas_center_scale(canvas, x1 + (x2 - x1) / 2,
                        y1 + (y2 - y1) / 2, x2 - x1, y2 - y1);
}

/**
 * cr_canvas_get_viewport:
 * @canvas:
 * @x1: The leftmost coordinate in user space.
 * @y1: The topmost coordinate in user space.
 * @x2: The rightmost coordinate in user space.
 * @y2: The bottommost coordinate in user space.
 *
 * The four corners of the current viewport in user space.
 */
void
cr_canvas_get_viewport(CrCanvas *canvas,
                gdouble *x1, gdouble *y1, gdouble *x2, gdouble *y2)
{
        CrItem *item;
        cairo_matrix_t *matrix_i;

        item = CR_ITEM(canvas->root);
        matrix_i = cr_item_get_inverse_matrix(item);

        *x1 = *y1 = 0;
        get_size(canvas, x2, y2);

        cairo_matrix_transform_point(matrix_i, x1, y1);
        cairo_matrix_transform_point(matrix_i, x2, y2);
}

/**
 * cr_canvas_zoom:
 * @canvas:
 * @x_factor: The zoom factor by which to reduce/increase the viewable area.
 * @y_factor: The zoom factor by which to reduce/increase the viewable area.
 *
 * Zooms the viewable area of the canvas in or out.  Values greater than 1 zoom
 * in, less than 1 zoom out. If the auto_scale or maintain_center properties are
 * set, the viewport will be zoomed about the current center point, otherwise
 * the viewport will be zoomed from upper left to lower right such that the
 * upper left user coordinate is maintained. For a canvas where the
 * maintain_aspect property is set, only the x_factor is used.
 */
void
cr_canvas_zoom(CrCanvas *canvas, gdouble x_factor, gdouble y_factor)
{
        gdouble cx, cy, ncx, ncy;
        CrItem *item;
        cairo_matrix_t *matrix, *matrix_i;

        item = CR_ITEM(canvas->root);
        matrix = cr_item_get_matrix(item);
        matrix_i = cr_item_get_inverse_matrix(item);

        if (canvas->flags & CR_CANVAS_MAINTAIN_ASPECT)
                y_factor = x_factor;

        if (canvas->flags & CR_CANVAS_MAINTAIN_CENTER) {

                get_size(canvas, &cx, &cy);
                ncx = cx = cx/2;
                ncy = cy = cy/2;

                cairo_matrix_transform_point(matrix_i, &cx, &cy);
        }

        limit_scale(canvas, &x_factor, &y_factor);

        cairo_matrix_scale(matrix, x_factor, y_factor);
        matrix_i = cr_item_get_inverse_matrix(item);

        if (canvas->flags & CR_CANVAS_MAINTAIN_CENTER) {

                cairo_matrix_transform_point(matrix_i, &ncx, &ncy);
                cairo_matrix_translate(matrix, ncx - cx, ncy - cy);
        }

        canvas->flags |= CR_CANVAS_VIEWPORT_CHANGED;

        cr_item_request_update(item);
}

/**
 * cr_canvas_zoom_world:
 * @canvas:
 *
 * Causes the canvas scale to zoom-out to show its maximum extents (or whole
 * world).  The maximum extents are determined as follows: (1) If finite world
 * model (set_scroll_region), then such coordinates are used to define the new
 * viewport.  (2) If infinite world model and the min_scale_factor has been
 * defined, then this is used.  (3) Else the canvas is zoomed by the current
 * scroll_factor.
 */
void
cr_canvas_zoom_world(CrCanvas *canvas)
{
        double x_scale, y_scale;
        CrItem *item;
        cairo_matrix_t *matrix;


        if (canvas->flags & CR_CANVAS_SCROLL_WORLD)

                cr_canvas_set_viewport(canvas, canvas->scroll_x1,
                                canvas->scroll_y1, canvas->scroll_x2,
                                canvas->scroll_y2);

        else if (canvas->flags & CR_CANVAS_MIN_SCALE_FACTOR) {

                item = CR_ITEM(canvas->root);
                matrix = cr_item_get_matrix(item);

                x_scale = y_scale = 1;

                cairo_matrix_transform_distance(matrix, &x_scale, &y_scale);
                x_scale = canvas->min_x_scale_factor / x_scale;
                y_scale = canvas->min_y_scale_factor / x_scale;

                cr_canvas_zoom(canvas, x_scale, y_scale);
        }
        else
                cr_canvas_zoom(canvas, 1/canvas->scroll_factor_x, 
                                1/canvas->scroll_factor_y);
}

/**
 * cr_canvas_set_max_scale_factor:
 * @canvas:
 * @x_factor: Defines the pixel per world coordinate factor.
 * @y_factor: Defines the pixel per world coordinate factor. Ignored if
 *              maintain_aspect is true.
 * 
 * Defines the closest zoom-in scale allowed.  This is used to prevent the
 * underlying rendering routines from doing crazy things when a user presses a
 * zoom-in button too many times.  Example: For a 100x100 pixel
 * window, values of 4,4 would allow the user to zoom in to show no less than
 * 25x25 world coordinates.
 */
void
cr_canvas_set_max_scale_factor(CrCanvas *canvas, double x_factor, 
                double y_factor)
{
        if (canvas->flags & CR_CANVAS_MAINTAIN_ASPECT &&
                        x_factor != y_factor)
                g_warning("Setting unequal max scale factor for"
                                "maintain aspect canvas.");

        canvas->flags |= CR_CANVAS_MAX_SCALE_FACTOR;
        canvas->max_x_scale_factor = x_factor;
        canvas->max_y_scale_factor = y_factor;
}

/**
 * cr_canvas_set_min_scale_factor:
 * @canvas:
 * @x_factor: Defines the pixel per world coordinate factor.
 * @y_factor: Defines the pixel per world coordinate factor. Ignored if
 *              maintain_aspect is true.
 *
 * Defines the farthest zoom-out scale allowed.  Example: For a 100x100 pixel
 * window, values of 0.25, 0.25 would allow the user to zoom out to show no more
 * than 400x400 world coordinates.  In infinite world mode, this routine can be
 * used to limit the maximum amount of the world view that can be rendered to
 * There is no need to ever call this function if you use the finite world model
 * by calling set_scroll_region.  It will automatically be called whenever the
 * canvas is resized.
 */
void
cr_canvas_set_min_scale_factor(CrCanvas *canvas, double x_factor, 
                double y_factor)
{
        if (canvas->flags & CR_CANVAS_MAINTAIN_ASPECT &&
                        x_factor != y_factor)
                g_warning("Setting unequal min scale factor for "
                                "maintain aspect canvas.");

        canvas->flags |= CR_CANVAS_MIN_SCALE_FACTOR;
        canvas->min_x_scale_factor = x_factor;
        canvas->min_y_scale_factor = y_factor;
}

/**
 *cr_canvas_set_repaint_mode:
 * @canvas:
 * @on: TRUE use repaint mode, FALSE to use default invalidate mode.
 *
 * Changes the canvas redraw model from invalidate to repaint.  In the repaint
 * model, the canvas redraws all items regardless of how many requested to be
 * redrawn.  This is useful when implementing scaling tools which would require
 * all of the items to be redrawn for each update.  It reduces the processing
 * load slightly by preventing the canvas from running its invalidate loop. It
 * may also be useful for implemenations that wish to implement a periodic
 * screen refresh.
 */
void
cr_canvas_set_repaint_mode(CrCanvas *canvas, gboolean on)
{
        if (on)
                canvas->flags |= CR_CANVAS_REPAINT_MODE;
        else {
                canvas->flags &= ~CR_CANVAS_REPAINT_MODE;
        }
}

/**
 * cr_canvas_queue_repaint:
 * @canvas:
 *
 * Queues all items in the canvas to be repainted. This avoids running the
 * canvas update loop until after the redraw is complete.  This is useful when
 * you need to continuously repaint for several cycles and want to avoid the
 * cost of the canvas update loop until the repaint cycles are complete.
 * Calling the routine is the same setting canvas repaint mode on and then
 * switching it off at the first G_PRIORITY_LOW idle event.
 */
void
cr_canvas_queue_repaint(CrCanvas *canvas)
{
        if (!(canvas->flags & CR_CANVAS_REPAINT_MODE)) {
                canvas->flags |= (CR_CANVAS_REPAINT_MODE |
                                        CR_CANVAS_REPAINT_REVERT);
        }

        if (canvas->update_idle_id)
                g_source_remove(canvas->update_idle_id);

        canvas->update_idle_id = g_idle_add_full(
                        GDK_PRIORITY_REDRAW - 20,
                        (GSourceFunc) on_repaint_revert_idle,
                        canvas, NULL);

        gtk_widget_queue_draw(GTK_WIDGET(canvas));
}

/**
 * cr_canvas_new:
 * @first_arg_name: A list of object argument name/value pairs, NULL-terminated
 * used to set the canvas widget properties.
 * @varargs:
 *
 * A convenience routine to create the canvas widget and set its properties in
 * one call.
 *
 * Returns: The newly created canvas widget cast to a GtkWidget type.
 */
GtkWidget *
cr_canvas_new(const gchar *first_arg_name, ...)
{
        GtkWidget *canvas;
        va_list args;

        va_start (args, first_arg_name);
        canvas = GTK_WIDGET(g_object_new_valist(CR_TYPE_CANVAS, first_arg_name,
                                args));
        va_end (args);

        return canvas;
}

