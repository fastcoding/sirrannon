/* cr-arrow.h 
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
#ifndef _CR_ARROW_H_
#define _CR_ARROW_H_
 
#include <cr-path.h>

G_BEGIN_DECLS

#define CR_TYPE_ARROW  (cr_arrow_get_type())

#define CR_ARROW(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        CR_TYPE_ARROW, CrArrow))

#define CR_ARROW_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        CR_TYPE_ARROW, CrArrowClass))

#define CR_IS_ARROW(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CR_TYPE_ARROW))

#define CR_IS_ARROW_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CR_TYPE_ARROW))

#define CR_ARROW_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        CR_TYPE_ARROW, CrArrowClass))

typedef struct _CrArrow CrArrow;
typedef struct _CrArrowClass CrArrowClass;

struct _CrArrow
{
        CrPath parent;
        CrPath *group_parent;
        double length, fatness, depth;
        double x_offset, y_offset;
        int point_id;
        guint flags;
};

struct _CrArrowClass
{
        CrPathClass parent_class;
        /**
         * CrArrow::connect_parent:
         * @arrow:
         * @parent: The arrow's parent group.
         *
         * The arrow is dependent upon it's parent for properties and position.
         */
        void (*connect_parent)(CrArrow *arrow, CrPath *parent);
};

enum {
        CR_ARROW_SCALEABLE = 1 << 0
};

GType cr_arrow_get_type(void);

CrItem *cr_arrow_new(CrItem *parent, int point_id,
                const gchar *first_arg_name, ...);

void cr_arrow_connect_parent(CrArrow *arrow, CrItem *parent);

G_END_DECLS

#endif /* _CR_ARROW_H_ */
