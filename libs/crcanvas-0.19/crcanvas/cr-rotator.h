/* cr-rotator.h
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
#ifndef _CR_ROTATOR_H_
#define _CR_ROTATOR_H_
 
#include <gdk/gdk.h>
#include <cr-canvas.h>

G_BEGIN_DECLS

#define CR_TYPE_ROTATOR  (cr_rotator_get_type())

#define CR_ROTATOR(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        CR_TYPE_ROTATOR, CrRotator))

#define CR_ROTATOR_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        CR_TYPE_ROTATOR, CrRotatorClass))

#define CR_IS_ROTATOR(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CR_TYPE_ROTATOR))

#define CR_IS_ROTATOR_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), CR_TYPE_ROTATOR))

#define CR_ROTATOR_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        CR_TYPE_ROTATOR, CrRotatorClass))

typedef struct _CrRotator CrRotator;
typedef struct _CrRotatorClass CrRotatorClass;

struct _CrRotator
{
        GObject parent;
        CrCanvas *canvas;
        GdkCursorType cursor;
        double last_msec, last_angle;
        guint flags;
};

struct _CrRotatorClass
{
        GObjectClass parent_class;
};

GType cr_rotator_get_type(void);

void cr_rotator_activate(CrRotator *panner);
void cr_rotator_deactivate(CrRotator *rotator);

CrRotator *cr_rotator_new(CrCanvas *canvas, const gchar *first_arg_name, ...);

enum {
        CR_ROTATOR_DRAGGING = 1 << 0,
        CR_ROTATOR_ACTIVE = 1 << 1,
        CR_ROTATOR_ROOT_AVOID_TEST = 1 << 2
};

G_END_DECLS

#endif /* _CR_ROTATOR_H_ */
