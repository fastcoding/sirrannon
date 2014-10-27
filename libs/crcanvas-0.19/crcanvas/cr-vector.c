/* cr-vector.c
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

#include "cr-vector.h"

/**
 * SECTION:cr-vector
 * @title: CrVector
 * @short_description: A simple two point line vector where the end point can be
 * optionally drawn to device coordinates.
 *
 * The is a simple way to create a two point line vector.  The only difference
 * between this object and #CrLine is that the #CrVector:x2 and #CrVector:y2 
 * properties can be optionally referenced to device space.
 */

static GObjectClass *parent_class = NULL;

enum {
        ARG_0,
        PROP_X2,
        PROP_Y2,
        PROP_END_SCALEABLE
};

static void
cr_vector_dispose(GObject *object)
{
        parent_class->dispose(object);
}

static void
cr_vector_finalize(GObject *object)
{
        parent_class->finalize(object);
}

static void
create_path(CrVector *vector, cairo_t *c)
{
        if (!(vector->flags & CR_VECTOR_END_SCALEABLE)) return;
        cairo_new_path(c);
        cairo_move_to(c, 0, 0);
        cairo_rel_line_to(c, vector->x2, vector->y2);
        CR_PATH(vector)->path = cairo_copy_path(c);
}

static gboolean 
calculate_bounds(CrItem *item, cairo_t *c, CrBounds *bounds,
                CrDeviceBounds *device)
{
        CrVector *vector;

        vector = CR_VECTOR(item);

        if (!CR_PATH(item)->path)
                create_path(vector, c);

        if (!(vector->flags & CR_VECTOR_END_SCALEABLE)) {
                if (vector->x2 < 0) 
                        device->x1 = vector->x2 - CR_PATH(item)->line_width;
                else 
                        device->x2 = vector->x2 + CR_PATH(item)->line_width;

                if (vector->y2 < 0) 
                        device->y1 = vector->y2 - CR_PATH(item)->line_width;
                else
                        device->y2 = vector->y2 + CR_PATH(item)->line_width;

                return TRUE;
        }
        return CR_ITEM_CLASS(parent_class)->
                calculate_bounds(item, c, bounds, device);
}

static void
paint(CrItem *item, cairo_t *c)
{
        CrVector *vector;
        CrPath *path;
        double x, y;

        vector = CR_VECTOR(item);
        path = CR_PATH(item);

        if (!(vector->flags & CR_VECTOR_END_SCALEABLE)) {
                x = y = 0;
                cairo_save(c);

                if (path->outline_color_rgba) 
                        cr_path_set_color(c, path->outline_color_rgba);

                cairo_user_to_device(c, &x, &y);
                cairo_identity_matrix(c);
                cairo_new_path(c);
                cairo_move_to(c, x, y);
                cairo_rel_line_to(c, vector->x2, vector->y2);
                cr_path_setup_line(path, c);
                cairo_stroke(c);
                cairo_restore(c);
        }
        else
                CR_ITEM_CLASS(parent_class)->paint(item, c);
}


static CrItem *
test(CrItem *item, cairo_t *c, double x, double y)
{
        CrVector *vector;
        CrItem *ret;

        vector = CR_VECTOR(item);

        if (vector->flags & CR_VECTOR_END_SCALEABLE)
                return CR_ITEM_CLASS(parent_class)->test(item, c, x, y);
        else {
                ret = NULL;
                cairo_save(c);

                cairo_user_to_device(c, &x, &y);

                cairo_new_path(c);
                cairo_move_to(c, 0, 0);
                cairo_identity_matrix(c);
                cairo_rel_line_to(c, vector->x2, vector->y2);

                cairo_set_line_width(c, CR_PATH(item)->line_width);
                if (cairo_in_stroke(c, x, y)) ret = item;
                cairo_restore(c);
        }
        return ret;
}

static void
cr_vector_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        CrVector *vector = (CrVector*) object;
        switch (property_id) {
                case PROP_X2:
                        vector->x2 = g_value_get_double(value);
                        if (CR_PATH(vector)->path) 
                                cairo_path_destroy(CR_PATH(vector)->path);
                        CR_PATH(vector)->path = NULL;
                        cr_item_request_update(CR_ITEM(vector));
                        break;
                case PROP_Y2:
                        vector->y2 = g_value_get_double(value);
                        if (CR_PATH(vector)->path) 
                                cairo_path_destroy(CR_PATH(vector)->path);
                        CR_PATH(vector)->path = NULL;
                        cr_item_request_update(CR_ITEM(vector));
                        break;
                case PROP_END_SCALEABLE:
                        if (g_value_get_boolean(value))
                                vector->flags |= CR_VECTOR_END_SCALEABLE;
                        else
                                vector->flags &= ~CR_VECTOR_END_SCALEABLE;
                        break;
                        cr_item_request_update(CR_ITEM(vector));
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_vector_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrVector *vector = (CrVector*) object;
        switch (property_id) {
                case PROP_X2:
                        g_value_set_double(value, vector->x2);
                        break;
                case PROP_Y2:
                        g_value_set_double(value, vector->y2);
                        break;
                case PROP_END_SCALEABLE:
                        g_value_set_boolean(value, 
                                vector->flags & CR_VECTOR_END_SCALEABLE);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_vector_init(CrVector *vector)
{
        vector->flags |= CR_VECTOR_END_SCALEABLE;
}

static void
cr_vector_class_init(CrVectorClass *klass)
{
        GObjectClass *object_class;
        CrItemClass *item_class;

        object_class = (GObjectClass *) klass;
        item_class = (CrItemClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->get_property = cr_vector_get_property;
        object_class->set_property = cr_vector_set_property;
        object_class->dispose = cr_vector_dispose;
        object_class->finalize = cr_vector_finalize;
        item_class->calculate_bounds = calculate_bounds;
        item_class->test = test;
        item_class->paint = paint;
        g_object_class_install_property
                (object_class,
                 PROP_X2,
                 g_param_spec_double("x2", "x2", "The length of the "
                        "vector in the positive x-axis direction",
                        -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                        G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_Y2,
                 g_param_spec_double("y2", "y2", "The length of the "
                        "vector in the positive y-axis direction",
                        -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                        G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_END_SCALEABLE,
                 g_param_spec_boolean ("end_scaleable", "end scaleable", 
                         "True if the end of the vector is tied to item "
                         "coordinate space. FALSE if the end of the vector is "
                         "tied to device coordinate space.",
				       TRUE,
				       G_PARAM_READWRITE));
}

GType
cr_vector_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrVectorClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_vector_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrVector),
                0,
                (GInstanceInitFunc) cr_vector_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(CR_TYPE_PATH,
                        "CrVector", &info, 0);
        }
        return type;
}

/**
 * cr_vector_new:
 * @parent: The parent item.
 * @x: The starting point of the vector.
 * @y: The starting point of the vector.
 * @x2: The relative ending point of the vector along the x-axis.
 * @y2: The relative ending point of the vector along the y-axis.
 *
 * A convenience function to create a simple line vector and add it to a canvas
 * item parent group in one step.
 *
 * Returns: A reference to a new CrItem.  You must call
 * g_object_ref if you intend to use this reference outside the local scope.
 */
CrItem *
cr_vector_new(CrItem *parent, double x, double y, 
                double x2, double y2,
                const gchar *first_arg_name, ...)
{
        CrItem *item;
        GArray *array;
        va_list args;

        va_start (args, first_arg_name);
        item = cr_item_construct(parent, CR_TYPE_VECTOR, first_arg_name, 
                        args);
        va_end (args);

        if (item) {
                g_object_set(item, "x", x, "y", y, "x2", x2, "y2",
                                y2, NULL);
        }

        return item;
}

