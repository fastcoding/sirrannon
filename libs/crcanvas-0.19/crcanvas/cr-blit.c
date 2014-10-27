#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cr-blit.h"

/**
 * SECTION:cr-blit
 * @title: CrBlit
 * @short_description: A grouping item to improve performance by caching
 * complex item trees off-screen.

 * The Blit item is used to group any arbitrary item tree into an offscreen
 * memory surface.  The memory surface is then painted to the screen in a single
 * operation. It should be used in cases where the CPU time needed to paint the
 * item tree to the screen is greater than then frequency at which the tree
 * needs to be repainted. 
 *
 * To use the item, either set a fixed memory size by setting the
 * #CrBlit:device_width and #CrBlit:device_height properties explicitly,
 * or allow it to automatically determine the best memory size by setting the
 * #CrBlit:canvas and #CrBlit:scale-factor properties.  When using fixed memory
 * sizing, the image will often look blurry.  When using automatic sizing, the
 * image will be re-built each time the canvas is zoomed or panned.
 *
 * One example use is a detailed vector map with a moving animation over it.  
 * As the animation moves rapidly over the map, there is no need to redraw the
 * map vectors for each movement of the animation.  The Blit item can be used to
 * rasterize the map so that only a single image will be repainted with each
 * movement of the animation.
 *
 * Another example use is a very complex item tree that needs to be scaled or
 * panned.  The operation may be sluggish if the amount of time required to
 * render the vector graphics is greater than 0.1 seconds.  Because rendering an
 * image is generally faster, using CrBlit during the scaling or panning
 * operation will make the operation happen smoothly. 
 *
 * (<emphasis>BUGS:</emphasis> On some hardware, this item does not work
 * properly and in some cases causes the window manager to crash.  On other
 * hardware it works perfectly.  Suspect problem is in cairo library -- still
 * investigating.)
 */


/*
 * How it works----
 * The item is blitted if there is any overlapping changed area always.
 *
 * -------------------- 2 transformations required ---------------------------
 *
 *  internal to render child items.  this ensures that user coordinate
 *  rectangle maps to physical size of internal surface. This will be determined
 *  on the fly during paint. so no specific matrix required
 *
 *  external to render surface properly onto device.  This will also be setup on
 *  the fly during invoke paint.
 *
 *  The item matrix will normally be identity, but could be used to apply a
 *  consistent trasnformation if necessary,
 *
 * ------------------- Do This: ----------------------------------------
 *
 * override add,  setup a custom invalidate for the child items.  this custom
 * handler looks at the mask and uses gdk functions to add and subtract to the
 * user rectangle.  It also blocks the signal from going to the parent item.
 * Additionally its NEED_UPDATE flag gets set whenever it gets called.
 * (will need custom remove to disconnect)
 *
 * override calculate bounds.  report the bounds determined by the gdk funcs
 * inside custom invalidate handler or report bounds set by user.
 *
 * On a invoke_paint. Swap the context with a local surf context and apply the
 * internal matrix.  do this only if there is at least one child that has
 * changed else skip calling the children and apply the external matrix.
 *
 * on paint, copy the pattern to the context given the matrix of this item.
 * (normally the matrix for this item is identity)
 *
 */

#define DEBUG 0

static GObjectClass *parent_class = NULL;

enum {
        ARG_0,
        PROP_DEVICE_WIDTH,
        PROP_DEVICE_HEIGHT,
        PROP_TEST_IMAGE,
        PROP_CANVAS,
        PROP_SCALE_FACTOR
};

static gboolean 
on_timeout(CrBlit *blit)
{
        if (blit->pattern) {
                cairo_pattern_destroy(blit->pattern);
                blit->pattern = NULL;
        }
        cr_item_request_update(CR_ITEM(blit));
        return FALSE;
}

static void
on_before_paint(CrBlit *blit, CrContext *c, gboolean viewport_changed)
{
        gboolean zoom_changed, update_needed;


        /* This prevents a double update.  If we get here as a result of the
         * blit item being updated, there is no reason to queue up another
         * update. */
        //if (blit->flags & CR_BLIT_UPDATE_READY) {
        if (0){
                blit->flags &= ~CR_BLIT_UPDATE_READY;
                update_needed = FALSE;
                if (blit->timer_id)
                        g_source_remove(blit->timer_id);
                blit->timer_id = 0;
        }
        else update_needed = TRUE;


        zoom_changed = (blit->canvas->flags & 
                        (CR_CANVAS_MAINTAIN_CENTER | CR_CANVAS_AUTO_SCALE)) &&
                blit->display_width != 
                        GTK_WIDGET(blit->canvas)->allocation.width &&
                blit->display_height != 
                        GTK_WIDGET(blit->canvas)->allocation.height;

        if (!zoom_changed && !viewport_changed) return;
                        
        blit->display_width = GTK_WIDGET(blit->canvas)->allocation.width;
        blit->display_height = GTK_WIDGET(blit->canvas)->allocation.height;

        if (blit->timer_id) {
                g_source_remove(blit->timer_id);
                blit->timer_id = 0;
        }

        if (update_needed)
                blit->timer_id = g_timeout_add(1000, (GSourceFunc) 
                                on_timeout, blit);
}

static void
set_canvas(CrBlit *blit, GObject *object)
{
        g_return_if_fail (!object || CR_IS_CANVAS(object));

        if (blit->timer_id) {
                g_source_remove(blit->timer_id);
                blit->timer_id = 0;
        }

        if (blit->canvas) {
                g_signal_handlers_disconnect_by_func(blit->canvas, 
                                (GCallback) on_before_paint, blit);
                g_object_unref(blit->canvas);
                blit->canvas = NULL;
        }

        if (object) {
                blit->canvas = CR_CANVAS(object);
                g_object_ref(blit->canvas);
                g_signal_connect_swapped(blit->canvas, "before-paint",
                                (GCallback) on_before_paint, blit);
                on_timeout(blit);
        }
}

static void
cr_blit_dispose(GObject *object)
{
        parent_class->dispose(object);
        set_canvas(CR_BLIT(object), NULL);
}

static void
cr_blit_finalize(GObject *object)
{
        CrBlit *blit;

        blit = CR_BLIT(object);

        if (blit->region)
                gdk_region_destroy(blit->region);
        if (blit->pattern)
                cairo_pattern_destroy(blit->pattern);
        if (blit->buffer)
                g_free(blit->buffer);

        parent_class->finalize(object);
}

static void
cr_blit_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        CrBlit *blit = (CrBlit*) object;
        CrItem *item = CR_ITEM(object);
        double val;
        gboolean flag;

        switch (property_id) {
                case PROP_DEVICE_WIDTH:
                        val = g_value_get_double(value);
                        if (val != blit->device_width) {
                                blit->device_width = val;
                                cr_item_request_update(item);
                        }
                        break;
                case PROP_DEVICE_HEIGHT:
                        val = g_value_get_double(value);
                        if (val != blit->device_height) {
                                blit->device_height = val;
                                cr_item_request_update(item);
                        }
                        break;
                case PROP_TEST_IMAGE:
                        flag = g_value_get_boolean(value);
                        if (flag)
                                blit->flags |= CR_BLIT_TEST_IMAGE;
                        else
                                blit->flags &= ~CR_BLIT_TEST_IMAGE;
                        break;
                case PROP_SCALE_FACTOR:
                        val = g_value_get_double(value);
                        if (val != blit->scale_factor) {
                                blit->scale_factor = val;
                                cr_item_request_update(item);
                        }
                        break;
                case PROP_CANVAS:
                        set_canvas(blit, g_value_get_object (value));
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_blit_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrBlit *blit = (CrBlit*) object;
        switch (property_id) {
                case PROP_DEVICE_WIDTH:
                        g_value_set_double (value, blit->device_width);
                        break;
                case PROP_DEVICE_HEIGHT:
                        g_value_set_double (value, blit->device_height);
                        break;
                case PROP_SCALE_FACTOR:
                        g_value_set_double (value, blit->scale_factor);
                        break;
                case PROP_TEST_IMAGE:
                        g_value_set_boolean(value, blit->flags &
                                        CR_BLIT_TEST_IMAGE);
                        break;
                case PROP_CANVAS:
                        g_value_set_object(value, blit->canvas);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_blit_init(CrBlit *blit)
{
        blit->flags |= CR_BLIT_TEST_IMAGE;
        blit->scale_factor = 2;
}

static inline void
remove_rotation(cairo_matrix_t *matrix,
                double *x1, double *y1, double *x2, double *y2)
{
        double ax1, ay1, ax2, ay2;
        double bx1, by1, bx2, by2;

        /* This factors-out any rotations by determining the
         * largest enclosing rectangle afterwards. */

        ax1 = bx1 = *x1; ay1 = by1 = *y1; 
        ax2 = bx2 = *x2; ay2 = by2 = *y2;

        cairo_matrix_transform_point(matrix, &ax1, &ay1);
        cairo_matrix_transform_point(matrix, &bx2, &by1);
        cairo_matrix_transform_point(matrix, &ax2, &ay2);
        cairo_matrix_transform_point(matrix, &bx1, &by2);
        *x1 = MIN(MIN(MIN(ax1, ax2), bx1), bx2);
        *y1 = MIN(MIN(MIN(ay1, ay2), by1), by2);
        *x2 = MAX(MAX(MAX(ax1, ax2), bx1), bx2);
        *y2 = MAX(MAX(MAX(ay1, ay2), by1), by2);
}

static void
invalidated(CrItem *item, int mask,
                double x1, double y1, double x2, double y2, 
                CrDeviceBounds *device)
{
        CrBlit *blit;
        GdkRectangle rect;
        GdkRegion *region;

        /* INVALIDATE_OLD is the reporting of old bounds. */
        /* INVALIDATE_NEW is the reporting of new bounds. */
        /* IN_REPORT_BOUNDS this parent is waiting for an invalidate. */
        /* INVALIDATE_UPDATE is reporting because some child has changed. */

        if (item->flags & CR_ITEM_NEED_UPDATE)
                mask |= CR_ITEM_INVALIDATE_UPDATE;

        /* The signal stops if it got here as a result of a request from a
         * different parent and no item lower on the tree has changed. */
        if (!(mask & CR_ITEM_INVALIDATE_UPDATE) && 
                        !(item->flags & CR_ITEM_IN_REPORT_BOUNDS)) return;

        blit = CR_BLIT(item);

        rect.x = x1;
        rect.y = y1;
        rect.width = x2 - x1;
        rect.height = y2 - y1;

        if (blit->region && (mask & CR_ITEM_INVALIDATE_OLD)) {
                /* subtract from the user region */
#if DEBUG
                g_print("sub\n");
#endif
                region = gdk_region_rectangle(&rect);
                gdk_region_subtract(blit->region, region);
                gdk_region_destroy(region);
                item->flags |= CR_ITEM_NEED_UPDATE;
                blit->flags |= CR_BLIT_REBUILD_PATTERN;
                if (gdk_region_empty(blit->region)) {
                        gdk_region_destroy(blit->region);
                        blit->region = NULL;
                }
        }
        else if (mask & CR_ITEM_INVALIDATE_NEW) {
                /* add to the user region */
#if DEBUG
                g_print("add %d,%d (%dx%d)\n", rect.x, rect.y, 
                                rect.width,rect.height);
#endif
                if (!blit->region)
                        blit->region = gdk_region_rectangle (&rect);
                else
                        gdk_region_union_with_rect(blit->region, &rect);
                item->flags |= CR_ITEM_NEED_UPDATE;
                blit->flags |= CR_BLIT_REBUILD_PATTERN;
        }

        /* invalidate signal is stopped here and prevented from moving up.
         * This item will eventually send an invalidate on the composite 
         * rectangle.
         * */
}

static void 
report_old_bounds(CrItem *item, cairo_t *ct, gboolean all)
{
        cairo_matrix_t *matrix;
        double x1, y1, x2, y2;
        int mask;
        gboolean bounds_sent;

        /* All gets sent as a result of a transformation change in some item
         * above.  This can be ignored if I optimize both report-old and
         * report-new. */

        bounds_sent = (item->flags & CR_ITEM_NEED_UPDATE);

        CR_ITEM_CLASS(parent_class)->report_old_bounds(item, ct, FALSE);

        /* NEED_UPDATE will be set if a child item sends an invalidate.
         * The below must be repeated from CrItem since it does not get done 
         * unless NEED UPDATE was set before hand. */

        if (!bounds_sent && item->bounds &&
                        ((item->flags & CR_ITEM_NEED_UPDATE) || all)) {

                item->flags |= CR_ITEM_IN_REPORT_BOUNDS;

                mask = CR_ITEM_INVALIDATE_OLD | CR_ITEM_INVALIDATE_UPDATE;

                x1 = item->bounds->x1;
                y1 = item->bounds->y1;
                x2 = item->bounds->x2;
                y2 = item->bounds->y2;

                if (item->matrix_p)
                        remove_rotation(item->matrix_p, &x1, &y1, &x2, &y2);

                g_signal_emit_by_name(item, "invalidate", mask, x1, y1, x2,
                                y2, item->device);

                item->flags &= ~CR_ITEM_IN_REPORT_BOUNDS;
        }
}

static void
report_new_bounds(CrItem *item, cairo_t *ct, gboolean all)
{
        GList *list, *items;
        int mask;
        CrBlit *blit;

        blit = CR_BLIT(item);

        /* This is the same as CrItem but is a special handler for the child
         * items where all is always false.
         *
         * This item gets handled by the parent implementation and the child
         * items get handled here.
         */

        if (!(item->flags & CR_ITEM_VISIBLE)) {
                item->flags &= ~CR_ITEM_NEED_UPDATE;
                return;
        }

        cairo_save(ct);

        if (item->matrix) {
                cairo_transform(ct, item->matrix);
                /* save this for later to report the old bounds. */
                *item->matrix_p = *item->matrix;
        }

        item->flags |= CR_ITEM_IN_REPORT_BOUNDS;

        for (list = item->items; list; list = list->next) {
                /* If a group needs to be updated, then all children also
                 * will need to re-report bounds */

                /* If the pattern does not yet exist, force them to report 
                 * bounds. Otherwise never cause them to report only if they
                 * have changed,*/

                cr_item_report_new_bounds(CR_ITEM(list->data), ct,
                               blit->pattern == NULL); 
        }

        item->flags &= ~CR_ITEM_IN_REPORT_BOUNDS;

        cairo_restore(ct);

        if (item->flags & CR_ITEM_NEED_UPDATE)
                /* tells the before-paint handler not to queue another refresh.
                 * */
                blit->flags |= CR_BLIT_UPDATE_READY;

        /* now the children are temporarily set to NULL so the parent
         * implementation can be called for this item.
         */
        items = item->items;
        item->items = NULL;
        CR_ITEM_CLASS(parent_class)->report_new_bounds(item, ct, all);
        item->items = items;
}

static void
add(CrItem *item, CrItem *child)
{
        gulong signal_id, handler_id;

        CR_ITEM_CLASS(parent_class)->add(item, child);

        signal_id = g_signal_lookup("invalidate", CR_TYPE_ITEM);

        handler_id = g_signal_handler_find(child, 
                        (G_SIGNAL_MATCH_DATA | G_SIGNAL_MATCH_ID),
                        signal_id, 0, NULL, NULL, item);

        g_signal_handler_disconnect (child, handler_id);


        /* The invalidate signal from the children is captured so that the user
         * space rectangle for the blit region may be determined and reported 
         * up the tree*/
        g_signal_connect_swapped(child, "invalidate", (GCallback) invalidated,
                        item);
}

static void
_remove(CrItem *item, CrItem *child)
{
        CrItem *i;
        GList *list, *link;

        link = g_list_find(item->items, child);

        g_object_ref(child);
        CR_ITEM_CLASS(parent_class)->remove(item, child);

        /* The invalidate from the child will be blocked because this item
         * overrides the report_old_bounds method, so it is
         * necessary for the blit to explicitly request an update. */

        if (link) {
                cr_item_request_update(item);
                g_signal_handlers_disconnect_by_func(child,
                                (GCallback)invalidated, item);
        }
        g_object_unref(child);
}

static gboolean 
calculate_bounds(CrItem *item, cairo_t *c, CrBounds *bounds,
                CrDeviceBounds *device)
{
        CrBlit *blit;
        GdkRectangle rect;
        double x, y, w, h;
        cairo_matrix_t matrix, matrix_i;

        blit = CR_BLIT(item);
#if DEBUG
        g_print("calc bounds\n");
#endif

        if (!blit->region) return FALSE;

        gdk_region_get_clipbox(blit->region, &rect);

        if (blit->canvas) {
                /* figures out the appropriate device-width, height, and x and y
                 * offset to use for the given canvas scaling and viewport.
                 */ 

                /* do this on unrotated coordinates. */
                cairo_get_matrix(c, &matrix);
                cairo_matrix_rotate(&matrix, -atan2(matrix.yx, matrix.yy));
                matrix_i = matrix;
                cairo_matrix_invert(&matrix_i);

                /* (1) the viewport in this item's user space */
                w = GTK_WIDGET(blit->canvas)->allocation.width;
                h = GTK_WIDGET(blit->canvas)->allocation.height;
                x = y = 0;
                cairo_matrix_transform_distance(&matrix_i, &w, &h);
                cairo_matrix_transform_point(&matrix_i, &x, &y);

                /* (2) adjust the viewport by the scale factor */
                x -= w * (blit->scale_factor - 1) / 2;
                y -= h * (blit->scale_factor - 1) / 2;
                w *= blit->scale_factor;
                h *= blit->scale_factor;

                /* (3) do intersection */
                if (x + w < rect.x || x > rect.x + rect.width ||
                                y + h < rect.y || y > rect.y + rect.height)
                        return FALSE;

                rect.x = MAX(x, rect.x);
                rect.y = MAX(y, rect.y);
                blit->device_width = MIN(x + w, rect.x + rect.width) - rect.x;
                blit->device_height = MIN(y + h, rect.y + rect.height) - rect.y;
                rect.width = blit->device_width;
                rect.height = blit->device_height;

                cairo_matrix_transform_distance(&matrix, &blit->device_width,
                                &blit->device_height);

#if DEBUG
                g_print("rect is %d,%d %dx%d\n", rect.x, rect.y, 
                                rect.width, rect.height);
                g_print("device is %gx%g\n", blit->device_width, 
                                blit->device_height);
#endif
        }

        bounds->x1 = rect.x;
        bounds->y1 = rect.y;
        bounds->x2 = rect.x + rect.width;
        bounds->y2 = rect.y + rect.height;
#if DEBUG
        g_print("%g %g %g %g\n", bounds->x1, bounds->y1 , 
                        bounds->x2 , bounds->y2);
#endif
        return TRUE;
}

static void 
create_pattern(CrBlit *blit)
{
        CrItem *item, *i;
        GList *list;
        cairo_surface_t *surface;
        cairo_t *c;

        item = CR_ITEM(blit);

        if (blit->pattern) {
                cairo_pattern_destroy(blit->pattern);
                blit->pattern = NULL;
        }

        if (blit->device_width == 0 || blit->device_height == 0) {
                if (!blit->canvas)
                        g_warning("CrBlit device_width and height must be set,"
                                " or the 'canvas' property should be set.");
                return;
        }
        if (blit->buffer) g_free(blit->buffer);

        blit->buffer = g_new0(unsigned char, (int) (
                        blit->device_height * blit->device_width * 4));

        surface = cairo_image_surface_create_for_data (blit->buffer, 
                        CAIRO_FORMAT_ARGB32, (int) blit->device_width,
                        (int) blit->device_height,
                        (int) blit->device_width * 4);

        c = cairo_create(surface);

        cairo_scale(c, blit->device_width/(item->bounds->x2 - item->bounds->x1),
                blit->device_height/(item->bounds->y2 - item->bounds->y1));

        cairo_translate(c, -item->bounds->x1, -item->bounds->y1);

        for (list = item->items; list; list = list->next) {
                i = list->data;
                cr_item_invoke_paint(i, c, TRUE, item->bounds->x1, 
                                item->bounds->y1, item->bounds->x2, 
                                item->bounds->y2);
        }


        blit->pattern = cairo_pattern_create_for_surface(surface);
        cairo_pattern_set_filter(blit->pattern, CAIRO_FILTER_FAST);
        cairo_surface_destroy(surface);


        cairo_destroy(c);

#if DEBUG
        g_print("create surf\n");
#endif



}

static inline gboolean
test_inside(double x1, double y1, double x2, double y2,
                double bx1, double by1, double bx2, double by2)
{
        /* If the rectangle intersection is not a rectangle, then it is
         * outside. */
        if (MAX(bx1, x1) > MIN(bx2, x2)) return FALSE;
        if (MAX(by1, y1) > MIN(by2, y2)) return FALSE;

        return TRUE;
}

static void
invoke_paint(CrItem *item, cairo_t *ct, gboolean all,
                double x1, double y1, double x2, double y2)
{
        cairo_matrix_t *imatrix;
        CrBlit *blit;

        blit = CR_BLIT(item);

        if (!(item->flags & CR_ITEM_VISIBLE)) {
                item->flags &= ~CR_ITEM_NEED_UPDATE;
                if (blit->pattern) {
                        cairo_pattern_destroy(blit->pattern);
                        blit->pattern = NULL;
                }
                return;
        }

        if (item->flags & CR_ITEM_NEED_UPDATE) {
                (*CR_ITEM_GET_CLASS(item)->invoke_calculate_bounds)(item, ct);

                item->flags &= ~CR_ITEM_NEED_UPDATE;

                /* tells the before-paint handler not to queue another 
                 * refresh. */
                blit->flags |= CR_BLIT_UPDATE_READY;
        }

        if (!item->bounds) return;

        if (item->matrix) {
                imatrix = cr_item_get_inverse_matrix(item);
                remove_rotation(imatrix, &x1, &y1, &x2, &y2);
        }

        cairo_save(ct);

        if (item->matrix) cairo_transform(ct, item->matrix);

        if (blit->flags & CR_BLIT_REBUILD_PATTERN) {
                create_pattern(blit);
                blit->flags &= ~CR_BLIT_REBUILD_PATTERN;
        }

        if (all || test_inside(x1, y1, x2, y2, item->bounds->x1,
                                item->bounds->y1, item->bounds->x2,
                                item->bounds->y2)) {

                g_signal_emit_by_name(item, "paint", ct);
        }

        cairo_restore(ct);
}

static void
paint(CrItem *item, cairo_t *c)
{
        CrBlit *blit;

        blit = CR_BLIT(item);

        if (!blit->pattern) return; 

        double x, y, sx, sy;

        sx = (item->bounds->x2 - item->bounds->x1)/blit->device_width;
        sy = (item->bounds->y2 - item->bounds->y1)/blit->device_height;
        cairo_translate(c, item->bounds->x1, item->bounds->y1);
        cairo_scale(c, sx, sy);
        cairo_set_source(c, blit->pattern);
        cairo_paint(c);


        /*
        cairo_new_path(c);
        cairo_rectangle(c, item->bounds->x1, item->bounds->y1,
                        item->bounds->x2 - item->bounds->x1,
                        item->bounds->y2 - item->bounds->y1);

        cairo_set_source_rgb(c, 255, 0, 0);
        cairo_stroke(c);
        */
}

static CrItem *
invoke_test(CrItem *item, cairo_t *ct, double x, double y)
{
        CrBlit *blit;
        CrItem *result;
        GList *items;

        blit = CR_BLIT(item);
        items = item->items;

        if (blit->flags & CR_BLIT_TEST_IMAGE)
                item->items = NULL;

        result = CR_ITEM_CLASS(parent_class)->invoke_test(item, ct, x, y);

        item->items = items;
        return result;
}

static CrItem *
test(CrItem *item, cairo_t *ct, double x, double y)
{
        double px, py;
	guchar *src;
        CrBlit *blit;

        blit = CR_BLIT(item);

        if (!item->bounds || !blit->buffer) return NULL;

        px = (x - item->bounds->x1) * blit->device_width / 
                (item->bounds->x2 - item->bounds->x1);
        py = (y - item->bounds->y1) * blit->device_width / 
                (item->bounds->y2 - item->bounds->y1);

        src = &blit->buffer[(((int) py) * ((int) blit->device_width) +
                        ((int) px)) * 4];

	if (src[3] < 128)
                return NULL;

        return item;
}

static void
cr_blit_class_init(CrBlitClass *klass)
{
        GObjectClass *object_class;
        CrItemClass *item_class;

        object_class = (GObjectClass *) klass;
        item_class = (CrItemClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->get_property = cr_blit_get_property;
        object_class->set_property = cr_blit_set_property;
        object_class->dispose = cr_blit_dispose;
        object_class->finalize = cr_blit_finalize;
        item_class->report_old_bounds = report_old_bounds;
        item_class->report_new_bounds = report_new_bounds;
        item_class->add = add;
        item_class->remove = _remove;
        item_class->invoke_paint = invoke_paint;
        item_class->calculate_bounds = calculate_bounds;
        item_class->paint = paint;
        item_class->invoke_test = invoke_test;
        item_class->test = test;

        g_object_class_install_property
                (object_class,
                PROP_DEVICE_WIDTH,
                g_param_spec_double ("device-width", "Device Width",
                "The pixel width dedicated to the blit surface.  This "
                "property will get set automatically if the canvas property "
                "is set.",
                0, G_MAXDOUBLE, 0,
                G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                PROP_DEVICE_HEIGHT,
                g_param_spec_double ("device-height", "Device Height",
                "The pixel height dedicated to the blit surface. This "
                "property will get set automatically if the canvas property "
                "is set.",
                0, G_MAXDOUBLE, 0,
                G_PARAM_READWRITE));
        g_object_class_install_property (object_class, PROP_TEST_IMAGE,
                g_param_spec_boolean("test-image", "Test Image",
                "If only the image should be tested rather than the "
                "individual child items.  For very complex item trees, this "
                "property will save cpu usage.  Set this to FALSE if you need "
                "to distinguish which part of the image is selected.",
                TRUE,
                G_PARAM_READWRITE));
        g_object_class_install_property(object_class,
                PROP_CANVAS,
                g_param_spec_object("canvas", "CrCanvas",
                "Reference to CrCanvas widget which can be used in lieu of "
                "setting device-width/height explicitly.  Setting the canvas "
                "property will cause the blitted image to get periodically "
                "recalculated as the scale and size of the referenced canvas "
                "changes. In the event that the blitted image is added to the "
                "item tree of multiple canvasses, the recalulation will "
                "obviously only be triggered by changes to the canvas "
                "referenced here.",
                CR_TYPE_CANVAS, 
                G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                PROP_SCALE_FACTOR,
                g_param_spec_double ("scale-factor", "Scale Factor",
                "A ratio that defines the maximum amount of memory that can "
                "be used to define the blit device-width and height when the "
                "canvas property is set.  i.e., If canvas is zoomed in to show "
                "only 10x10 user coordinates and the scale-factor=2, "
                "but the blitted image is 100x100 in user corrdinates, only "
                "a 20x20 fraction of the image will be stored.  This property "
                "has no-effect when the canvas property is not set.",
                0, G_MAXDOUBLE, 2,
                G_PARAM_READWRITE));
}

GType
cr_blit_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrBlitClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_blit_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrBlit),
                0,
                (GInstanceInitFunc) cr_blit_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(CR_TYPE_ITEM,
                        "CrBlit", &info, 0);
        }
        return type;
}

/**
 * cr_blit_new:
 * @parent: The parent canvas item.
 *
 * A convenience routine for creating a cr-blit item and adding it to an item
 * group in one step.
 *
 * Returns: A reference to a new CrItem.  You must call g_object_ref if you
 * intend to use this reference outside the local scope.
 */
CrItem *
cr_blit_new(CrItem *parent, const gchar *first_arg_name, ...)
{
        CrItem *item;
        va_list args;

        va_start (args, first_arg_name);
        item = cr_item_construct(parent, CR_TYPE_BLIT, first_arg_name, args);
        va_end (args);

        return item;
}


