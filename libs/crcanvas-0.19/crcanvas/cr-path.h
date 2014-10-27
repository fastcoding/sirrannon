/* cr-path.h
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
#ifndef _CR_PATH_H_
#define _CR_PATH_H_
 
#include <cairo.h>
#include "cr-types.h"
#include "cr-item.h"

G_BEGIN_DECLS

#define CR_TYPE_PATH  (cr_path_get_type())

#define CR_PATH(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        CR_TYPE_PATH, CrPath))

#define CR_PATH_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        CR_TYPE_PATH, CrPathClass))

#define CR_IS_PATH(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CR_TYPE_PATH))

#define CR_IS_PATH_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CR_TYPE_PATH))

#define CR_PATH_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        CR_TYPE_PATH, CrPathClass))

typedef struct _CrPath CrPath;
typedef struct _CrPathClass CrPathClass;

struct _CrPath
{
        CrItem parent;
        gint32 flags;
        cairo_path_t *path;
        double line_width;
        guint fill_color_rgba, outline_color_rgba;
        cairo_pattern_t *pattern;
        CrDash *dash;
        cairo_line_cap_t cap;
        cairo_fill_rule_t fill_rule;
        cairo_line_join_t join;
};

struct _CrPathClass
{
        CrItemClass parent_class;

        /**
         * CrPath::make-path:
         * @path:
         * @c: Cairo context with transformation set to match the current item.
         *
         * This signal is part of the calculate-bounds signal for path items.
         * First the path is constructed, then the bounds are calculated.  By
         * overriding this signal and not overriding calculate-bounds, it is
         * possible to render the path here and allow the default handler to
         * calculate the bounds.  The path will be copied from the cairo_t*.
         *
         * Returns: TRUE if the path from the cairo_t* should be used. FALSE to
         * use whatever path was created previously if any.
         */
        gboolean (*make_path)(CrPath *path, cairo_t *c);
};

enum {
        CR_PATH_LINE_SCALEABLE = 1 << 0,
        CR_PATH_PATTERN_SCALEABLE = 1 << 1,
        CR_PATH_LINE_WIDTH = 1 << 2,
        CR_PATH_LINE_WIDTH_USE_Y = 1 << 3,
        CR_PATH_CAP = 1 << 4,
        CR_PATH_FILL_RULE = 1 << 5,
        CR_PATH_JOIN = 1 << 6,
        CR_PATH_TEST_FILL = 1 << 7
};

GType cr_path_get_type(void);

CrItem *cr_path_new(CrItem *parent, const gchar *first_arg_name, ...);

void cr_path_set_color(cairo_t *c, guint rgba);

void cr_path_setup_line(CrPath *path, cairo_t *c);

G_END_DECLS

#endif /* _CR_PATH_H_ */
