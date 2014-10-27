/* cr-rectangle.h
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
#ifndef _CR_RECTANGLE_H_
#define _CR_RECTANGLE_H_
 
#include "cr-path.h"

G_BEGIN_DECLS

#define CR_TYPE_RECTANGLE  (cr_rectangle_get_type())

#define CR_RECTANGLE(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        CR_TYPE_RECTANGLE, CrRectangle))

#define CR_RECTANGLE_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        CR_TYPE_RECTANGLE, CrRectangleClass))

#define CR_IS_RECTANGLE(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CR_TYPE_RECTANGLE))

#define CR_IS_RECTANGLE_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CR_TYPE_RECTANGLE))

#define CR_RECTANGLE_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        CR_TYPE_RECTANGLE, CrRectangleClass))

typedef struct _CrRectangle CrRectangle;
typedef struct _CrRectangleClass CrRectangleClass;

struct _CrRectangle
{
        CrPath parent;
        gint32 flags;
        gdouble width, height;
};

struct _CrRectangleClass
{
        CrPathClass parent_class;
};

enum {
        CR_RECTANGLE_WIDTH = 1 << 0,
        CR_RECTANGLE_HEIGHT = 1 << 1,
};

GType cr_rectangle_get_type(void);

CrItem *cr_rectangle_new(CrItem *parent, double x, double y, 
                double width, double height, const gchar *first_arg_name, ...);


G_END_DECLS

#endif /* _CR_RECTANGLE_H_ */
