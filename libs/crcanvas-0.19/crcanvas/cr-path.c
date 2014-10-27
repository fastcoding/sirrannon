/* cr-path.c
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
#include <string.h>
#include <math.h>
#include <cairo.h>
#include "cr-types.h"
#include "cr-marshal.h"
#include "cr-path.h"

#define DEBUG_PROFILE 0

/**
 * SECTION:cr-path
 * @title: CrPath
 * @short_description: A canvas item capable of rendering a #cairo_path_t.
 *
 * This is the base class for canvas items that are drawn from the
 * #cairo_path_t such as #CrEllipse, #CrRectangle, and #CrLine.
 */

#if DEBUG_PROFILE
static int num_paths = 0;
static int num_empties = 0;
void
cr_path_print_stats()
{
        g_print("\t\tnum paths=%d, num enpties=%d\n", num_paths, num_empties);
        num_paths = num_empties = 0;
}
#endif

enum {
        ARG_0,
        PROP_LINE_SCALEABLE,
        PROP_PATTERN_SCALEABLE,
        PROP_PATH,
        PROP_FILL_COLOR_RGBA,
        PROP_FILL_COLOR,
        PROP_OUTLINE_COLOR_RGBA,
        PROP_OUTLINE_COLOR,
        PROP_LINE_WIDTH,
        PROP_LINE_WIDTH_USE_Y,
        PROP_PATTERN,
        PROP_DASH,
        PROP_CAP,
        PROP_FILL_RULE,
        PROP_JOIN,
        PROP_TEST_FILL
};

enum {
        MAKE_PATH,
        LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

static void
cr_path_dispose(GObject *object)
{
        CrPath *path;

        path = CR_PATH(object);

        if (path->pattern)
                cairo_pattern_destroy(path->pattern);
        path->pattern = NULL;
        if (path->path)
                cairo_path_destroy(path->path);
        path->path = NULL;

        if (path->dash)
                cr_dash_unref(path->dash);
        path->dash = NULL;

        parent_class->dispose(object);
}

static void
cr_path_finalize(GObject *object)
{
        parent_class->finalize(object);
}

static void
cr_path_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        const char *str;
        GdkColor color = { 0, 0, 0, 0, };
        CrPath *path = (CrPath*) object;

        switch (property_id) {
                case PROP_PATH:
                        if (path->path)
                                cairo_path_destroy(path->path);
                        path->path = g_value_get_pointer(value);
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_LINE_SCALEABLE:
                        if (g_value_get_boolean(value))
                                path->flags |= CR_PATH_LINE_SCALEABLE;
                        else
                                path->flags &= ~CR_PATH_LINE_SCALEABLE;
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_PATTERN_SCALEABLE:
                        if (g_value_get_boolean(value))
                                path->flags |= CR_PATH_PATTERN_SCALEABLE;
                        else
                                path->flags &= ~CR_PATH_PATTERN_SCALEABLE;
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_LINE_WIDTH:
                        path->line_width = g_value_get_double (value);
                        path->flags |= CR_PATH_LINE_WIDTH;
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_LINE_WIDTH_USE_Y:
                        if (g_value_get_boolean(value))
                                path->flags |= CR_PATH_LINE_WIDTH_USE_Y;
                        else
                                path->flags &= ~CR_PATH_LINE_WIDTH_USE_Y;
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_PATTERN:
                        if (path->pattern) 
                                cairo_pattern_destroy(path->pattern);
                        path->pattern = g_value_dup_boxed(value);
                        break;
                case PROP_FILL_COLOR_RGBA:
                        path->fill_color_rgba = g_value_get_uint(value);
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_FILL_COLOR:
                        str = g_value_get_string(value);
                        if (str)
                                gdk_color_parse(str, &color);
                        path->fill_color_rgba = ((color.red & 0xff00) << 16 |
                                        (color.green & 0xff00) << 8 |
                                        (color.blue & 0xff00) |
                                        0xff);
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_OUTLINE_COLOR_RGBA:
                        path->outline_color_rgba = g_value_get_uint(value);
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_OUTLINE_COLOR:
                        str = g_value_get_string(value);
                        if (str)
                                gdk_color_parse(str, &color);
                        path->outline_color_rgba = ((color.red & 0xff00) << 16 |
                                        (color.green & 0xff00) << 8 |
                                        (color.blue & 0xff00) |
                                        0xff);
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_DASH:
                        if (path->dash)
                                cr_dash_unref(path->dash);
                        path->dash = g_value_dup_boxed(value);
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_CAP:
                        path->flags |= CR_PATH_CAP;
                        path->cap = g_value_get_enum(value);
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_FILL_RULE:
                        path->flags |= CR_PATH_FILL_RULE;
                        path->fill_rule = g_value_get_enum(value);
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_JOIN:
                        path->flags |= CR_PATH_JOIN;
                        path->join = g_value_get_enum(value);
                        cr_item_request_update(CR_ITEM(path));
                        break;
                case PROP_TEST_FILL:
                        if (g_value_get_boolean(value))
                                path->flags |= CR_PATH_TEST_FILL;
                        else
                                path->flags &= ~CR_PATH_TEST_FILL;
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_path_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrPath *path = (CrPath*) object;
        char color_string[8];

        switch (property_id) {
                case PROP_LINE_WIDTH:
                        g_value_set_double (value, path->line_width);
                        break;
                case PROP_LINE_WIDTH_USE_Y:
                        g_value_set_boolean(value, 
                                path->flags & CR_PATH_LINE_WIDTH_USE_Y);
                        break;
                case PROP_PATTERN:
                        g_value_set_boxed(value, path->pattern);
                        break;
                case PROP_PATTERN_SCALEABLE:
                        g_value_set_boolean(value, 
                                path->flags & CR_PATH_PATTERN_SCALEABLE);
                        break;
                case PROP_LINE_SCALEABLE:
                        g_value_set_boolean(value, 
                                path->flags & CR_PATH_LINE_SCALEABLE);
                        break;
                case PROP_OUTLINE_COLOR_RGBA:
                        g_value_set_uint(value, path->outline_color_rgba);
                        break;
                case PROP_OUTLINE_COLOR:
                        sprintf(color_string, "#%.6x", 
                                        path->outline_color_rgba >> 8);
                        g_value_set_string(value, color_string);
                        break;
                case PROP_FILL_COLOR_RGBA:
                        g_value_set_uint(value, path->fill_color_rgba);
                        break;
                case PROP_FILL_COLOR:
                        sprintf(color_string, "#%.6x", 
                                        path->fill_color_rgba >> 8);
                        g_value_set_string(value, color_string);
                        break;
                case PROP_PATH:
                        g_value_set_pointer(value, path->path);
                        break;
                case PROP_DASH:
                        g_value_set_boxed(value, path->dash);
                        break;
                case PROP_CAP:
                        g_value_set_enum(value, path->cap);
                        break;
                case PROP_FILL_RULE:
                        g_value_set_enum(value, path->fill_rule);
                        break;
                case PROP_JOIN:
                        g_value_set_enum(value, path->join);
                        break;
                case PROP_TEST_FILL:
                        g_value_set_boolean(value, 
                                path->flags & CR_PATH_TEST_FILL);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_path_init(CrPath *path)
{
        path->line_width = 2;
        path->flags |= (CR_PATH_LINE_SCALEABLE | CR_PATH_PATTERN_SCALEABLE);
}

/**
 * cr_path_set_color:
 * @c: A cairo context to set the color into.
 * @rgba: An integer representing RGBA as four characters.
 *
 * A convenience function to set the rgba integer into the cairo 
 * context.  It may be called by derived #CrPath inplementations.
 */
void
cr_path_set_color(cairo_t *c, guint rgba)
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

/**
 * cr_path_setup_line:
 * @path: The path object.
 * @c: The current cairo context.
 *
 * A convenience function to set up the #CrPath:line-width,
 * #CrPath:line-scaleable, and #CrPath:dash properties into the cairo context.
 * It may be called by derived implementations of #CrPath.
 */
void
cr_path_setup_line(CrPath *path, cairo_t *c)
{
        double dx, dy, *dashes;
        cairo_matrix_t matrix;
        int i;

        if (!(path->flags & CR_PATH_LINE_SCALEABLE)) {
                cairo_get_matrix(c, &matrix);
                cairo_matrix_rotate(&matrix, 
                                -atan2(matrix.yx, matrix.yy));
                cairo_matrix_invert(&matrix);

                /* line width needs to be set explicitly when the line is not 
                 * scaleable otherwise the width will change with scale */
                dx = dy = path->line_width;
                cairo_matrix_transform_distance(&matrix, &dx, &dy);
                cairo_set_line_width(c, 
                                path->flags & CR_PATH_LINE_WIDTH_USE_Y ?
                                dy : dx);
        }
        else if (path->flags & CR_PATH_LINE_WIDTH)
                cairo_set_line_width(c, path->line_width);


        if (path->dash) {
                if (!(path->flags & CR_PATH_LINE_SCALEABLE)) {
                        dashes = g_memdup(path->dash->array->data,
                                        path->dash->array->len * 
                                        sizeof(double));
                        for (i = 0; i < path->dash->array->len; i++) {
                                dx = dy = dashes[i];
                                cairo_matrix_transform_distance(
                                                &matrix, &dx, &dy);
                                dashes[i] = 
                                        path->flags & CR_PATH_LINE_WIDTH_USE_Y ?
                                        dy : dx;
                        }
                        dx = dy = path->dash->offset;
                        cairo_matrix_transform_distance(&matrix, &dx, &dy);
                        cairo_set_dash(c, dashes, path->dash->array->len, 
                                        path->flags & CR_PATH_LINE_WIDTH_USE_Y ?
                                        dx : dy);
                }
                else 
                        cairo_set_dash(c, (double *) path->dash->array->data,
                                        path->dash->array->len,
                                        path->dash->offset);
        }
}

static gboolean 
calculate_bounds(CrItem *item, cairo_t *c, CrBounds *bounds,
                CrDeviceBounds *device)
{
        CrPath *path;
        cairo_matrix_t matrix;
        gboolean use_path;

        path = CR_PATH(item);
        use_path = FALSE;

        g_signal_emit(path, signals[MAKE_PATH], 0, c, &use_path);

        if (use_path) {
                if (path->path)
                        cairo_path_destroy(path->path);
                path->path = cairo_copy_path(c);
        }

        if (path->path) {
                cairo_save(c);

                /* The item base class expects the unrotated user-extents to be
                 * reported back.  This is because it needs to do bounds
                 * checking on a proper rectangle. So the rotation must be
                 * removed, but the approximate scale must be preserved. */
                cairo_get_matrix(c, &matrix);
                cairo_rotate(c, -atan2(matrix.yx, matrix.yy));

                cairo_new_path(c);
                cairo_append_path(c, path->path);

                /* This adds about 20% to the cpu time for this routine, but it
                 * should only run after a request-update. */
                if (path->outline_color_rgba) {
                        if (path->flags & CR_PATH_LINE_SCALEABLE) {

                                cr_path_setup_line(path, c);
                                cairo_stroke_extents(c, 
                                                &bounds->x1, &bounds->y1, 
                                                &bounds->x2, &bounds->y2);
                        }
                        else {
                                device->anchor = GTK_ANCHOR_CENTER;
                                device->x1 = device->y1 = -path->line_width;
                                device->x2 = device->y2 = path->line_width;

                                cairo_path_extents(c, 
                                                &bounds->x1, &bounds->y1,
                                                &bounds->x2, &bounds->y2);

                                if (bounds->x2 - bounds->x1 == 1.0) {
                                        bounds->x1 += .5;
                                        bounds->x2 -= .5;
                                }
                                else if (bounds->y2 - bounds->y1 == 1.0) {
                                        bounds->y1 += .5;
                                        bounds->y2 -= .5;
                                }
                        }
                }
                else {
                        cairo_fill_extents(c, &bounds->x1, &bounds->y1,
                                        &bounds->x2, &bounds->y2);
                }

                cairo_restore(c);
                return TRUE;
        }
        return FALSE;
}

static void
paint(CrItem *item, cairo_t *c)
{
        CrPath *path;
        cairo_matrix_t matrix, matrix2;
        int i;

        path = CR_PATH(item);

        /*
                cairo_matrix_t m;
                double w, h, th;
                w = 200;
                h = 100;
                cairo_get_matrix(c,&m);
                th = atan2(m.yx, m.yy);
                g_print("w = %g, h = %g\n", w*cos(th) - h*sin(th), h * cos(th)+w*sin(th));

                */

#if 0
        /* temporary used for profiling*/
        if (!path->points) return;

        if (item->flags & CR_ITEM_NEED_UPDATE) {
                create_path(path, c);

                if (path->path) cairo_path_destroy(path->path);
                path->path = cairo_copy_path(c);
                item->flags &= ~CR_ITEM_NEED_UPDATE;
        }
#endif

#if DEBUG_PROFILE
        num_paths++;
#endif
        if (!path->path) {
#if DEBUG_PROFILE
                num_empties++;
#endif
                return;
        }

        cairo_new_path(c);
        cairo_append_path(c, path->path);

        if (path->pattern) {
                if (!(path->flags & CR_PATH_PATTERN_SCALEABLE)) {
                        cairo_pattern_get_matrix(path->pattern, &matrix);
                        cairo_get_matrix(c, &matrix2);
                        cairo_matrix_multiply(&matrix2, &matrix, &matrix2);
                        cairo_pattern_set_matrix(path->pattern, &matrix2);
                }
                cairo_set_source(c, path->pattern);
                cairo_fill_preserve(c);
                if (!(path->flags & CR_PATH_PATTERN_SCALEABLE)) {
                        cairo_pattern_set_matrix(path->pattern, &matrix);
                }
        }
        else if (path->fill_color_rgba) {
                if (path->flags & CR_PATH_FILL_RULE)
                        cairo_set_fill_rule(c, path->fill_rule);
                cr_path_set_color(c, path->fill_color_rgba);
                cairo_fill_preserve(c);
        }

        if (path->outline_color_rgba) {
                cr_path_set_color(c, path->outline_color_rgba);

                cr_path_setup_line(path, c);

                if (path->flags & CR_PATH_CAP) 
                        cairo_set_line_cap(c, path->cap);
                if (path->flags & CR_PATH_JOIN)
                        cairo_set_line_join(c, path->join);

                cairo_stroke(c);
        }
}

static CrItem *
test(CrItem *item, cairo_t *ct, double x, double y)
{
        CrPath *path;
        CrItem *ret;

        path = CR_PATH(item);
        ret = NULL;

        if (path->path) {
                cairo_new_path(ct);
                cairo_append_path(ct, path->path);
                if (((path->flags & CR_PATH_TEST_FILL) ||
                                path->fill_color_rgba || path->pattern)
                                && cairo_in_fill(ct, x, y))
                        ret = item;
                else if (path->outline_color_rgba) {
                        cr_path_setup_line(path, ct);
                        if (cairo_in_stroke(ct, x, y))
                                ret = item;
                }
        }
        return ret;
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
cr_path_class_init(CrPathClass *klass)
{
        GObjectClass *object_class;
        CrItemClass *item_class;

        object_class = (GObjectClass *) klass;
        item_class = (CrItemClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->get_property = cr_path_get_property;
        object_class->set_property = cr_path_set_property;
        object_class->dispose = cr_path_dispose;
        object_class->finalize = cr_path_finalize;
        item_class->calculate_bounds = calculate_bounds;
        item_class->test = test;
        item_class->paint = paint;

        g_object_class_install_property
                (object_class,
                 PROP_LINE_WIDTH,
                 g_param_spec_double ("line_width", "Line Width", 
                 "Path line width in user units when #CrPath:line-scaleable "
                 " is TRUE.  When #CrPath:line-scaleable is FALSE this is "
                 " in device units",
                 -G_MAXDOUBLE, G_MAXDOUBLE, 2,
                 G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_LINE_WIDTH_USE_Y,
                 g_param_spec_boolean ("line_width_use_y", "Line Width Use Y", 
                 "If the Y axis should be used for determining the "
                 "device coordinates for the line-width. "
                 "This property only applies when line-scalable is FALSE, "
                 "outline-color is set, and the canvas widget has "
                 " the maintain-aspect property set to FALSE. "
                 "The default is to use the x-axis transformation to "
                 "determine device line-width.",
				       FALSE,
				       G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_PATTERN,
                 g_param_spec_boxed("pattern", "pattern",
                         "Cairo pattern to fill the path region",
                         CR_TYPE_PATTERN,
                         G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_LINE_SCALEABLE,
                 g_param_spec_boolean ("line_scaleable", "Line Scaleable", 
                         "If line width should be scaleable",
				       TRUE,
				       G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_PATTERN_SCALEABLE,
                 g_param_spec_boolean ("pattern_scaleable",
                         "Pattern Scaleable", 
                         "If pattern should scale",
				       TRUE,
				       G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_PATH,
                 g_param_spec_pointer("path", "path", "a cairo_path_t. "
                         "On set the item owns it.  On get the item still "
                         "owns it. This is exclusive with points.",
                         G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_FILL_COLOR_RGBA,
                 g_param_spec_uint ("fill_color_rgba", NULL, 
                                    "Region fill color, red,grn,blue,alpha",
				    0, G_MAXUINT, 0,
				    G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_FILL_COLOR,
                 g_param_spec_string ("fill_color", "Fill Color", 
                         "A string color such as 'red', or '#123456'"
                         " to be used to fill the path.",
                         NULL, G_PARAM_READWRITE));

        g_object_class_install_property
                (object_class,
                 PROP_OUTLINE_COLOR_RGBA,
                 g_param_spec_uint ("outline-color-rgba", NULL, 
                                    "Path color, red,grn,blue,alpha",
				    0, G_MAXUINT, 0,
				    G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_OUTLINE_COLOR,
                 g_param_spec_string ("outline-color", "Outline Color", 
                         "A string color such as 'red', or '#123456'"
                         " to be used to sroke the path.",
                         NULL, G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_DASH,
                 g_param_spec_boxed("dash", "Dash", "a boxed array "
                         "indicating the dash pattern to be used by Cairo. "
                         " See #cairo_set_dash for more information.",
                         CR_TYPE_DASH,
                         G_PARAM_READWRITE));
        g_object_class_install_property(object_class,
                        PROP_CAP,
                        g_param_spec_enum("cap", "Cap", "The enumeration for "
                                "the style of line endings.  See "
                                "#cairo_line_cap_t for more information.",
                                CR_TYPE_CAP, 0, G_PARAM_READWRITE));
        g_object_class_install_property(object_class,
                        PROP_FILL_RULE,
                        g_param_spec_enum("fill_rule", "Fill Rule", 
                                "The enumeration used to determine how paths "
                                "are filled.  See #cairo_fill_rule_t for "
                                "more information.",
                                CR_TYPE_FILL_RULE, 0,  G_PARAM_READWRITE));
        g_object_class_install_property(object_class,
                        PROP_JOIN,
                        g_param_spec_enum("join", "Join", "The enumeration for "
                                "determining how line segments are joined "
                                "together.  See #cairo_line_join_t for more "
                                "information.",
                                CR_TYPE_JOIN, 0, G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_TEST_FILL,
                 g_param_spec_boolean ("test-fill",
                         "Test Fill", 
                         "If the fill area should be grabbable by the pointer "
                         " regardless of if it is filled.  The most common use "
                         "for this is as an invisible child of a very small "
                         "item that would be difficult to grab on its own.",
				       FALSE,
				       G_PARAM_READWRITE));
        signals[MAKE_PATH] = g_signal_new ("make-path", CR_TYPE_PATH,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(CrPathClass, make_path),
                        boolean_handled_accumulator,
                        NULL,
                        cr_marshal_BOOLEAN__BOXED,
                        G_TYPE_BOOLEAN, 1, CR_TYPE_CONTEXT);
}

GType
cr_path_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrPathClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_path_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrPath),
                0,
                (GInstanceInitFunc) cr_path_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(CR_TYPE_ITEM,
                        "CrPath", &info, 0);
        }
        return type;
}

/**
 * cr_path_new:
 * @parent: The parent canvas item.
 *
 * A convenience function to create a path item and add it to a canvas item
 * parent group in one step.
 *
 * Returns: A reference to a new CrItem.  You must call
 * g_object_ref if you intend to keep it around.
 */
CrItem *
cr_path_new(CrItem *parent, const gchar *first_arg_name, ...)
{
        CrItem *item;
        va_list args;

        va_start (args, first_arg_name);
        item = cr_item_construct(parent, CR_TYPE_PATH, first_arg_name, args);
        va_end (args);

        return item;
}

