/* cr-inverse.h
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

#ifndef _CR_INVERSE_H_
#define _CR_INVERSE_H_
 
#include "cr-item.h"

G_BEGIN_DECLS

#define CR_TYPE_INVERSE  (cr_inverse_get_type())

#define CR_INVERSE(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        CR_TYPE_INVERSE, CrInverse))

#define CR_INVERSE_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        CR_TYPE_INVERSE, CrInverseClass))

#define CR_IS_INVERSE(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CR_TYPE_INVERSE))

#define CR_IS_INVERSE_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CR_TYPE_INVERSE))

#define CR_INVERSE_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        CR_TYPE_INVERSE, CrInverseClass))

typedef struct _CrInverse CrInverse;
typedef struct _CrInverseClass CrInverseClass;

struct _CrInverse
{
        CrItem parent;
        gint flags;
};

struct _CrInverseClass
{
        CrItemClass parent_class;
};

GType cr_inverse_get_type(void);

enum {
        CR_INVERSE_PRESERVE_SCALE = 1 << 0,
        CR_INVERSE_PRESERVE_ROTATION = 1 << 1
};

CrItem *cr_inverse_new(CrItem *parent, double x, double y,
                const gchar *first_arg_name, ...);

G_END_DECLS

#endif /* _CR_INVERSE_H_ */
