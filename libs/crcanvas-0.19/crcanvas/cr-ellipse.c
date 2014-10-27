#include <math.h>
#include "cr-ellipse.h"

/**
 * SECTION:cr-ellipse
 * @title: CrEllipse
 * @short_description: A simple ellipse canvas item.
 *
 * The ellipse is defined by its center point #CrItem:x, #CrItem:y,
 * its #CrEllipse:width half to either side of x, its #CrEllipse:height half to 
 * either side of y, and its #CrEllipse:orientation measure clockwise 
 * from the positive X axis.
 */

static GObjectClass *parent_class = NULL;

enum {
        ARG_0,
        PROP_WIDTH,
        PROP_HEIGHT,
        PROP_ORIENTATION
};

static void
cr_ellipse_dispose(GObject *object)
{
        parent_class->dispose(object);
}

static void
cr_ellipse_finalize(GObject *object)
{
        parent_class->finalize(object);
}

static gboolean
make_path(CrPath *path, cairo_t *c)
{
        double w2, h2, wm, hm;
        CrEllipse *ellipse;

        if (path->path) return FALSE;

        ellipse = CR_ELLIPSE(path);

        if (!(ellipse->flags & (CR_ELLIPSE_WIDTH | CR_ELLIPSE_HEIGHT)))
                return FALSE;

        /* not sure why, but validity of path seems to depend upon what scale it
         * was created at, so here it is created using a real-life cairo_t.
         * Hopefully the scale won't change too drastically. */

        cairo_new_path(c);

        /* It is easier to draw an ellipse using cairo_arc and a matrix
         * transformation.  The problem is that line width also gets skewed
         * by the transformation. Using curves is not as nice but fixes the
         * line_width problem.
         */
        w2 = ellipse->width/2;
        h2 = ellipse->height/2;

        if (ellipse->width == ellipse->height) 
                cairo_arc(c, 0.0, 0.0, w2, 0.0, 2 * M_PI);
        else {
                /* magic number for a circle approximation from
                 * http://www.tinaja.com/cubic01.asp*/
                wm = 0.55228475 * w2;
                hm = 0.55228475 * h2;

                cairo_move_to(c, -w2, 0);
                cairo_curve_to(c, -w2, -hm, -wm, -h2, 0, -h2);
                cairo_curve_to(c, wm, -h2, w2, -hm, w2, 0);
                cairo_curve_to(c, w2, hm, wm, h2, 0, h2);
                cairo_curve_to(c, -wm, h2, -w2, hm, -w2, 0);
        }
        cairo_close_path (c);
        return TRUE;
}

static void
apply_rotation(CrEllipse *ellipse, double orientation)
{
        double x, y, delta;
        cairo_matrix_t *m;

        /* width and height cannot be applied through the matrix because
         * this would skew the line width as well. */

        m = cr_item_get_matrix(CR_ITEM(ellipse));

        delta = orientation - atan2(m->yx, m->yy);
        x = m->x0;
        y = m->y0;
        m->x0 = m->y0 = 0;

        cairo_matrix_rotate(m, delta);
        m->x0 = x;
        m->y0 = y;

        ellipse->orientation = orientation;
        ellipse->flags |= CR_ELLIPSE_ORIENTATION;

        if (CR_PATH(ellipse)->path) 
                cairo_path_destroy(CR_PATH(ellipse)-> path);
        CR_PATH(ellipse)->path = NULL;

        cr_item_request_update(CR_ITEM(ellipse));
}

static void
cr_ellipse_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        CrEllipse *ellipse = (CrEllipse*) object;
        double val;

        switch (property_id) {
                case PROP_WIDTH:
                        val = g_value_get_double (value);
                        if (!(ellipse->flags & CR_ELLIPSE_WIDTH) ||
                                        val != ellipse->width) {
                                ellipse->width = val;
                                ellipse->flags |= CR_ELLIPSE_WIDTH;

                                if (CR_PATH(ellipse)->path) 
                                        cairo_path_destroy(CR_PATH(ellipse)->
                                                        path);
                                CR_PATH(ellipse)->path = NULL;
                                cr_item_request_update(CR_ITEM(ellipse));
                        }
                        break;
                case PROP_HEIGHT:
                        val = g_value_get_double (value);
                        if (!(ellipse->flags & CR_ELLIPSE_HEIGHT) ||
                                        val != ellipse->height) {
                                ellipse->height = val;
                                ellipse->flags |= CR_ELLIPSE_HEIGHT;

                                if (CR_PATH(ellipse)->path) 
                                        cairo_path_destroy(CR_PATH(ellipse)->
                                                        path);
                                CR_PATH(ellipse)->path = NULL;
                                cr_item_request_update(CR_ITEM(ellipse));
                        }
                        break;
                case PROP_ORIENTATION:
                        val = g_value_get_double (value);
                        if (!(ellipse->flags & CR_ELLIPSE_ORIENTATION) ||
                                        val != ellipse->orientation) {
                                apply_rotation(ellipse, val);
                        }
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_ellipse_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrEllipse *ellipse = (CrEllipse*) object;
        switch (property_id) {
                case PROP_WIDTH:
                        g_value_set_double (value, ellipse->width);
                        break;
                case PROP_HEIGHT:
                        g_value_set_double (value, ellipse->height);
                        break;
                case PROP_ORIENTATION:
                        g_value_set_double (value, ellipse->orientation);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_ellipse_init(CrEllipse *ellipse)
{
}

static void
cr_ellipse_class_init(CrEllipseClass *klass)
{
        GObjectClass *object_class;
        CrItemClass *item_class;
        CrPathClass *path_class;

        object_class = (GObjectClass *) klass;
        item_class = (CrItemClass *) klass;
        path_class = (CrPathClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->get_property = cr_ellipse_get_property;
        object_class->set_property = cr_ellipse_set_property;
        object_class->dispose = cr_ellipse_dispose;
        object_class->finalize = cr_ellipse_finalize;
        path_class->make_path = make_path;

        g_object_class_install_property
                (object_class,
                 PROP_WIDTH,
                 g_param_spec_double ("width", "Width", 
                         "Ellipse width axis length in item units. Note this "
                         "is half to the left of #CrItem:x and half to the "
                         "right of #CrItem:x",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_HEIGHT,
                 g_param_spec_double ("height", "Height", 
                         "Ellipse height axis length in item units. Note this "
                         "is half above #CrItem:y and half below #CrItem:y.",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      G_PARAM_READWRITE));

        g_object_class_install_property
                (object_class,
                 PROP_ORIENTATION,
                 g_param_spec_double ("orientation", "Orientation", 
                         "Ellipse orientation angle in radians CW from the "
                         "positive X axis.",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      G_PARAM_READWRITE));
}

GType
cr_ellipse_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrEllipseClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_ellipse_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrEllipse),
                0,
                (GInstanceInitFunc) cr_ellipse_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(CR_TYPE_PATH,
                        "CrEllipse", &info, 0);
        }
        return type;
}

/**
 * cr_ellipse_new:
 * @parent: The parent canvas item.
 * @x: Center point
 * @y: Center point
 * @width: Width in item units.
 * @height: Height in item units.
 * @orientation: Angle of ellipse in degrees counter-clockwise from the 
 * positive X axis.
 *
 * A convenience constructor for creating an ellipse and adding it to an item
 * group in one step.
 *
 * Returns: A reference to a new CrItem.  You must call
 * g_object_ref if you intend to use this reference outside the local scope.
 */
CrItem *cr_ellipse_new(CrItem *parent, double x, double y, 
                double width, double height, double orientation,
                const gchar *first_arg_name, ...)
{
        CrItem *item;
        va_list args;

        va_start (args, first_arg_name);
        item = cr_item_construct(parent, CR_TYPE_ELLIPSE, first_arg_name, 
                        args);
        va_end (args);

        if (item) {
                g_object_set(item, "x", x, "y", y, 
                                "width", width, 
                                "height", height, 
                                "orientation", orientation, NULL);
        }

        return item;

}

