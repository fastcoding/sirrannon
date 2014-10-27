/* cr-zoomer.c 
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
#include <math.h>
#include "cr-marshal.h"
#include "cr-zoomer.h"
#include "cr-rectangle.h"

/**
 * SECTION:cr-zoomer
 * @title: CrZoomer
 * @short_description: A object for setting up box zooming on a 
 * #CrCanvas widget.
 *
 * By default #CrZoomer:fill-color and #CrZoomer:outline-color properties 
 * are disabled.  These should be set
 * at construction time in order to see the zoom box.
 */

static GObjectClass *parent_class = NULL;

enum {
        ARG_0,
        PROP_ACTIVE,
        PROP_FILL_COLOR_RGBA,
        PROP_FILL_COLOR,
        PROP_OUTLINE_COLOR_RGBA,
        PROP_OUTLINE_COLOR,
        PROP_CURSOR,
        PROP_CANVAS,
        PROP_LINE_WIDTH,
        PROP_MAINTAIN_ASPECT,
        PROP_CORNER_TO_CORNER
};

enum {
        ACTIVATE,
        SELECT,
        DEACTIVATE,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
calculate_zoombox(CrZoomer *zoomer, double new_x, double new_y)
{
        double delta_x, delta_y, width, height, x, y, z;
        cairo_matrix_t *matrix_i;

        delta_x = fabs(new_x - zoomer->start_x);
        delta_y = fabs(new_y - zoomer->start_y);

        /* if maintain_aspect, then must keep same aspect ratio as the screen
         * so keep the largest delta  and adjust the
         * smaller delta to reflect the aspect.
         */
        width = GTK_WIDGET(zoomer->canvas)->allocation.width;
        height = GTK_WIDGET(zoomer->canvas)->allocation.height;

        if (zoomer->flags & CR_ZOOMER_MAINTAIN_ASPECT) {
                if (delta_x > delta_y) 
                        delta_y = delta_x * (height/width);
                else
                        delta_x = delta_y * (width/height);
        }

        x = zoomer->start_x;
        y = zoomer->start_y;


        if (zoomer->flags & CR_ZOOMER_CORNER_TO_CORNER) {

                if (new_x < zoomer->start_x)
                        x -= delta_x/2;
                else
                        x += delta_x/2;

                if (new_y < zoomer->start_y)
                        y -= delta_y/2;
                else
                        y += delta_y/2;

                width = delta_x;
                height = delta_y;
        }
        else {
                width =  delta_x*2;
                height = delta_y*2;
        }


        matrix_i = cr_item_get_inverse_matrix(zoomer->canvas->root);
        z = 0;
        cairo_matrix_transform_distance(matrix_i, &width, &z);
        z = 0;
        cairo_matrix_transform_distance(matrix_i, &z, &height);
        cairo_matrix_transform_point(matrix_i, &x, &y);

        g_object_set(zoomer->box, 
                        "x", x,
                        "y", y,
                        "width", width,
                        "height", height,
                        NULL);
}

static gboolean
on_canvas_event(CrZoomer *zoomer, GdkEvent *event, CrCanvas *canvas)
{
        gboolean state, used;
        double cx, cy, w, h, z;
        CrItem *item;

        item = CR_ITEM(canvas->root);

        state = FALSE;

        if (event->type == GDK_BUTTON_PRESS) {
                if (event->button.button == 1) {
                        zoomer->start_x = event->button.x;
                        zoomer->start_y = event->button.y;
                }
                state = TRUE;
        }
        else if (event->type == GDK_MOTION_NOTIFY) {
                if (event->motion.state & GDK_BUTTON1_MASK) {
                        zoomer->flags |= CR_ZOOMER_DRAGGING;
                        calculate_zoombox(zoomer, event->motion.x,
                                        event->motion.y);
                        state = TRUE;
                }
        }
        else if (event->type == GDK_BUTTON_RELEASE) {
                        zoomer->flags &= ~CR_ZOOMER_DRAGGING;
                        calculate_zoombox(zoomer, event->motion.x,
                                        event->motion.y);
                        g_object_get(zoomer->box,
                                        "x", &cx,
                                        "y", &cy,
                                        "width", &w,
                                        "height", &h,
                                        NULL);
                        if (w > 0 && h > 0) {
                                g_signal_emit(zoomer, signals[SELECT], 0,
                                                cx, cy, w, h, &used);

                                if (!used)
                                        cr_canvas_center_scale(canvas, 
                                                        cx, cy, w, h);
                        }
                        cr_zoomer_deactivate(zoomer);
                        state = TRUE;
        }
        return state;
}

static void
set_canvas(CrZoomer *zoomer, GObject *object)
{
        g_return_if_fail (!object || CR_IS_CANVAS(object));

        if (zoomer->canvas) {
                if (zoomer->box) {
                        if (zoomer->canvas->root)
                                cr_item_remove(zoomer->canvas->root, 
                                                zoomer->box);
                        g_object_unref(zoomer->box);
                        zoomer->box = NULL;
                }
                g_object_unref(zoomer->canvas);
                zoomer->canvas = NULL;
        }

        if (object) {
                zoomer->canvas = CR_CANVAS(object);
                g_object_ref(zoomer->canvas);
                zoomer->box = g_object_new(CR_TYPE_RECTANGLE, 
                                "visible", FALSE, 
                                "line_scaleable", FALSE, NULL);
                cr_item_add(zoomer->canvas->root, zoomer->box);
        }
}

static void
cr_zoomer_dispose(GObject *object)
{
        CrZoomer *zoomer;

        zoomer = CR_ZOOMER(object);

        set_canvas(zoomer, NULL);
        parent_class->dispose(object);
}

static void
cr_zoomer_finalize(GObject *object)
{
        parent_class->finalize(object);
}

static void
set_color(cairo_t *c, guint rgba)
{
        double r, g, b, a;

        r = ((rgba & 0xff000000) >> 24) / 255.;
        g = ((rgba & 0x00ff0000) >> 16) / 255.;
        b = ((rgba & 0x0000ff00) >> 8) / 255.;
        a = (rgba & 0xff) / 255.;
        if (a == 1)
                cairo_set_source_rgb(c, r, g, b);
        else
                cairo_set_source_rgba(c, r, g, b, a);
}

static void
cr_zoomer_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrZoomer *zoomer = CR_ZOOMER(object);
        gpointer object_value;
        char color_string[8];

        switch (property_id) {
                case PROP_ACTIVE:
                        g_value_set_boolean(value, zoomer->flags &
                                        CR_ZOOMER_ACTIVE);
                        break;
                case PROP_LINE_WIDTH:
                        g_value_set_double(value, zoomer->line_width);
                        break;
                case PROP_CANVAS:
                        g_value_set_object(value, zoomer->canvas);
                        break;
                case PROP_CURSOR:
                        g_value_set_int(value, zoomer->cursor);
                        break;
                case PROP_FILL_COLOR_RGBA:
                        g_value_set_uint(value, zoomer->fill_color_rgba);
                        break;
                case PROP_FILL_COLOR:
                        sprintf(color_string, "#%.6x", 
                                        zoomer->fill_color_rgba >> 8);
                        g_value_set_string(value, color_string);
                        break;
                case PROP_OUTLINE_COLOR_RGBA:
                        g_value_set_uint(value, 
                                        zoomer->outline_color_rgba);
                        break;
                case PROP_OUTLINE_COLOR:
                        sprintf(color_string, "#%x", 
                                        zoomer->outline_color_rgba >> 8);
                        g_value_set_string(value, color_string);
                        break;
                case PROP_MAINTAIN_ASPECT:
                        g_value_set_boolean(value, zoomer->flags &
                                        CR_ZOOMER_MAINTAIN_ASPECT);
                        break;
                case PROP_CORNER_TO_CORNER:
                        g_value_set_boolean(value, zoomer->flags &
                                        CR_ZOOMER_CORNER_TO_CORNER);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_zoomer_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        const char *str;
        GdkColor color = { 0, 0, 0, 0, };
        CrZoomer *zoomer = CR_ZOOMER(object);
        guint32 flag;
        gboolean bval;

        flag = 0;

        switch (property_id) {
                case PROP_ACTIVE:
                        bval = g_value_get_boolean(value);
                        if (bval && !(zoomer->flags & CR_ZOOMER_ACTIVE))
                                cr_zoomer_activate(zoomer);
                        else if (!bval && (zoomer->flags & CR_ZOOMER_ACTIVE))
                                cr_zoomer_deactivate(zoomer);
                        break;
                case PROP_LINE_WIDTH:
                        zoomer->line_width = g_value_get_double (value);
                        break;
                case PROP_CANVAS:
                        set_canvas(zoomer, g_value_get_object (value));
                        break;
                case PROP_CURSOR:
                        zoomer->cursor = g_value_get_int(value);
                        break;
                case PROP_FILL_COLOR_RGBA:
                        zoomer->fill_color_rgba = g_value_get_uint(value);
                        zoomer->flags |= CR_ZOOMER_FILL_COLOR_RGBA;
                        break;
                case PROP_FILL_COLOR:
                        str = g_value_get_string(value);
                        if (str)
                                gdk_color_parse(str, &color);
                        zoomer->fill_color_rgba = ((color.red & 0xff00) << 16 |
                                        (color.green & 0xff00) << 8 |
                                        (color.blue & 0xff00) |
                                        0xff);
                        zoomer->flags |= CR_ZOOMER_FILL_COLOR_RGBA;
                        break;

                case PROP_OUTLINE_COLOR_RGBA:
                        zoomer->outline_color_rgba = g_value_get_uint(value);
                        zoomer->flags |= CR_ZOOMER_OUTLINE_COLOR_RGBA;
                        break;
                case PROP_OUTLINE_COLOR:
                        str = g_value_get_string(value);
                        if (str)
                                gdk_color_parse(str, &color);
                        zoomer->outline_color_rgba = (
                                        (color.red & 0xff00) << 16 |
                                        (color.green & 0xff00) << 8 |
                                        (color.blue & 0xff00) |
                                        0xff);
                        zoomer->flags |= CR_ZOOMER_OUTLINE_COLOR_RGBA;
                        break;
                case PROP_MAINTAIN_ASPECT:
                        flag = CR_ZOOMER_MAINTAIN_ASPECT;
                        break;
                case PROP_CORNER_TO_CORNER:
                        flag = CR_ZOOMER_CORNER_TO_CORNER;
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
        if (flag) {
                bval = g_value_get_boolean(value);
                if (bval)
                        zoomer->flags |= flag;
                else
                        zoomer->flags &= ~flag;
        }
}
 
static void
cr_zoomer_init(CrZoomer *zoomer)
{
        zoomer->cursor = GDK_ICON;
        zoomer->line_width = 2.;
        zoomer->flags |= CR_ZOOMER_MAINTAIN_ASPECT;
}

static void 
activate(CrZoomer *zoomer)
{
        GdkWindow *window;
        GdkCursor *cursor;
        gboolean flag;

        g_return_if_fail(zoomer->canvas != NULL);
        g_return_if_fail(!(zoomer->flags & CR_ZOOMER_ACTIVE));

        zoomer->flags |= CR_ZOOMER_ACTIVE;

        g_signal_connect_swapped(zoomer->canvas, "event", (GCallback)
                        on_canvas_event, zoomer);

        window = GTK_WIDGET(zoomer->canvas)->window;
        cursor = gdk_cursor_new(zoomer->cursor);
        gdk_window_set_cursor(window, cursor);
        gdk_cursor_unref(cursor);

        if (zoomer->flags & CR_ZOOMER_FILL_COLOR_RGBA)
                g_object_set(zoomer->box, "fill_color_rgba", 
                                zoomer->fill_color_rgba, NULL);
        if (zoomer->flags & CR_ZOOMER_OUTLINE_COLOR_RGBA)
                g_object_set(zoomer->box, "outline_color_rgba", 
                                zoomer->outline_color_rgba, NULL);
        g_object_set(zoomer->box, "visible", TRUE, 
                        "line_width", zoomer->line_width, NULL);

        /* stopping the canvas from running the test during motion
         * improves performance */
        g_object_get(zoomer->canvas->root, "avoid-test", &flag, NULL);
        if (!flag) {
                g_object_set(zoomer->canvas->root, "avoid-test", TRUE, NULL);
                zoomer->flags |= CR_ZOOMER_ROOT_AVOID_TEST;
        }
}

static void 
deactivate(CrZoomer *zoomer)
{
        GdkWindow *window;

        if (!(zoomer->flags & CR_ZOOMER_ACTIVE)) return;

        zoomer->flags &= ~CR_ZOOMER_ACTIVE;

        g_signal_handlers_disconnect_by_func(zoomer->canvas, (GCallback)
                        on_canvas_event, zoomer);

        g_object_set(zoomer->box, "visible", FALSE, "width", 0.0, "height", 0.0,
                        NULL);

        window = GTK_WIDGET(zoomer->canvas)->window;
        gdk_window_set_cursor(window, NULL);

        if (zoomer->flags  & CR_ZOOMER_ROOT_AVOID_TEST) {
                g_object_set(zoomer->canvas->root, "avoid-test", FALSE, NULL);
                zoomer->flags &= ~CR_ZOOMER_ROOT_AVOID_TEST;
        }
}

static gboolean
boolean_handled_accumulator (GSignalInvocationHint *ihint,
                             GValue                *return_accu,
                             const GValue          *handler_return,
                             gpointer               dummy)
{
        gboolean continue_emission;
        gboolean signal_handled;

        signal_handled = g_value_get_boolean (handler_return);
        g_value_set_boolean (return_accu, signal_handled);
        continue_emission = !signal_handled;

        return continue_emission;
}

static void
cr_zoomer_class_init(CrZoomerClass *klass)
{
        GObjectClass *object_class;

        object_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->dispose = cr_zoomer_dispose;
        object_class->finalize = cr_zoomer_finalize;
        object_class->get_property = cr_zoomer_get_property;
        object_class->set_property = cr_zoomer_set_property;
        klass->activate = activate;
        klass->deactivate = deactivate;

        signals[ACTIVATE] = g_signal_new ("activate",
                        CR_TYPE_ZOOMER,
                        G_SIGNAL_RUN_FIRST,
                        G_STRUCT_OFFSET(CrZoomerClass, activate),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);
        signals[SELECT] = g_signal_new ("select", 
                        CR_TYPE_ZOOMER,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(CrZoomerClass, select),
                        boolean_handled_accumulator, NULL,
                        cr_marshal_BOOLEAN__DOUBLE_DOUBLE_DOUBLE_DOUBLE,
                        G_TYPE_BOOLEAN, 4,
                        G_TYPE_DOUBLE, G_TYPE_DOUBLE, 
                        G_TYPE_DOUBLE, G_TYPE_DOUBLE);
        signals[DEACTIVATE] = g_signal_new ("deactivate",
                        CR_TYPE_ZOOMER,
                        G_SIGNAL_RUN_FIRST,
                        G_STRUCT_OFFSET(CrZoomerClass, deactivate),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);

        g_object_class_install_property (object_class, PROP_ACTIVE,
                        g_param_spec_boolean("active",
                                "Active",
                                "Active/Deactivate the zoomer object or "
                                "check the activation status.",
                                FALSE,
                                G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_LINE_WIDTH,
                 g_param_spec_double ("line-width", "Line Width", 
                         "Line width  in user units",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 2,
				      G_PARAM_READWRITE));
        g_object_class_install_property(object_class,
                        PROP_CANVAS,
                        g_param_spec_object("canvas", "CrCanvas",
                                "Reference to CrCanvas widget",
                                CR_TYPE_CANVAS, 
                                G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                        PROP_CURSOR,
                        g_param_spec_int("cursor",
                                "Zoombox Cursor",
                                "GDK Cursor to use when zoombox is selected",
                                0, GDK_LAST_CURSOR, GDK_ICON,
                                G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                        PROP_FILL_COLOR_RGBA,
                        g_param_spec_uint ("fill_color_rgba", 
                                "Zoombox Fill Color RGBA",
                                "Zoombox Fill Color RGBA",
                                0, G_MAXUINT, 0,
                                G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_FILL_COLOR,
                 g_param_spec_string ("fill_color", "Fill Color", 
                         "A string color such as 'red', or '#123456'"
                         " to be used to fill the zoombox.",
                         NULL, G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                        PROP_OUTLINE_COLOR_RGBA,
                        g_param_spec_uint ("outline_color_rgba", 
                                "Zoombox Outline Color RGBA",
                                "Zoombox Outline Color RGBA",
                                0, G_MAXUINT, 0,
                                G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_OUTLINE_COLOR,
                 g_param_spec_string ("outline_color", "Outline Color", 
                         "A string color such as 'red', or '#123456'"
                         " to be used to sroke the zoombox.",
                         NULL, G_PARAM_READWRITE));
        g_object_class_install_property (object_class, PROP_MAINTAIN_ASPECT,
                        g_param_spec_boolean("maintain_aspect",
                                "Maintain Aspect",
                                "If the aspect ratio of the rectangle should "
                                "match the aspect of the canvas widget.",
                                TRUE,
                                G_PARAM_READWRITE));
        g_object_class_install_property (object_class, PROP_CORNER_TO_CORNER,
                        g_param_spec_boolean("corner_to_corner",
                                "Corner to Corner",
                                "If the zoombox should be made by dragging "
                                "from corner to corner.  The default is to "
                                "drag from the center.",
                                FALSE,
                                G_PARAM_READWRITE));
}

GType
cr_zoomer_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrZoomerClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_zoomer_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrZoomer),
                0,
                (GInstanceInitFunc) cr_zoomer_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(G_TYPE_OBJECT,
                        "CrZoomer", &info, 0);
        }
        return type;
}

/**
 * cr_zoomer_new:
 * @canvas: The canvas widget to operate on.
 * @first_arg_name: A list of object argument name/value pairs, NULL-terminated,
 * used to configure the item.
 * @varargs:
 *
 * A factory to create a CrZoomer object and connect it to a #CrCanvas in one
 * step.
 *
 * Returns: A newly created #CrZoomer object.  You must call g_object_unref when
 * finished with it.
 */
CrZoomer *
cr_zoomer_new(CrCanvas *canvas, const gchar *first_arg_name, ...)
{
        CrZoomer *zoomer;
        va_list args;

        g_return_val_if_fail (CR_IS_CANVAS(canvas), NULL);

        zoomer = g_object_new(CR_TYPE_ZOOMER, NULL);
        set_canvas(zoomer, G_OBJECT(canvas));

        va_start (args, first_arg_name);
        g_object_set_valist(G_OBJECT(zoomer), first_arg_name, args);
        va_end (args);

        return zoomer;
}

void 
cr_zoomer_activate(CrZoomer *zoomer)
{
        g_signal_emit(zoomer, signals[ACTIVATE], 0);
}

void 
cr_zoomer_deactivate(CrZoomer *zoomer)
{
        g_signal_emit(zoomer, signals[DEACTIVATE], 0);
}

