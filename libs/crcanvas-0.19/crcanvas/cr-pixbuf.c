/* cr-pixbuf.c 
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
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "cr-pixbuf.h"

/**
 * SECTION:cr-pixbuf
 * @title: CrPixbuf
 * @short_description: A item for rendering #GdkPixbuf, png files, or
 * a cairo image surface.
 *
 * This canvas item is capable of rendering a #GdkPixbuf, a png file, or
 * any cairo image surface by setting either of the #CrPixbuf:pixbuf,
 * #CrPixbuf:png, or #CrPixbuf:surface properties.
 */

static GObjectClass *parent_class = NULL;

enum {
        ARG_0,
        PROP_WIDTH,
        PROP_HEIGHT,
        PROP_SCALEABLE,
        PROP_ANCHOR,
        PROP_PIXBUF,
        PROP_PNG,
        PROP_X_OFFSET,
        PROP_Y_OFFSET,
        PROP_PATTERN,
        PROP_SURFACE,
        PROP_TEST_FILL
};

static void
cr_pixbuf_dispose(GObject *object)
{
        CrPixbuf *pixbuf;

        pixbuf = CR_PIXBUF(object);

        if (pixbuf->pixbuf)
                g_object_unref(pixbuf->pixbuf);
        pixbuf->pixbuf = NULL;
        parent_class->dispose(object);
}

static void
cr_pixbuf_finalize(GObject *object)
{
        CrPixbuf *pixbuf;

        pixbuf = CR_PIXBUF(object);

        if (pixbuf->pattern)
                cairo_pattern_destroy(pixbuf->pattern);

        parent_class->finalize(object);
}

static void
png_to_pattern(CrPixbuf *pixbuf, const char *file)
{
        cairo_surface_t *surface;

        if (pixbuf->pixbuf) 
                g_object_unref(pixbuf->pixbuf);
        if (pixbuf->pattern)
                cairo_pattern_destroy(pixbuf->pattern);
        pixbuf->pixbuf = NULL;
        pixbuf->pattern = NULL;
        pixbuf->width = pixbuf->height = 0;

        surface = cairo_image_surface_create_from_png(file);

        if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
                g_warning("Could not load png [%s]\nError was [%s]\n",
                                file, cairo_status_to_string(
                                        cairo_surface_status(surface)));
                cairo_surface_destroy(surface);
                return;
        }
        pixbuf->pattern = cairo_pattern_create_for_surface(surface);
        cairo_pattern_set_filter (pixbuf->pattern, CAIRO_FILTER_FAST);
        pixbuf->width = (double) cairo_image_surface_get_width(surface);
        pixbuf->height = (double) cairo_image_surface_get_height(surface);
        cairo_surface_destroy(surface);
        cr_item_request_update(CR_ITEM(pixbuf));
}

typedef struct {
        char *data;
        gsize len, pos;
} PixbufBuffer;

static cairo_status_t
read_func(PixbufBuffer *buf, unsigned char *data, unsigned int length)
{
        memcpy(data, &buf->data[buf->pos], length);
        buf->pos += length;
        return CAIRO_STATUS_SUCCESS;
}

static void
pixbuf_to_pattern(CrPixbuf *pixbuf, GdkPixbuf *pbuf)
{
        cairo_surface_t *surface;
        PixbufBuffer b;
        GError *error;

        if (pixbuf->pixbuf) {
                g_object_unref(pixbuf->pixbuf);
                pixbuf->pixbuf = NULL;
        }
        if (pixbuf->pattern) {
                cairo_pattern_destroy(pixbuf->pattern);
                pixbuf->pattern = NULL;
        }

        if (!pbuf) {
                cr_item_request_update(CR_ITEM(pixbuf));
                return;
        }

        error = NULL;
        gdk_pixbuf_save_to_buffer(pbuf, &b.data, &b.len, "png", &error, NULL);
        b.pos = 0;

        surface = cairo_image_surface_create_from_png_stream(
                        (cairo_read_func_t) read_func, (void *) &b);
        free (b.data);
        pixbuf->pattern = cairo_pattern_create_for_surface(surface);
        cairo_pattern_set_filter (pixbuf->pattern, CAIRO_FILTER_FAST);
        cairo_surface_destroy(surface);

        pixbuf->width = (double) gdk_pixbuf_get_width(pbuf);
        pixbuf->height = (double) gdk_pixbuf_get_height(pbuf);
        pixbuf->pixbuf = pbuf;
        g_object_ref(pbuf);
        cr_item_request_update(CR_ITEM(pixbuf));
}

static void
cr_pixbuf_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        const char *png;
        GdkPixbuf *pbuf;
        CrPixbuf *pixbuf = (CrPixbuf*) object;
        cairo_surface_t *surface;

        switch (property_id) {
                case PROP_PIXBUF:
                        pbuf = GDK_PIXBUF (g_value_get_object (value));
                        pixbuf_to_pattern(pixbuf, pbuf);
                        break;
                case PROP_PNG:
                        png_to_pattern(pixbuf, g_value_get_string(value));
                        break;
                case PROP_SCALEABLE:
                        if (g_value_get_boolean(value))
                                pixbuf->flags |= CR_PIXBUF_SCALEABLE;
                        else
                                pixbuf->flags &= ~CR_PIXBUF_SCALEABLE;
                        cr_item_request_update(CR_ITEM(pixbuf));
                        break;
                case PROP_ANCHOR:
                        pixbuf->anchor = g_value_get_enum (value);
                        cr_item_request_update(CR_ITEM(pixbuf));
                        break;
                case PROP_X_OFFSET:
                        pixbuf->x_offset = g_value_get_double (value);
                        cr_item_request_update(CR_ITEM(pixbuf));
                        break;
                case PROP_Y_OFFSET:
                        pixbuf->y_offset = g_value_get_double (value);
                        cr_item_request_update(CR_ITEM(pixbuf));
                        break;
                case PROP_SURFACE:
                        if (pixbuf->pixbuf)
                                g_object_unref(pixbuf->pixbuf);
                        pixbuf->pixbuf = NULL;
                        if (pixbuf->pattern) 
                                cairo_pattern_destroy(pixbuf->pattern);

                        surface = g_value_get_boxed(value);

                        if (!surface) {
                                pixbuf->width =  pixbuf->height = 0;
                                pixbuf->pattern = NULL;
                                cr_item_request_update(CR_ITEM(pixbuf));
                                return;
                        }

                        g_return_if_fail(cairo_surface_get_type(surface) ==
                                        CAIRO_SURFACE_TYPE_IMAGE);

                        pixbuf->width = (double) 
                                cairo_image_surface_get_width(surface);
                        pixbuf->height = (double) 
                                cairo_image_surface_get_height(surface);

                        pixbuf->pattern = cairo_pattern_create_for_surface(
                                        surface);
                        cairo_pattern_set_filter (pixbuf->pattern, 
                                        CAIRO_FILTER_FAST);
                        cr_item_request_update(CR_ITEM(pixbuf));
                        break;
                case PROP_TEST_FILL:
                        if (g_value_get_boolean(value))
                                pixbuf->flags |= CR_PIXBUF_TEST_FILL;
                        else
                                pixbuf->flags &= ~CR_PIXBUF_TEST_FILL;
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_pixbuf_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrPixbuf *pixbuf = (CrPixbuf*) object;
        switch (property_id) {
                case PROP_PIXBUF:
                        g_value_set_object(value, pixbuf->pixbuf);
                        break;
                case PROP_WIDTH:
                        g_value_set_double (value, pixbuf->width);
                        break;
                case PROP_HEIGHT:
                        g_value_set_double (value, pixbuf->height);
                        break;
                case PROP_SCALEABLE:
                        g_value_set_boolean(value, 
                                        pixbuf->flags & CR_PIXBUF_SCALEABLE);
                        break;
                case PROP_ANCHOR:
                        g_value_set_enum (value, pixbuf->anchor);
                        break;
                case PROP_X_OFFSET:
                        g_value_set_double (value, pixbuf->x_offset);
                        break;
                case PROP_Y_OFFSET:
                        g_value_set_double (value, pixbuf->y_offset);
                        break;
                case PROP_PATTERN:
                        g_value_set_boxed(value, pixbuf->pattern);
                        break;
                case PROP_TEST_FILL:
                        g_value_set_boolean(value,
                                pixbuf->flags & CR_PIXBUF_TEST_FILL);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
get_device_extents(CrPixbuf *pixbuf, cairo_t *c, CrDeviceBounds *device)
{
        double w, h;

        w = pixbuf->width;
        h = pixbuf->height;

        device->anchor = pixbuf->anchor;

        switch (pixbuf->anchor) {
                case GTK_ANCHOR_N:
                case GTK_ANCHOR_CENTER:
                case GTK_ANCHOR_S:
                        device->x1 = -w/2 + pixbuf->x_offset;
                        device->x2 = w/2 + pixbuf->x_offset;
                        break;
                case GTK_ANCHOR_NE:
                case GTK_ANCHOR_E:
                case GTK_ANCHOR_SE:
                        device->x1 = -w + pixbuf->x_offset;
                        device->x2 = 0 + pixbuf->x_offset;
                        break;
                case GTK_ANCHOR_NW:
                case GTK_ANCHOR_W:
                case GTK_ANCHOR_SW:
                        device->x1 = 0 + pixbuf->x_offset;
                        device->x2 = w + pixbuf->x_offset;
                        break;
        }
        switch (pixbuf->anchor) {
                case GTK_ANCHOR_W:
                case GTK_ANCHOR_CENTER:
                case GTK_ANCHOR_E:
                        device->y1 = -h/2 + pixbuf->y_offset;
                        device->y2 = h/2 + pixbuf->y_offset;
                        break;
                case GTK_ANCHOR_SW:
                case GTK_ANCHOR_S:
                case GTK_ANCHOR_SE:
                        device->y1 = -h + pixbuf->y_offset;
                        device->y2 = 0 + pixbuf->y_offset;
                        break;
                case GTK_ANCHOR_NW:
                case GTK_ANCHOR_N:
                case GTK_ANCHOR_NE:
                        device->y1 = 0 + pixbuf->y_offset;
                        device->y2 = h + pixbuf->y_offset;
                        break;
        }
}

static void
get_item_extents(CrPixbuf *pixbuf, cairo_t *c, double *x1, double *y1, 
                double *x2, double *y2)
{
        double w, h;

        w = pixbuf->width;
        h = pixbuf->height;
        *x1 = *y1 = 0;

        switch (pixbuf->anchor) {
                case GTK_ANCHOR_N:
                case GTK_ANCHOR_CENTER:
                case GTK_ANCHOR_S:
                        *x1 -= w/ 2.0;
                        break;

                case GTK_ANCHOR_NE:
                case GTK_ANCHOR_E:
                case GTK_ANCHOR_SE:
                        *x1 -= w;
                        break;
	}
        switch (pixbuf->anchor) {
                case GTK_ANCHOR_W:
                case GTK_ANCHOR_CENTER:
                case GTK_ANCHOR_E:
                        *y1 -= h/ 2.0;
                        break;
                case GTK_ANCHOR_SW:
                case GTK_ANCHOR_S:
                case GTK_ANCHOR_SE:
                        *y1 -= h;
                        break;
        }
        *x2 = *x1 + w;
        *y2 = *y1 + h;
}

static void
paint(CrItem *item, cairo_t *c)
{
        CrPixbuf *pixbuf;
        cairo_matrix_t matrix;
        double x, y;

        pixbuf = CR_PIXBUF(item);

        if (!pixbuf->pattern) return;

        x = item->bounds->x1;
        y = item->bounds->y1;
        cairo_save(c);

        if (pixbuf->flags & CR_PIXBUF_SCALEABLE) {
                cairo_matrix_init_identity(&matrix);
                cairo_matrix_translate(&matrix, x, y);
                cairo_matrix_invert(&matrix);
        }
        else {
                cairo_user_to_device(c, &x, &y);
                cairo_identity_matrix(c);
                cairo_matrix_init_identity(&matrix);
                cairo_matrix_translate(&matrix, x + item->device->x1, 
                                y + item->device->y1);
                cairo_matrix_invert(&matrix);
        }

        cairo_pattern_set_matrix(pixbuf->pattern, &matrix);
        cairo_set_source (c, pixbuf->pattern);
        cairo_paint(c);
        cairo_restore(c);

        /* this here for debug of anchors..
        cairo_set_source_rgb(c, 255, 0, 0);
        cairo_move_to(c, 0, 0);
        cairo_line_to(c, 0, -10);
        cairo_move_to(c, 0, 0);
        cairo_line_to(c, 10, 0);
        cairo_stroke(c);
        */
}

static gboolean
calculate_bounds(CrItem *item, cairo_t *ct, CrBounds *bounds,
                CrDeviceBounds *device)
{
        CrPixbuf *pixbuf;

        pixbuf = CR_PIXBUF(item);

        if (pixbuf->pattern) {
                if (pixbuf->flags & CR_PIXBUF_SCALEABLE) {
                        get_item_extents(pixbuf, ct, &bounds->x1, &bounds->y1, 
                                        &bounds->x2, &bounds->y2);
                }
                else {
                        get_device_extents(pixbuf, ct, device);
                }
        }
        return TRUE;
}

static CrItem *
test(CrItem *item, cairo_t *ct, double x, double y)
{
        CrPixbuf *pixbuf;
        double px, py;
	guchar *src;

        pixbuf = CR_PIXBUF(item);

        /* Note that at the moment cr-pixbufs initialized with a png file or
         * with a pattern directly vice a pixbuf will not test alpha as it is
         * done with the pixbuf.  To test alpha, it would need to keep the image
         * memory buffer around.  I am not sure how to get to this using the
         * cairo convenience function for loading pixbufs. */

        if ((pixbuf->flags & CR_PIXBUF_TEST_FILL) || !pixbuf->pixbuf) 
                return item;
                
        if (!gdk_pixbuf_get_has_alpha (pixbuf->pixbuf)) return item;


        if (pixbuf->flags & CR_PIXBUF_SCALEABLE) {
                px = (x - item->bounds->x1) * pixbuf->width / 
                        (item->bounds->x2 - item->bounds->x1);
                py = (y - item->bounds->y1) * pixbuf->width / 
                        (item->bounds->y2 - item->bounds->y1);
        }
        else {
                if (!item->device) return NULL;
                cairo_user_to_device_distance(ct, &x, &y);
                px = x - item->device->x1;
                py = y - item->device->y1;
        }
        /*
        g_print("px=%g, py=%g\n", px, py);
        */

        /*
         * same thing as above:
        if (!(pixbuf->flags & CR_PIXBUF_SCALEABLE)) {
                cairo_matrix_invert(matrix);
                cairo_matrix_transform_distance(matrix, &px, &py);
                cairo_matrix_invert(matrix);
        }
        */

        src = gdk_pixbuf_get_pixels (pixbuf->pixbuf) +
                (int)py * gdk_pixbuf_get_rowstride (pixbuf->pixbuf) +
                (int)px * gdk_pixbuf_get_n_channels (pixbuf->pixbuf);

	if (src[3] < 128)
                return NULL;

        return item;
}

static void
cr_pixbuf_init(CrPixbuf *pixbuf)
{
        pixbuf->anchor = GTK_ANCHOR_NW;
        pixbuf->flags |= CR_PIXBUF_SCALEABLE;
}

static void
cr_pixbuf_class_init(CrPixbufClass *klass)
{
        GObjectClass *object_class;
        CrItemClass *item_class;

        object_class = (GObjectClass *) klass;
        item_class = (CrItemClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->get_property = cr_pixbuf_get_property;
        object_class->set_property = cr_pixbuf_set_property;
        object_class->dispose = cr_pixbuf_dispose;
        object_class->finalize = cr_pixbuf_finalize;
        item_class->calculate_bounds = calculate_bounds;
        item_class->test = test;
        item_class->paint = paint;

        g_object_class_install_property
                (object_class,
                 PROP_PIXBUF,
                 g_param_spec_object ("pixbuf", "pixbuf", "The GDK Pixbuf to "
                                "draw. For images that are in other formats "
                                "it may be more convenient to use the "
                                "#CrPixbuf:png or #CrPixbuf:surface properties "
                                "directly.",
                                GDK_TYPE_PIXBUF,
                                G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_PNG,
                 g_param_spec_string ("png", "png", "A png file to convert to "
                                "a cairo_pattern_t for drawing.  This "
                                "can be set as an alternative to creating "
                                "a #GdkPixbuf first.",
                                NULL,
                                G_PARAM_WRITABLE));
        g_object_class_install_property
                (object_class,
                 PROP_WIDTH,
                 g_param_spec_double ("width", "width", "Width of the pixbuf "
                                      "in device units",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      G_PARAM_READABLE));
        g_object_class_install_property
                (object_class,
                 PROP_HEIGHT,
                 g_param_spec_double ("height", "height", "Height of the "
                                      "pixbuf in device units",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      G_PARAM_READABLE));
        g_object_class_install_property
                (object_class,
                 PROP_SCALEABLE,
                 g_param_spec_boolean ("scaleable", "scaleable", "If the "
                                       "pixbuf should scale and rotate to " 
                                       "conform to item units.  Setting this "
                                       "to FALSE will cause the image to "
                                       "always be the same size.  See also "
                                       "#CrInverse for another way to "
                                       "achieve the same effect.",
				       TRUE,
				       G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_ANCHOR,
                 g_param_spec_enum ("anchor", "anchor", "The part of the "
                        "pixbuf that is referenced to the item's x, y "
                        "coordinates",
                        GTK_TYPE_ANCHOR_TYPE,
                        GTK_ANCHOR_NW,
                        G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_X_OFFSET,
                 g_param_spec_double ("x_offset", NULL, "A device offset "
                         "from the item's anchor position.  Only used when "
                         "scaleable=FALSE.",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_Y_OFFSET,
                 g_param_spec_double ("y_offset", NULL, "A device offset "
                         "from the item's anchor position.  Only used when "
                         "scaleable=FALSE.",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                PROP_PATTERN,
                g_param_spec_boxed("pattern", "pattern",
                "Access to the #cairo_pattern_t that is used to render this "
                "image.",
                CR_TYPE_PATTERN,
                G_PARAM_READABLE));
        g_object_class_install_property
                (object_class,
                PROP_SURFACE,
                g_param_spec_boxed("surface", "surface",
                "A #cairo_surface_t may be specified directly in lieu of "
                "using a #GdkPixbuf.  The surface must be of type "
                "CAIRO_SURFACE_TYPE_IMAGE",
                CR_TYPE_SURFACE,
                G_PARAM_WRITABLE));
        g_object_class_install_property
                (object_class,
                 PROP_TEST_FILL,
                 g_param_spec_boolean ("test-fill",
                 "Test Fill",
                 "If the whole rectangular area should be grabbable by the "
                 " pointer regardless of if it is filled. This is for pixbufs "
                 "that have alpha transparency.  when this flag is FALSE, "
                 "parts of the pixbuf with alpha < 128 will test negative.",
                 FALSE,
                 G_PARAM_READWRITE));
}

GType
cr_pixbuf_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrPixbufClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_pixbuf_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrPixbuf),
                0,
                (GInstanceInitFunc) cr_pixbuf_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(CR_TYPE_ITEM,
                        "CrPixbuf", &info, 0);
        }
        return type;
}

/**
 * cr_pixbuf_new:
 * @parent: The parent canvas item.
 * @x: X position of the pixbuf.
 * @y: Y position of the pixbuf.
 *
 * A convenience constructor for creating an pixbuf and adding it to 
 * an item group in one step.
 *
 * Returns: A reference to a new CrItem.  You must call g_object_ref if you
 * intend to use this reference outside the local scope.
 */
CrItem *
cr_pixbuf_new(CrItem *parent, double x, double y,
                const gchar *first_arg_name, ...)
{
        CrItem *item;
        va_list args;

        va_start (args, first_arg_name);
        item = cr_item_construct(parent, CR_TYPE_PIXBUF, first_arg_name, 
                        args);
        va_end (args);

        if (item) {
                g_object_set(item, "x", x, "y", y, NULL);
        }

        return item;
}


