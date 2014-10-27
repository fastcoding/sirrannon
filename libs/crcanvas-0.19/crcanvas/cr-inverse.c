/* cr-inverse.c
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
#include "cr-inverse.h"

/**
 * SECTION:cr-inverse
 * @title: CrInverse
 * @short_description: An item group that reverses a transformation higher up in
 * the item tree.
 *
 * This group can be used to reverse the cumulative scale and rotational effects
 * of all affine transformations higher than it in the item tree.  It will
 * always anchor the group to the x and y property settings according to the
 * current transformation matrix.  Once the anchor is established, it will then
 * optionally reverse the effects of scaling, rotation, or both.
 *
 * The typical use case for #CrInverse is for annotating a feature on a plot or
 * diagram.  As the feature is scaled or rotated, the annotation stays at the
 * same scale and or rotation. Another way to produce this same effect is to use
 * the 'scaleable' properties on #CrText, #CrVector, and #CrPixbuf.
 *
 * <emphasis>Warning</emphasis> this item will not function properly when
 * implemented concurrently on multiple views unless transformations formed
 * above it are the same in each view.
 */

static GObjectClass *parent_class = NULL;

enum {
        ARG_0,
        PROP_PRESERVE_SCALE,
        PROP_PRESERVE_ROTATION
};


static void
report_new_bounds(CrItem *item, cairo_t *c, gboolean all)
{
        cairo_matrix_t m1, *m2, cm;
        CrInverse *inverse;
        double x0, y0, angle;

        inverse = CR_INVERSE(item);

        /* user may have set x, y translation, so we want to keep it and put it
         * back when we are done. */
        x0 = y0 = 0;
        if (item->matrix) {
                x0 = item->matrix->x0;
                y0 = item->matrix->y0;
        }

        cairo_get_matrix(c, &cm);

        angle = atan2(cm.yx, cm.yy);

        m1 = cm;
        m1.x0 = m1.y0 = 0;

        if ((inverse->flags & CR_INVERSE_PRESERVE_ROTATION) && 
                        (inverse->flags & CR_INVERSE_PRESERVE_SCALE)) {
                cairo_matrix_init_identity(&m1);
        }
        else if (inverse->flags & CR_INVERSE_PRESERVE_ROTATION) {
                cairo_matrix_invert(&m1);
                cairo_matrix_rotate(&m1, angle);
        }
        else if (inverse->flags & CR_INVERSE_PRESERVE_SCALE) {
                cairo_matrix_init_identity(&m1);
                cairo_matrix_rotate(&m1, -angle);
        }
        else {
                cairo_matrix_invert(&m1);
        }

        m2 = cr_item_get_matrix(item);

        /* don't need this if logic since an update cycle is not required to
         * change the item matrix.
        if (m1.xx != m2->xx || m1.yx != m2->yx ||
                        m1.xy != m2->xy || m1.yy != m2->yy) {
                        */

        *m2 = m1;
        m2->x0 = x0;
        m2->y0 = y0;
        CR_ITEM_CLASS(parent_class)->report_new_bounds(item, c, all);
}

static void
cr_inverse_dispose(GObject *object)
{
        CrInverse *inverse;

        inverse = CR_INVERSE(object);

        parent_class->dispose(object);

}

static void
cr_inverse_finalize(GObject *object)
{
        parent_class->finalize(object);
}

static void
cr_inverse_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        CrInverse *inverse = (CrInverse*) object;
        guint32 flag;
        gboolean bval;

        flag = 0;
        switch (property_id) {
                case PROP_PRESERVE_SCALE:
                        flag = CR_INVERSE_PRESERVE_SCALE;
                        break;
                case PROP_PRESERVE_ROTATION:
                        flag = CR_INVERSE_PRESERVE_ROTATION;
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
        if (flag) {
                bval = g_value_get_boolean(value);
                if (bval)
                        inverse->flags |= flag;
                else
                        inverse->flags &= ~flag;
        }
}

static void
cr_inverse_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrInverse *inverse = (CrInverse*) object;
        switch (property_id) {
                case PROP_PRESERVE_SCALE:
                        g_value_set_boolean(value, inverse->flags &
                                        CR_INVERSE_PRESERVE_SCALE);
                        break;
                case PROP_PRESERVE_ROTATION:
                        g_value_set_boolean(value, inverse->flags &
                                        CR_INVERSE_PRESERVE_ROTATION);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_inverse_init(CrInverse *inverse)
{
}


static void
cr_inverse_class_init(CrInverseClass *klass)
{
        GObjectClass *object_class;
        CrItemClass *item_class;

        object_class = (GObjectClass *) klass;
        item_class = (CrItemClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->get_property = cr_inverse_get_property;
        object_class->set_property = cr_inverse_set_property;
        object_class->dispose = cr_inverse_dispose;
        object_class->finalize = cr_inverse_finalize;
        item_class->report_new_bounds = report_new_bounds;
        g_object_class_install_property (object_class, PROP_PRESERVE_SCALE,
                        g_param_spec_boolean("preserve_scale",
                                "Preserve Scale",
                                "The scaling from the parent is not altered.",
                                FALSE,
                                G_PARAM_READWRITE));
        g_object_class_install_property (object_class, 
                        PROP_PRESERVE_ROTATION,
                        g_param_spec_boolean("preserve_rotation",
                                "Preserve Rotation",
                                "The rotation from the parent is not altered.",
                                FALSE,
                                G_PARAM_READWRITE));
}

GType
cr_inverse_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrInverseClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_inverse_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrInverse),
                0,
                (GInstanceInitFunc) cr_inverse_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(CR_TYPE_ITEM,
                        "CrInverse", &info, 0);
        }
        return type;
}

/**
 * cr_inverse_new:
 * @parent: The parent canvas item.
 * @x: X position of the group.
 * @y: Y position of the group.
 *
 * A convenience constructor for creating an inverse group and adding it to 
 * an item group in one step.  The X and Y coordinates will mark the anchor
 * point in the item space of the parent group.
 *
 * Returns: A reference to a new CrItem.  You must call g_object_ref if you
 * intend to use this reference outside the local scope.
 */
CrItem *
cr_inverse_new(CrItem *parent, double x, double y,
                const gchar *first_arg_name, ...)
{
        CrItem *item;
        va_list args;

        va_start (args, first_arg_name);
        item = cr_item_construct(parent, CR_TYPE_INVERSE, first_arg_name, 
                        args);
        va_end (args);

        if (item) {
                g_object_set(item, "x", x, "y", y, NULL);
        }

        return item;
}

