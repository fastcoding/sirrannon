/* cr-ellipse.h
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
#ifndef _CR_ELLIPSE_H_
#define _CR_ELLIPSE_H_
 
#include "cr-path.h"

G_BEGIN_DECLS

#define CR_TYPE_ELLIPSE  (cr_ellipse_get_type())

#define CR_ELLIPSE(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        CR_TYPE_ELLIPSE, CrEllipse))

#define CR_ELLIPSE_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        CR_TYPE_ELLIPSE, CrEllipseClass))

#define CR_IS_ELLIPSE(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CR_TYPE_ELLIPSE))

#define CR_IS_ELLIPSE_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CR_TYPE_ELLIPSE))

#define CR_ELLIPSE_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        CR_TYPE_ELLIPSE, CrEllipseClass))

typedef struct _CrEllipse CrEllipse;
typedef struct _CrEllipseClass CrEllipseClass;

struct _CrEllipse
{
        CrPath parent;
        gint32 flags;
        double width, height, orientation;
};

struct _CrEllipseClass
{
        CrPathClass parent_class;
};

enum {
        CR_ELLIPSE_WIDTH = 1 << 0,
        CR_ELLIPSE_HEIGHT = 1 << 1,
        CR_ELLIPSE_ORIENTATION = 1 << 2,
};

GType cr_ellipse_get_type(void);

CrItem *cr_ellipse_new(CrItem *parent, double x, double y, 
                double width, double height, double orientation,
                const gchar *first_arg_name, ...);


G_END_DECLS

#endif /* _CR_ELLIPSE_H_ */
