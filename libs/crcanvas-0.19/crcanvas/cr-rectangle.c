#include "cr-rectangle.h"

/**
 * SECTION:cr-rectangle
 * @title: CrRectangle
 * @short_description: A simple rectangle canvas item.
 *
 * This is a simple rectangle path.  <emphasis>Note:</emphasis> that the 
 * #CrItem:x and #CrItem:y coordinates refer to the center of the 
 * rectangle and the #CrRectangle:width and #CrRectangle:height properties
 * are taken half to either side of the x, y position.
 */

static GObjectClass *parent_class = NULL;

enum {
        ARG_0,
        PROP_WIDTH,
        PROP_HEIGHT,
};

static void
cr_rectangle_dispose(GObject *object)
{
        parent_class->dispose(object);
}

static void
cr_rectangle_finalize(GObject *object)
{
        parent_class->finalize(object);
}

static gboolean
make_path(CrPath *path, cairo_t *c)
{
        CrRectangle *rectangle;

        if (path->path) return FALSE;

        rectangle = CR_RECTANGLE(path);

        if (!(rectangle->flags & (CR_RECTANGLE_WIDTH | CR_RECTANGLE_HEIGHT)))
                return FALSE;

        /* not sure why, but validity of path seems to depend upon what scale it
         * was created at, so here it is created using a real-life cairo_t.
         * Hopefully the scale won't change too drastically. */

        cairo_new_path(c);

        cairo_rectangle(c, -rectangle->width/2, -rectangle->height/2, 
                        rectangle->width, rectangle->height);

        return TRUE;
}

static void
cr_rectangle_set_property(GObject *object, guint property_id,
                const GValue *value, GParamSpec *pspec)
{
        CrRectangle *rectangle = (CrRectangle*) object;
        double val;

        switch (property_id) {
                case PROP_WIDTH:
                        val = g_value_get_double (value);
                        if (!(rectangle->flags & CR_RECTANGLE_WIDTH) ||
                                        val != rectangle->width) {
                                rectangle->width = val;
                                rectangle->flags |= CR_RECTANGLE_WIDTH;

                                if (CR_PATH(rectangle)->path) 
                                        cairo_path_destroy(CR_PATH(rectangle)->
                                                        path);
                                CR_PATH(rectangle)->path = NULL;
                                cr_item_request_update(CR_ITEM(rectangle));
                        }
                        break;
                case PROP_HEIGHT:
                        val = g_value_get_double (value);
                        if (!(rectangle->flags & CR_RECTANGLE_HEIGHT) ||
                                        val != rectangle->height) {
                                rectangle->height = val;
                                rectangle->flags |= CR_RECTANGLE_HEIGHT;

                                if (CR_PATH(rectangle)->path) 
                                        cairo_path_destroy(CR_PATH(rectangle)->
                                                        path);
                                CR_PATH(rectangle)->path = NULL;
                                cr_item_request_update(CR_ITEM(rectangle));
                        }
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_rectangle_get_property(GObject *object, guint property_id,
                GValue *value, GParamSpec *pspec)
{
        CrRectangle *rectangle = (CrRectangle*) object;
        switch (property_id) {
                case PROP_WIDTH:
                        g_value_set_double (value, rectangle->width);
                        break;
                case PROP_HEIGHT:
                        g_value_set_double (value, rectangle->height);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                property_id, pspec);
        }
}

static void
cr_rectangle_init(CrRectangle *rectangle)
{
}

static void
cr_rectangle_class_init(CrRectangleClass *klass)
{
        GObjectClass *object_class;
        CrItemClass *item_class;
        CrPathClass *path_class;

        object_class = (GObjectClass *) klass;
        item_class = (CrItemClass *) klass;
        path_class = (CrPathClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->get_property = cr_rectangle_get_property;
        object_class->set_property = cr_rectangle_set_property;
        object_class->dispose = cr_rectangle_dispose;
        object_class->finalize = cr_rectangle_finalize;
        path_class->make_path = make_path;

        g_object_class_install_property
                (object_class,
                 PROP_WIDTH,
                 g_param_spec_double ("width", "Width", 
                         "Width of rectangle in item units. Note this "
                         "is half to the left of #CrItem:x and half to the "
                         "right of #CrItem:x",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 2,
				      G_PARAM_READWRITE));
        g_object_class_install_property
                (object_class,
                 PROP_HEIGHT,
                 g_param_spec_double ("height", "Height", 
                         "Height of rectangle in item units. Note this is half "
                         "above #CrItem:y and half below #CrItem:y.",
				      -G_MAXDOUBLE, G_MAXDOUBLE, 2,
				      G_PARAM_READWRITE));
}

GType
cr_rectangle_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrRectangleClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_rectangle_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrRectangle),
                0,
                (GInstanceInitFunc) cr_rectangle_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(CR_TYPE_PATH,
                        "CrRectangle", &info, 0);
        }
        return type;
}

/**
 * cr_rectangle_new:
 * @parent: The parent canvas item.
 * @x: Center X coordinate in item units
 * @y: Center Y coordinate in item units
 * @width: Width in item units.
 * @height: Height in item units.
 *
 * A convenience constructor for creating a rectangle and adding it to an item
 * group in one step.
 *
 * Returns: A reference to a new CrItem.  You must call
 * g_object_ref if you intend to use this reference outside the local scope.
 */
CrItem * 
cr_rectangle_new(CrItem *parent,
                double x, double y, double width, double height,
                const gchar *first_arg_name, ...)
{
        CrItem *item;
        GArray *array;
        va_list args;

        va_start (args, first_arg_name);
        item = cr_item_construct(parent, CR_TYPE_RECTANGLE, first_arg_name, 
                        args);
        va_end (args);

        if (item) {
                g_object_set(item, "x", x, "y", y, "width", width, "height",
                                height, NULL);
        }

        return item;
}

