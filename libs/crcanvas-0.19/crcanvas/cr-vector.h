/* cr-vector.h
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

#ifndef _CR_VECTOR_H_
#define _CR_VECTOR_H_
 
#include "cr-path.h"

G_BEGIN_DECLS

#define CR_TYPE_VECTOR  (cr_vector_get_type())

#define CR_VECTOR(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        CR_TYPE_VECTOR, CrVector))

#define CR_VECTOR_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        CR_TYPE_VECTOR, CrVectorClass))

#define CR_IS_VECTOR(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CR_TYPE_VECTOR))

#define CR_IS_VECTOR_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CR_TYPE_VECTOR))

#define CR_VECTOR_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        CR_TYPE_VECTOR, CrVectorClass))

typedef struct _CrVector CrVector;
typedef struct _CrVectorClass CrVectorClass;

struct _CrVector
{
        CrPath parent;
        double x2, y2;
        gint32 flags;
};

struct _CrVectorClass
{
        CrPathClass parent_class;
};

enum {
        CR_VECTOR_END_SCALEABLE = 1 << 0
};

GType cr_vector_get_type(void);

CrItem *cr_vector_new(CrItem *parent, double x, double y, 
                double x2, double y2,
                const gchar *first_arg_name, ...);

G_END_DECLS

#endif /* _CR_VECTOR_H_ */
