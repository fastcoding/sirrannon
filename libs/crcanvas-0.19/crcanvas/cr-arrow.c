/* cr-arrow.c 
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
#include "cr-vector.h"
#include "cr-line.h"
#include "cr-arrow.h"

/**
 * SECTION:cr-arrow
 * @title: CrArrow
 * @short_description: An arrowhead that can be used to cap a line segment.
 *
 * The arrow is typically used at the start or end of a line, but can be used
 * independently or part of some other arrangement.  The #cr_arrow_new
 * convenience constructor will automatically place the arrow on a point of the
 * line segment if it is called with #CrVector or #CrLine as a parent group.
 *
 * When the convenience constructor #cr_arrow_new is used the #CrArrow:angle,
 * #CrArrow:scaleable, #CrArrow:x_offset, #CrArrow:y_offset, #CrItem:x, and
 * #CrItem:y properties are set and updated automatically.  When the arrow used
 * in some other way, these properties need to be set-up by the user.
 */

static GObjectClass *parent_class = NULL;

enum {
        ARG_0,
        PROP_LENGTH,
        PROP_FATNESS,
        PROP_DEPTH,
        PROP_ANGLE,
        PROP_SCALEABLE,
        PROP_POINT_ID,
        PROP_X_OFFSET,
        PROP_Y_OFFSET
};

static void
parent_request_update(CrArrow *arrow, CrPath *parent)
{
        gboolean scaleable;
        guint color;
        double angle, x1, y1, x2, y2, offset;
        int i;
        CrLine *line;

        g_object_get(parent, "outline_color_rgba", &color,
                        "line_scaleable", &scaleable, NULL);
        g_object_set(arrow, "fill_color_rgba", color,
                        "line_scaleable", scaleable, NULL);

        /* The arrow point gets offset slightly from the parent location so 
         * that it looks like a point.  This is not ideal.  For more accuracy
         * the arrow should be created independently from the line. */
        offset = MIN(arrow->length, arrow->fatness);

        if (CR_IS_VECTOR(parent)) {
                g_object_get(parent, "x2", &x2, "y2", &y2, 
                                "end_scaleable", &scaleable, NULL);

                if (arrow->point_id >= 0)
                        angle = atan2(-y2, -x2);
                else
                        angle = atan2(y2, x2);

                if (arrow->point_id % 2 == 0)
                        x2 = y2 = 0;

                x2 += offset * cos(angle);
                y2 += offset * sin(angle);

                if (!scaleable)
                        g_object_set(arrow, "x_offset", x2,
                                        "y_offset", y2, 
                                        "angle", angle,
                                        "scaleable", FALSE, NULL);
                else
                        g_object_set(arrow, "x", x2,
                                        "y", y2, 
                                        "angle", angle, NULL);

        }
        else if (CR_IS_LINE(parent)) {
                line = CR_LINE(parent);

                if (!line->points || line->points->array->len < 4) {
                        cr_item_hide(CR_ITEM(arrow));
                        return;
                }
                else
                        cr_item_show(CR_ITEM(arrow));

                if (arrow->point_id < 0) {
                        i = line->points->array->len/2 + arrow->point_id - 1;
                        if (i < 0) i = 0;
                        x1 = g_array_index(line->points->array, double, i*2);
                        y1 = g_array_index(line->points->array, double, i*2+1);
                        i += 1;
                        x2 = g_array_index(line->points->array, double, i*2);
                        y2 = g_array_index(line->points->array, double, i*2+1);
                }
                else {
                        i = arrow->point_id + 1;
                        if (i >= line->points->array->len/2) 
                                i = line->points->array->len/2 - 1;
                        x1 = g_array_index(line->points->array, double, i*2);
                        y1 = g_array_index(line->points->array, double, i*2+1);
                        i -= 1;
                        x2 = g_array_index(line->points->array, double, i*2);
                        y2 = g_array_index(line->points->array, double, i*2+1);
                }
                angle = atan2(y2 - y1, x2 - x1);
                x2 += offset * cos(angle);
                y2 += offset * sin(angle);
                g_object_set(arrow, "x", x2, "y", y2, "angle", angle, NULL);
        }
}

static void
connect_parent(CrArrow *arrow, CrPath *parent)
{
        if (arrow->group_parent) {
                g_signal_handlers_disconnect_by_func(arrow->group_parent,
                                (GCallback)parent_request_update, arrow);
                arrow->group_parent = NULL;
        }
        if (parent) {
                arrow->group_parent = parent;
                g_signal_connect_object(parent, "request_update",
                                (GCallback)parent_request_update, 
                                arrow, G_CONNECT_SWAPPED);
                parent_request_update(arrow, parent); 
        }
}

static void
cr_arrow_dispose(GObject *object)
{
        connect_parent(CR_ARROW(object), NULL);

        parent_class->dispose(object);
}

static void
cr_arrow_finalize(GObject *object)
{
        parent_class->finalize(object);
}

static void
make_path(CrArrow *arrow, cairo_t *c)
{
        cairo_new_path(c);
        cairo_move_to(c, 0, 0);
        cairo_line_to(c, -arrow->length, -arrow->fatness/2);
        cairo_rel_line_to(c, arrow->depth, arrow->fatness/2);
        cairo_line_to(c, -arrow->length, arrow->fatness/2);
        cairo_close_path(c);
}

static gboolean 
calculate_bounds(CrItem *item, cairo_t *c, CrBounds *bounds,
                CrDeviceBounds *device)
{
        CrArrow *arrow;
        gboolean status;

        arrow = CR_ARROW(item);

        if (!CR_PATH(item)->path) {
                make_path(arrow, c);
                CR_PATH(arrow)->path = cairo_copy_path(c);
        }

        if (!CR_PATH(arrow)->path) return FALSE;

        if (arrow->flags & CR_ARROW_SCALEABLE) {


                status = CR_ITEM_CLASS(parent_class)->
                        calculate_bounds(item, c, bounds, device);

        }
        else {
                cairo_save(c);
                cairo_identity_matrix(c);

                if (item->matrix)
                        cairo_rotate(c, atan2(item->matrix->yx, 
                                                item->matrix->yy));
                cairo_new_path(c);
                cairo_append_path(c, CR_PATH(item)->path);
                cairo_identity_matrix(c);

                cairo_fill_extents(c, &device->x1, &device->y1, 
                                &device->x2, &device->y2);
                
                device->anchor = GTK_ANCHOR_E;

                device->x1 += arrow->x_offset;
                device->y1 += arrow->y_offset;
                device->x2 += arrow->x_offset;
                device->y2 += arrow->y_offset;

                cairo_restore(c);
#if 0
                /* This does about the same thing as above, but it doesn't look
                 * as nice. */
                w = -arrow->length + (arrow->depth < 0 ? -arrow->depth : 0);
                h = arrow->fatness/2;
                ax1 = ax2 = w;
                ay1 = -h;
                ay2 = h;
                g_print("%g %g %g %g\n", ax1, ay1, ax2, ay2);
                if (item->matrix) {
                        angle = atan2(item->matrix->yx, item->matrix->yy);
                        g_print("angle %g\n", (180/M_PI)*angle);
                        cairo_matrix_init_identity(&matrix);
                        cairo_matrix_rotate(&matrix, angle);
                        cairo_matrix_transform_point(&matrix, &ax1, &ay1);
                        cairo_matrix_transform_point(&matrix, &ax2, &ay2);
                }
                g_print("%g %g %g %g\n", ax1, ay1, ax2, ay2);
                device->x1 = MIN(MIN(ax1, ax2), 0);
                device->x2 = MAX(MAX(ax1, ax2), 0);
                device->y1 = MIN(MIN(ay1, ay2), 0);
                device->y2 = MAX(MAX(ay1, ay2), 0);
                device->anchor = GTK_ANCHOR_E;
                g_print("%g %g %g %g\n", device->x1,device->y1,device->x2,device->y2);
#endif
                        
                status = TRUE;
        }
        return status;
}

static void
paint(CrItem *item, cairo_t *c)
{
        CrArrow *arrow;
        double x, y;

        arrow = CR_ARROW(item);

        cairo_save(c);
        if (!(arrow->flags & CR_ARROW_SCALEABLE)) {
                x = y = 0;
                cairo_user_to_device(c, &x, &y);
                cairo_identity_matrix(c);
                cairo_translate(c, x + arrow->x_offset, y + arrow->y_offset);
                if (item->matrix)
                        cairo_rotate(c, atan2(item->matrix->yx, 
                                                item->matrix->yy));
        }
        CR_ITEM_CLASS(parent_class)->paint(item, c);
        cairo_restore(c);
}

static CrItem *
test(CrItem *item, cairo_t *c, double x, double y)
{
        CrArrow *arrow;
        double ax, ay;
        CrItem *ret;

        arrow = CR_ARROW(item);
        ret = NULL;

        if (arrow->flags & CR_ARROW_SCALEABLE)
                ret = CR_ITEM_CLASS(parent_class)->test(item, c, x, y);
        else {
                cairo_save(c);
                ax = ay = 0;
                cairo_user_to_device(c, &ax, &ay);
                cairo_user_to_device(c, &x, &y);
                cairo_identity_matrix(c);
                cairo_translate(c, ax + arrow->x_offset, ay + arrow->y_offset);
                if (item->matrix)
                        cairo_rotate(c, atan2(item->matrix->yx, 
                                                item->matrix->yy));
                cairo_device_to_user(c, &x, &y);
                ret = CR_ITEM_CLASS(parent_class)->test(item, c, x, y);
                cairo_restore(c);
        }

        return ret;
}

static void
cr_arrow_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        CrArrow *arrow = (CrArrow*) object;
        cairo_matrix_t *matrix;
        double angle;

        switch (property_id) {
                case PROP_LENGTH:
                        arrow->length = g_value_get_double(value);
                        cr_item_request_update(CR_ITEM(arrow));
                        break;
                case PROP_FATNESS:
                        arrow->fatness = g_value_get_double(value);
                        cr_item_request_update(CR_ITEM(arrow));
                        break;
                case PROP_DEPTH:
                        arrow->depth = g_value_get_double(value);
                        cr_item_request_update(CR_ITEM(arrow));
                        break;
                case PROP_ANGLE:
                        angle = g_value_get_double(value);
                        matrix = cr_item_get_matrix(CR_ITEM(arrow));
                        cairo_matrix_rotate(matrix, angle - atan2(matrix->yx,
                                                matrix->yy));
                        cr_item_request_update(CR_ITEM(arrow));
                        break;
                case PROP_SCALEABLE:
                        if (g_value_get_boolean(value))
                                arrow->flags |= CR_ARROW_SCALEABLE;
                        else
                                arrow->flags &= ~CR_ARROW_SCALEABLE;
                        cr_item_request_update(CR_ITEM(arrow));
                        break;
                case PROP_POINT_ID:
                        arrow->point_id = g_value_get_int(value);
                        cr_item_request_update(CR_ITEM(arrow));
                        break;
                case PROP_X_OFFSET:
                        arrow->x_offset = g_value_get_double (value);
                        cr_item_request_update(CR_ITEM(arrow));
                        break;
                case PROP_Y_OFFSET:
                        arrow->y_offset = g_value_get_double (value);
                        cr_item_request_update(CR_ITEM(arrow));
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_arrow_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrArrow *arrow = (CrArrow*) object;
        cairo_matrix_t *matrix;
        double angle;

        switch (property_id) {
                case PROP_LENGTH:
                        g_value_set_double(value, arrow->length);
                        break;
                case PROP_FATNESS:
                        g_value_set_double(value, arrow->fatness);
                        break;
                case PROP_DEPTH:
                        g_value_set_double(value, arrow->depth);
                        break;
                case PROP_ANGLE:
                        angle = 0;
                        if (CR_ITEM(arrow)->matrix)
                                angle = atan2(CR_ITEM(arrow)->matrix->yx,
                                                CR_ITEM(arrow)->matrix->yy);
                        g_value_set_double(value, angle);
                        break;
                case PROP_SCALEABLE:
                        g_value_set_boolean(value, 
                                        arrow->flags & CR_ARROW_SCALEABLE);
                        break;
                case PROP_POINT_ID:
                        g_value_set_int(value, arrow->point_id);
                        break;
                case PROP_X_OFFSET:
                        g_value_set_double (value, arrow->x_offset);
                        break;
                case PROP_Y_OFFSET:
                        g_value_set_double (value, arrow->y_offset);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_arrow_init(CrArrow *arrow)
{
        arrow->length = 14;
        arrow->fatness = 7;
        arrow->depth = 3;
        arrow->flags |= CR_ARROW_SCALEABLE;
}

static void
cr_arrow_class_init(CrArrowClass *klass)
{
        GObjectClass *object_class;
        CrItemClass *item_class;

        object_class = (GObjectClass *) klass;
        item_class = (CrItemClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->get_property = cr_arrow_get_property;
        object_class->set_property = cr_arrow_set_property;
        object_class->dispose = cr_arrow_dispose;
        object_class->finalize = cr_arrow_finalize;
        item_class->calculate_bounds = calculate_bounds;
        item_class->test = test;
        item_class->paint = paint;
        klass->connect_parent = connect_parent;
        g_object_class_install_property
                (object_class,
                 PROP_LENGTH,
                 g_param_spec_double("length", NULL, "How long from the tip "
                        "to the end along the direction it is pointing.",
                        -G_MAXDOUBLE, G_MAXDOUBLE, 14,
                        G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_FATNESS,
                 g_param_spec_double("fatness", NULL, "How wide measured "
                        "perpendicular to the pointing direction.",
                        -G_MAXDOUBLE, G_MAXDOUBLE, 7,
                        G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_DEPTH,
                 g_param_spec_double("depth", NULL, "The concaveness or "
                        "convexness if negative.",
                        -G_MAXDOUBLE, G_MAXDOUBLE, 3,
                        G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_ANGLE,
                 g_param_spec_double("angle", NULL, "The angle in radians to "
                         "which the arrow points.  Zero points East.  Positive"
                         " angles increase CW. When parented to #CrLine or "
                         " #CrVector, this property will be set automatically.",
                        -M_PI, 2*M_PI, 0,
                        G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_SCALEABLE,
                 g_param_spec_boolean ("scaleable", "scaleable", "If the "
                        "arrow should scale to conform to item units.  Setting "
                        "this to FALSE will cause the arrow to always be the "
                        "same size.  When using the arrow with #CrLine or "
                        "#CrVector, this will be set automatically to conform "
                        "to the 'line-scaleable' property. See also #CrInverse "
                        "for another way to achieve ths same effect.",
				       TRUE,
				       G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_POINT_ID,
                 g_param_spec_int("point_id", NULL, 
                         "Negative values count from the back and point toward "
                         " the end.  Positive values count from the front and "
                         " point toward the beginning.  Use 0 for a 'start' "
                         "arrow and -1 for an 'end' arrow.  This is only "
                         "valid when parented to a #CrLine or #CrVector item.",
                         -G_MAXINT, G_MAXINT, -1,
                         G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_X_OFFSET,
                 g_param_spec_double ("x_offset", NULL, "A device offset "
                         "from the arrow's point.  Only used when "
                         "scaleable=FALSE.",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_Y_OFFSET,
                 g_param_spec_double ("y_offset", NULL, "A device offset "
                         "from the arrow's point.  Only used when "
                         "scaleable=FALSE.",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      G_PARAM_READWRITE));

}

GType
cr_arrow_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrArrowClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_arrow_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrArrow),
                0,
                (GInstanceInitFunc) cr_arrow_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(CR_TYPE_PATH,
                        "CrArrow", &info, 0);
        }
        return type;
}

/**
 * cr_arrow_new:
 * @parent: The parent item.
 * @point_id: Negative values count from the back and point toward the end.
 * Positive values count from the front and point toward the beginning.  Use 0
 * for a 'start' arrow and -1 for an 'end' arrow. This is only valid when
 * parented to a #CrLine or #CrVector item.
 *
 * A convenience function to create an arrow-head and add it to a canvas
 * item parent group in one step. If the item is added to a @CrVector or a
 * @CrLine parent, then most set-up will be taken care of automatically by this
 * constructor.
 *
 * Returns: A reference to a new CrItem.  You must call
 * g_object_ref if you intend to use this reference outside the local scope.
 */
CrItem *cr_arrow_new(CrItem *parent, int point_id,
                const gchar *first_arg_name, ...)
{
        CrItem *item;
        GArray *array;
        va_list args;

        va_start (args, first_arg_name);
        item = cr_item_construct(parent, CR_TYPE_ARROW, first_arg_name, 
                        args);
        va_end (args);

        if (item) {
                g_object_set(item, "point_id", point_id, NULL);
        }
        if (CR_IS_PATH(parent)) {
                (*CR_ARROW_GET_CLASS(item)->connect_parent)
                        (CR_ARROW(item), CR_PATH(parent));
        }

        return item;
}

/**
 * cr_arrow_connect_parent:
 * @arrow:
 * @parent: Parent item.  Should be a #CrLine or #CrVector.
 *
 * Connects the arrow to a parent line or vector so it can automatically 
 * align itself.  Normally, when using the C convenience constructor
 * #cr_arrow_new, it is not necessary to call this. The routine is here for 
 * language bindings or where @g_object_new is used.
 */
void cr_arrow_connect_parent(CrArrow *arrow, CrItem *parent)
{
        g_return_if_fail(CR_IS_PATH(parent));

        (*CR_ARROW_GET_CLASS(arrow)->connect_parent) (arrow, CR_PATH(parent));
}

