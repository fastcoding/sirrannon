/* cr-line.h
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

#ifndef _CR_LINE_H_
#define _CR_LINE_H_
 
#include "cr-path.h"

G_BEGIN_DECLS

#define CR_TYPE_LINE  (cr_line_get_type())

#define CR_LINE(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        CR_TYPE_LINE, CrLine))

#define CR_LINE_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        CR_TYPE_LINE, CrLineClass))

#define CR_IS_LINE(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CR_TYPE_LINE))

#define CR_IS_LINE_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CR_TYPE_LINE))

#define CR_LINE_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        CR_TYPE_LINE, CrLineClass))

typedef struct _CrLine CrLine;
typedef struct _CrLineClass CrLineClass;

struct _CrLine
{
        CrPath parent;
        gint32 flags;
        CrPoints *points;
};

struct _CrLineClass
{
        CrPathClass parent_class;
};

enum {
        CR_LINE_CLOSE = 1 << 0
};

GType cr_line_get_type(void);

CrItem *cr_line_new(CrItem *parent, const gchar *first_arg_name, ...);


G_END_DECLS

#endif /* _CR_LINE_H_ */
