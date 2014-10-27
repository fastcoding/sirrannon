/* cr-zoomer.h
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
#ifndef _CR_ZOOMER_H_
#define _CR_ZOOMER_H_
 
#include <gdk/gdk.h>
#include <cr-canvas.h>
#include <cr-item.h>

G_BEGIN_DECLS

#define CR_TYPE_ZOOMER  (cr_zoomer_get_type())

#define CR_ZOOMER(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        CR_TYPE_ZOOMER, CrZoomer))

#define CR_ZOOMER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        CR_TYPE_ZOOMER, CrZoomerClass))

#define CR_IS_ZOOMER(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CR_TYPE_ZOOMER))

#define CR_IS_ZOOMER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CR_TYPE_ZOOMER))

#define CR_ZOOMER_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        CR_TYPE_ZOOMER, CrZoomerClass))

typedef struct _CrZoomer CrZoomer;
typedef struct _CrZoomerClass CrZoomerClass;

struct _CrZoomer
{
        GObject parent;
        CrItem *box;
        CrCanvas *canvas;
        double start_x, start_y, line_width;
        guint fill_color_rgba, outline_color_rgba;
        GdkCursorType cursor;
        gboolean flags;
};

struct _CrZoomerClass
{
        GObjectClass parent_class;
        /**
         * CrZoomer::activate:
         * @zoomer:
         *
         * This signal is emitted whenever the zoombox is first activated.
         */
        void (*activate)(CrZoomer *zoomer);
        /**
         * CrZoomer::select:
         * @zoomer:
         * @cx: The selected area center x coordinate.
         * @cy: The selected area center y coordinate.
         * @w: The selected area width.
         * @h: The selected area height.
         *
         * This signal is emitted just prior to zooming the canvas.  It can be
         * intercepted and used to change the default behavior of the zoomer 
         * to possibly do something other than zoom the canvas. 
         *
         * Returns: True to stop the canvas from being zoomed.  False to allow
         * the canvas to be zoomed.
         */
        void (*select)(CrZoomer *zoomer, double cx, double cy, 
              double w, double h);
        /**
         * CrZoomer::deactivate:
         * @zoomer:
         *
         * This signal is emitted whenever the zoombox is deactivated. 
         * It can be used to get to a callback from a zoombox selection.
         */
        void (*deactivate)(CrZoomer *zoomer);
};

GType cr_zoomer_get_type(void);

void cr_zoomer_activate(CrZoomer *zoomer);
void cr_zoomer_deactivate(CrZoomer *zoomer);

CrZoomer *cr_zoomer_new(CrCanvas *canvas, const gchar *first_arg_name, ...);

enum {
        CR_ZOOMER_FILL_COLOR_RGBA = 1 << 0,
        CR_ZOOMER_OUTLINE_COLOR_RGBA = 1 << 1,
        CR_ZOOMER_DRAGGING = 1 << 2,
        CR_ZOOMER_ACTIVE = 1 << 3,
        CR_ZOOMER_MAINTAIN_ASPECT = 1 << 4,
        CR_ZOOMER_CORNER_TO_CORNER = 1 << 5,
        CR_ZOOMER_ROOT_AVOID_TEST = 1 << 6
};

G_END_DECLS

#endif /* _CR_ZOOMER_H_ */
