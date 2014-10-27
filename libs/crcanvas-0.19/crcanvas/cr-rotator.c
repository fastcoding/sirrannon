/* cr-rotator.c
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
#include "cr-rotator.h"

/**
 * SECTION:cr-rotator
 * @title: CrRotator
 * @short_description: A object for setting up mouse rotation on a #CrCanvas
 * widget.
 *
 * 
 */

static GObjectClass *parent_class = NULL;

enum {
        ARG_0,
        PROP_ACTIVE,
        PROP_CURSOR,
        PROP_CANVAS
};

static void
rotate_canvas(CrRotator *rotator, double newangle)
{
        CrItem *item;
        double da, cx, cy;
        cairo_matrix_t *matrix;

        item = CR_ITEM(rotator->canvas->root);
        matrix = cr_item_get_matrix(item);

        cr_canvas_get_center(rotator->canvas, &cx, &cy);
        cairo_matrix_translate(matrix, +cx, +cy);

        da = newangle - rotator->last_angle;
        rotator->last_angle = newangle;
        
        cairo_matrix_rotate(matrix, da);
        cairo_matrix_translate(matrix, -cx, -cy);

        cr_item_request_update(item);
}

static gboolean
on_canvas_event(CrRotator *rotator, GdkEvent *event, CrCanvas *canvas)
{
        gboolean state;
        double cx, cy, angle;

        if (!(rotator->flags & CR_ROTATOR_ACTIVE)) return FALSE;

        state = FALSE;

        cx = GTK_WIDGET(canvas)->allocation.width /2.;
        cy = GTK_WIDGET(canvas)->allocation.height /2.;

        if (event->type == GDK_BUTTON_PRESS) {
                rotator->last_angle = atan2(event->button.y - cy, 
                                event->button.x - cx);
                rotator->flags |= CR_ROTATOR_DRAGGING;
                rotator->last_msec = event->button.time;
                state = TRUE;
        }
        else if (event->type == GDK_MOTION_NOTIFY) {
                if ((rotator->flags & CR_ROTATOR_DRAGGING) &&
                        event->button.time - rotator->last_msec >= 100) {
                        angle = atan2(event->motion.y - cy, event->motion.x -
                                        cx);

                        rotate_canvas(rotator, angle);
                        rotator->last_msec = event->button.time;
                        state = TRUE;
                }
        }
        else if (event->type == GDK_BUTTON_RELEASE) {
                cr_rotator_deactivate(rotator);
                rotator->flags &= ~CR_ROTATOR_DRAGGING;
                state = TRUE;
        }
        return state;
}

static void
set_canvas(CrRotator *rotator, GObject *object)
{
        g_return_if_fail (CR_IS_CANVAS(object));

        if (rotator->canvas) {
                g_signal_handlers_disconnect_by_func(rotator->canvas,
                                (GCallback) on_canvas_event, rotator);
                g_object_unref(rotator->canvas);
        }
        rotator->canvas = CR_CANVAS(object);
        g_object_ref(rotator->canvas);

        g_signal_connect_swapped(rotator->canvas, "event", (GCallback)
                        on_canvas_event, rotator);
}

static void
cr_rotator_dispose(GObject *object)
{
        CrRotator *rotator;

        rotator = CR_ROTATOR(object);

        if (rotator->canvas)
                g_object_unref(rotator->canvas);

        parent_class->dispose(object);
}

static void
cr_rotator_finalize(GObject *object)
{
        parent_class->finalize(object);
}

static void
cr_rotator_init(CrRotator *rotator)
{
        rotator->cursor = GDK_EXCHANGE;
}

static void
cr_rotator_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrRotator *rotator = CR_ROTATOR(object);
        gpointer object_value;

        switch (property_id) {
                case PROP_ACTIVE:
                        g_value_set_boolean(value, rotator->flags &
                                        CR_ROTATOR_ACTIVE);
                        break;
                case PROP_CANVAS:
                        g_value_set_object(value, rotator->canvas);
                        break;
                case PROP_CURSOR:
                        g_value_set_int(value, rotator->cursor);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_rotator_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        CrRotator *rotator = CR_ROTATOR(object);
        gboolean bval;

        switch (property_id) {
                case PROP_ACTIVE:
                        bval = g_value_get_boolean(value);
                        if (bval && !(rotator->flags & CR_ROTATOR_ACTIVE))
                                cr_rotator_activate(rotator);
                        else if (!bval && (rotator->flags & CR_ROTATOR_ACTIVE))
                                cr_rotator_deactivate(rotator);
                        break;
                case PROP_CANVAS:
                        set_canvas(rotator, g_value_get_object (value));
                        break;
                case PROP_CURSOR:
                        rotator->cursor = g_value_get_int(value);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_rotator_class_init(CrRotatorClass *klass)
{
        GObjectClass *object_class;

        object_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->dispose = cr_rotator_dispose;
        object_class->finalize = cr_rotator_finalize;
        object_class->get_property = cr_rotator_get_property;
        object_class->set_property = cr_rotator_set_property;

        g_object_class_install_property (object_class, PROP_ACTIVE,
                        g_param_spec_boolean("active",
                                "Active",
                                "Active/Deactivate the rotator object or "
                                "check the activation status.",
                                FALSE,
                                G_PARAM_READWRITE));
        g_object_class_install_property(object_class,
                        PROP_CANVAS,
                        g_param_spec_object("canvas", "CrCanvas",
                                "Reference to CrCanvas widget",
                                CR_TYPE_CANVAS, 
                                G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                        PROP_CURSOR,
                        g_param_spec_int("cursor",
                                "Rotator Cursor",
                                "GDK Cursor to use when rotator is selected",
                                0, GDK_LAST_CURSOR, GDK_EXCHANGE,
                                G_PARAM_READWRITE));

}

GType
cr_rotator_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrRotatorClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_rotator_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrRotator),
                0,
                (GInstanceInitFunc) cr_rotator_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(G_TYPE_OBJECT,
                        "CrRotator", &info, 0);
        }
        return type;
}

/**
 * cr_rotator_new:
 * @canvas: The canvas device that this panner will be used with.
 * @first_arg_name: A list of object argument name/value pairs, NULL-terminated,
 * used to configure the item.
 * @varargs:
 *
 * A factory method to create a new CrRotator and connect it to a canvas in one
 * step.
 *
 * Returns: The newly created CrRotator object.  
 *
 * Unlike with the constructors for CrItem implementations, you own the returned
 * reference.  You should call g_object_unref when you are finished with this
 * object.
 */
CrRotator *
cr_rotator_new(CrCanvas *canvas, const gchar *first_arg_name, ...)
{
        CrRotator *rotator;
        va_list args;

        g_return_val_if_fail (CR_IS_CANVAS(canvas), NULL);

        rotator = g_object_new(CR_TYPE_ROTATOR, NULL);
        set_canvas(rotator, G_OBJECT(canvas));

        va_start (args, first_arg_name);
        g_object_set_valist(G_OBJECT(rotator), first_arg_name, args);
        va_end (args);

        return rotator;
}

void 
cr_rotator_activate(CrRotator *rotator)
{
        GdkWindow *window;
        GdkCursor *cursor;
        gboolean flag;

        g_return_if_fail(rotator->canvas != NULL);

        window = GTK_WIDGET(rotator->canvas)->window;
        cursor = gdk_cursor_new(rotator->cursor);
        gdk_window_set_cursor(window, cursor);
        gdk_cursor_unref(cursor);
        rotator->flags |= CR_ROTATOR_ACTIVE;

        /* stopping the canvas from running the test during motion
         * improves performance*/
        g_object_get(rotator->canvas->root, "avoid-test", &flag, NULL);
        if (!flag) {
                g_object_set(rotator->canvas->root, "avoid-test", TRUE, NULL);
                rotator->flags |= CR_ROTATOR_ROOT_AVOID_TEST;
        }
}

void 
cr_rotator_deactivate(CrRotator *rotator)
{
        GdkWindow *window;

        window = GTK_WIDGET(rotator->canvas)->window;
        gdk_window_set_cursor(window, NULL);
        rotator->flags &= ~CR_ROTATOR_ACTIVE;

        if (rotator->flags & CR_ROTATOR_ROOT_AVOID_TEST) {
                g_object_set(rotator->canvas->root, "avoid-test", FALSE, NULL);
                rotator->flags &= ~CR_ROTATOR_ROOT_AVOID_TEST;
        }
}

