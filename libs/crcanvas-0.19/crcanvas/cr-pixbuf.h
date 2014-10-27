/* cr-pixbuf.h
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
#ifndef _CR_PIXBUF_H_
#define _CR_PIXBUF_H_
 
#include <gtk/gtk.h>
#include <cairo.h>
#include <cr-item.h>

G_BEGIN_DECLS

#define CR_TYPE_PIXBUF  (cr_pixbuf_get_type())

#define CR_PIXBUF(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        CR_TYPE_PIXBUF, CrPixbuf))

#define CR_PIXBUF_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        CR_TYPE_PIXBUF, CrPixbufClass))

#define CR_IS_PIXBUF(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CR_TYPE_PIXBUF))

#define CR_IS_PIXBUF_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CR_TYPE_PIXBUF))

#define CR_PIXBUF_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        CR_TYPE_PIXBUF, CrPixbufClass))

typedef struct _CrPixbuf CrPixbuf;
typedef struct _CrPixbufClass CrPixbufClass;

struct _CrPixbuf
{
        CrItem parent;
        gint32 flags;
        double x_offset, y_offset, width, height;
        cairo_pattern_t *pattern;
        GdkPixbuf *pixbuf;
        GtkAnchorType anchor;
};

struct _CrPixbufClass
{
        CrItemClass parent_class;
};

enum {
        CR_PIXBUF_SCALEABLE = 1 << 0,
        CR_PIXBUF_TEST_FILL = 1 << 1
};

CrItem *cr_pixbuf_new(CrItem *parent, double x, double y,
                const gchar *first_arg_name, ...);

GType cr_pixbuf_get_type(void);


G_END_DECLS

#endif /* _CR_PIXBUF_H_ */
