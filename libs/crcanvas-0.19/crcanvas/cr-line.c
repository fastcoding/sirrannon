/* cr-line.c
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
#include "cr-line.h"

/**
 * SECTION:cr-line
 * @title: CrLine
 * @short_description: A polygon or polyline canvas item.
 *
 * This canvas item can render polygons or polylines.
 */

static GObjectClass *parent_class = NULL;

enum {
        ARG_0,
        PROP_POINTS,
        PROP_ARRAY,
        PROP_CLOSE
};

static void
cr_line_dispose(GObject *object)
{
        CrLine *line;

        line = CR_LINE(object);

        if (line->points)
                cr_points_unref(line->points);
        line->points = NULL;

        parent_class->dispose(object);
}

static void
cr_line_finalize(GObject *object)
{
        parent_class->finalize(object);
}

static void
points_to_path(CrLine *line, cairo_t *c)
{
        GArray *array;
        double h, w;
        int i;

        if (!line->points) return;

        array = line->points->array;

        if (array->len < 2) return;

        cairo_new_path(c);

        cairo_move_to(c, g_array_index(array, double, 0),
                        g_array_index(array, double, 1));

        for (i = 2; i < array->len; i += 2) {
                cairo_line_to(c, g_array_index(array, double, i),
                                g_array_index(array, double, i+1));
        }

        if (line->flags & CR_LINE_CLOSE)
                cairo_close_path(c);

        CR_PATH(line)->path = cairo_copy_path(c);
}

static gboolean 
calculate_bounds(CrItem *item, cairo_t *c, CrBounds *bounds,
                CrDeviceBounds *device)
{
        /* not sure why, but validity of path seems to depend upon what scale it
         * was created at, so here it is created using a real-life cairo_t.
         * Hopefully the scale won't change too drastically. */

        if (!CR_PATH(item)->path)
                points_to_path(CR_LINE(item), c);

        return CR_ITEM_CLASS(parent_class)->
                calculate_bounds(item, c, bounds, device);
}

static void
cr_line_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        GArray *array;
        CrLine *line = (CrLine*) object;

        switch (property_id) {
                case PROP_ARRAY:
                        if (line->points)
                                cr_points_unref(line->points);
                        array = g_value_get_pointer(value);
                        if (array) {
                                line->points = g_new(CrPoints, 1);
                                line->points->array = array;
                                line->points->ref_count = 1;
                        }
                        else 
                                line->points = NULL;

                        if (CR_PATH(line)->path) 
                                cairo_path_destroy(CR_PATH(line)->path);
                        CR_PATH(line)->path = NULL;
                        cr_item_request_update(CR_ITEM(line));
                        break;
                case PROP_POINTS:
                        if (line->points)
                                cr_points_unref(line->points);
                        line->points = g_value_dup_boxed(value);
                        if (CR_PATH(line)->path) 
                                cairo_path_destroy(CR_PATH(line)->path);
                        CR_PATH(line)->path = NULL;
                        cr_item_request_update(CR_ITEM(line));
                        break;
                case PROP_CLOSE:
                        if (g_value_get_boolean(value))
                                line->flags |= CR_LINE_CLOSE;
                        else
                                line->flags &= ~CR_LINE_CLOSE;
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_line_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrLine *line = (CrLine*) object;
        switch (property_id) {
                case PROP_POINTS:
                        g_value_set_boxed(value, line->points);
                        break;
                case PROP_CLOSE:
                        g_value_set_boolean(value, 
                                        line->flags & CR_LINE_CLOSE);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_line_init(CrLine *line)
{
        line->flags |= CR_LINE_CLOSE;
}

static void
cr_line_class_init(CrLineClass *klass)
{
        GObjectClass *object_class;
        CrItemClass *item_class;

        object_class = (GObjectClass *) klass;
        item_class = (CrItemClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->get_property = cr_line_get_property;
        object_class->set_property = cr_line_set_property;
        object_class->dispose = cr_line_dispose;
        object_class->finalize = cr_line_finalize;
        item_class->calculate_bounds = calculate_bounds;
        g_object_class_install_property
                (object_class,
                 PROP_ARRAY,
                 g_param_spec_pointer("array", NULL, "an array of double"
                         "precision x,y values. item takes ownership.",
                         G_PARAM_WRITABLE));
        g_object_class_install_property
                (object_class,
                 PROP_POINTS,
                 g_param_spec_boxed("points", "Points", "a boxed array "
                         "of double precision x,y values",
                         CR_TYPE_POINTS,
                         G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_CLOSE,
                 g_param_spec_boolean ("close", "close", 
                         "True, if it is a polygon.",
				       TRUE,
				       G_PARAM_READWRITE));
}

GType
cr_line_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrLineClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_line_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrLine),
                0,
                (GInstanceInitFunc) cr_line_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(CR_TYPE_PATH,
                        "CrLine", &info, 0);
        }
        return type;
}

/**
 * cr_line_new:
 * @parent: The parent canvas item.
 *
 * A convenience function to create a polyline and add it to a canvas item
 * parent group in one step.
 *
 * Returns: A reference to a new CrItem.  You must call
 * g_object_ref if you intend to keep it around.
 */
CrItem *
cr_line_new(CrItem *parent, const gchar *first_arg_name, ...)
{
        CrItem *item;
        va_list args;

        va_start (args, first_arg_name);
        item = cr_item_construct(parent, CR_TYPE_LINE, first_arg_name, args);
        va_end (args);

        return item;
}

