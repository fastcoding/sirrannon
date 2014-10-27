#include <string.h>
#include <math.h>
#include <cairo.h>
#include "cr-item.h"
#include "unit-test.h"

static CrItemClass *parent_class;
static int _paint_count = 0;
static int _test_count = 0;
static int _use_parent;
static cairo_surface_t *_surface = NULL;

typedef struct {
        double x1, y1, x2, y2;
} InvalidateData;

static void
paint_count(CrItem *i, cairo_t *c)
{
        _paint_count++;
}
static CrItem * 
test_count(CrItem *i, cairo_t *c, double x, double y)
{
        _test_count++;
        return i;
}
static gboolean
calculate_bounds(CrItem *i, cairo_t *c, CrBounds *bounds,
                CrDeviceBounds *device)
{
        bounds->x1 = 100.;
        bounds->y1 = 100.;
        bounds->x2 = 200.;
        bounds->y2 = 200.;
        return TRUE;
}
static gboolean
calculate_bounds2(CrItem *i, cairo_t *c, CrBounds *bounds,
                CrDeviceBounds *device)
{
        bounds->x1 = 100.;
        bounds->y1 = 100.;
        bounds->x2 = 200.;
        bounds->y2 = 200.;
        device->anchor = GTK_ANCHOR_CENTER;
        device->x1 = -5;
        device->x2 = 5;
        device->y1 = -10;
        device->y2 = 10;
        return TRUE;
}
static void
invalidate(CrItem *i, int phase,
                double x1, double y1, double x2, double y2, 
                CrDeviceBounds *device,
                InvalidateData *data)
{
        data->x1 = x1;
        data->y1 = y1;
        data->x2 = x2;
        data->y2 = y2;
}

typedef struct _CrTestItem CrTestItem;
typedef struct _CrTestItemClass CrTestItemClass;

struct _CrTestItem 
{
        CrItem parent;
};
struct _CrTestItemClass
{
        CrItemClass parent_class;
};
static void 
cr_test_item_init(CrTestItem *item)
{
        _paint_count = _test_count = 0;
}
static void
finalize(GObject *object)
{
}
static void
cr_test_item_class_init(CrTestItemClass *klass)
{
        GObjectClass *object_class;
        CrItemClass *cr_item_class;

        object_class = (GObjectClass *) klass;
        cr_item_class = (CrItemClass *) klass;

        parent_class = g_type_class_peek_parent (klass);
        object_class->finalize = finalize;
        cr_item_class->paint = paint_count;
        cr_item_class->test = test_count;
}
GType cr_test_item_get_type(void)
{
        static GType type = 0;
        static const GTypeInfo info = {
                sizeof(CrTestItemClass),
                NULL, /*base_init*/
                NULL, /*base_finalize*/
                (GClassInitFunc) cr_test_item_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(CrTestItem),
                0,
                (GInstanceInitFunc) cr_test_item_init,
                NULL
        };
        if (!type) {
                type = g_type_register_static(CR_TYPE_ITEM,
                        "CrTestItem", &info, 0);
        }
        return type;
}

static void
setup(void)
{
        _surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 200, 200);
        _test_count = _paint_count = 0;
}

static void
teardown(void)
{
        cairo_surface_destroy(_surface);
}

TEST_BEGIN(item, setup, teardown)

TEST_NEW(signals_only_item)
{
        CrItem *item;
        cairo_t *ct;

        item = g_object_new(CR_TYPE_ITEM, NULL);
        ct = cairo_create(_surface);

        g_signal_connect(item, "paint", (GCallback)paint_count, NULL);
        g_signal_connect(item, "test", (GCallback)test_count, NULL);

        /* bounds are not defined - no paint unless all is selected. */
        cr_item_invoke_paint(item, ct, FALSE, -1000, -1000, 1000, 1000);
        TEST(_paint_count == 0);

        /* items without bounds only paint when all-TRUE */
        cr_item_invoke_paint(item, ct, TRUE, 0, 0, 0, 0);
        TEST(_paint_count == 1);

        cr_item_invoke_test(item, ct, 0, 0);

        TEST(_test_count == 1);

        cairo_destroy(ct);
}

TEST_NEW(show_and_hide)
{
        CrItem *item;
        cairo_t *ct;

        item = g_object_new(CR_TYPE_ITEM, NULL);
        ct = cairo_create(_surface);

        g_signal_connect(item, "paint", (GCallback)paint_count, NULL);
        g_signal_connect(item, "test", (GCallback)test_count, NULL);

        cr_item_hide(item);
        cr_item_invoke_paint(item, ct, TRUE, 0, 0, 0, 0);
        TEST(_paint_count == 0);
        cr_item_invoke_test(item, ct, 0, 0);
        TEST(_test_count == 0);

        g_object_set(item, "visible", TRUE, NULL);
        _test_count = _paint_count = 0;
        cr_item_invoke_paint(item, ct, TRUE, 0, 0, 0, 0);
        TEST(_paint_count == 1);
        cr_item_invoke_test(item, ct, 0, 0);
        TEST(_test_count == 1);

        cairo_destroy(ct);
}

TEST_NEW(overridden_item)
{
        CrItem *item;
        cairo_t *ct;

        item = g_object_new(cr_test_item_get_type(), NULL);
        ct = cairo_create(_surface);

        cr_item_invoke_paint(item, ct, TRUE, 0, 0, 0, 0);

        TEST(_paint_count == 1);

        cr_item_invoke_test(item, ct, 0, 0);

        TEST(_test_count == 1);
        cairo_destroy(ct);
}

TEST_NEW(matrix_operations)
{
        CrItem *item;
        cairo_t *c1, *c2;
        cairo_matrix_t *matrix;
        double x1, y1, x2, y2;

        item = g_object_new(CR_TYPE_ITEM, NULL);
        c1 = cairo_create(_surface);
        c2 = cairo_create(_surface);

        cairo_translate(c1, 10., -10.);
        cairo_scale(c1, 2., 4.);
        cairo_rotate(c1, M_PI / 2.);

        matrix = cr_item_get_matrix(item);
        cairo_matrix_translate(matrix, 10., -10.);
        cairo_matrix_scale(matrix, 2., 4.);
        cairo_matrix_rotate(matrix, M_PI / 2.);

        cairo_set_matrix(c2, item->matrix);

        x1 = x2 = y1 = y2 = 0.0;

        cairo_user_to_device(c1, &x1, &y1);
        cairo_user_to_device(c2, &x2, &y2);

        TESTFL(x1, x2);
        TESTFL(y1, y2);
        cairo_destroy(c1);
        cairo_destroy(c2);
}

TEST_NEW(bounds_calculation_and_painting)
{
        CrItem *item;
        cairo_t *ct;
        InvalidateData invdata;
        double px, py, x1, y1, x2, y2;
        memset(&invdata, 0, sizeof(invdata));
        item = g_object_new(cr_test_item_get_type(), NULL);

        g_signal_connect(item, "calculate_bounds", (GCallback)
                        calculate_bounds, NULL);
        g_signal_connect(item, "invalidate", (GCallback)
                        invalidate, &invdata);

        ct = cairo_create(_surface);

        /* this makes sure the item is up to date before we start.*/
        cr_item_report_old_bounds(item, ct, FALSE);
        cr_item_report_new_bounds(item, ct, FALSE);
        TEST((item->flags & CR_ITEM_NEED_UPDATE) == 0);

        /* now make a change to the item.*/
        cairo_matrix_scale(cr_item_get_matrix(item), 2, 2);
        cr_item_request_update(item);
        TEST((item->flags & CR_ITEM_NEED_UPDATE));

        TEST(cr_item_get_bounds(item, &x1, &y1, &x2, &y2));

        /* The first step of the update process is to report the old bounds 
         * through the invalidate signal */
        cr_item_report_old_bounds(item, ct, FALSE);
        TESTFL(invdata.x1, x1);
        TESTFL(invdata.y1, y1);
        TESTFL(invdata.x2, x2);
        TESTFL(invdata.y2, y2);
        memset(&invdata, 0, sizeof(invdata));

        /* The second step of the process is to report the new bounds. */
        cr_item_report_new_bounds(item, ct, FALSE);
        /* the item bounds are still the same, but now to cairo context is
         * set up for the transform conversion. */
        TESTFL(invdata.x1, x1*2);
        TESTFL(invdata.y1, y1*2);
        TESTFL(invdata.x2, x2*2);
        TESTFL(invdata.y2, y2*2);

        TEST(cr_item_get_bounds(item, &x1, &y1, &x2, &y2));
        TESTFL(invdata.x1/2, x1);
        TESTFL(invdata.y1/2, y1);
        TESTFL(invdata.x2/2, x2);
        TESTFL(invdata.y2/2, y2);

        /* painting out of bounds is ignored.*/
        cr_item_invoke_paint(item, ct, FALSE, 0, 0, 0, 0);
        TEST(_paint_count == 0);

        /* painting at least partially in bounds is allowed */
        cr_item_invoke_paint(item, ct, FALSE, 0, 0, x1*2+1, y1*2+1);
        TEST(_paint_count == 1);
}

TEST_NEW(bounds_with_device_coords)
{
        CrItem *item;
        cairo_t *ct;
        InvalidateData invdata;
        double px, py, x1, y1, x2, y2;
        double bx1, by1, bx2, by2;
        CrDeviceBounds device;
        memset(&invdata, 0, sizeof(invdata));
        item = g_object_new(cr_test_item_get_type(), NULL);

        g_signal_connect(item, "calculate_bounds", (GCallback)
                        calculate_bounds2, NULL);

        ct = cairo_create(_surface);

        /* this makes sure the item is up to date before we start.*/
        cr_item_report_old_bounds(item, ct, FALSE);
        cr_item_report_new_bounds(item, ct, FALSE);
        TEST((item->flags & CR_ITEM_NEED_UPDATE) == 0);

        /* now make a change to the item.*/
        cairo_matrix_scale(cr_item_get_matrix(item), .5, .5);
        cr_item_request_update(item);
        TEST((item->flags & CR_ITEM_NEED_UPDATE));

        /* standard */
        cr_item_report_old_bounds(item, ct, FALSE);
        cr_item_report_new_bounds(item, ct, FALSE);
        TEST(cr_item_get_bounds(item, &x1, &y1, &x2, &y2));
        TEST(cr_item_get_device_bounds(item, &device));

        TEST(device.anchor == GTK_ANCHOR_CENTER);
        TESTFL(device.x1, -5.);
        TESTFL(device.x2, 5.);
        TESTFL(device.y1, -10.);
        TESTFL(device.y2, 10.);

        /* The total rectangle is bounds + device bounds
         * I am only using the center anchor point.  There are other
         * possibilities, but this would make test more tedious. 
         * The cairo_t has no transform, but the item does.*/

        bx1 = x1/2 + device.x1;
        by1 = y1/2 + device.y1;
        bx2 = x2/2 + device.x2;
        by2 = y2/2 + device.y2;

        /* paint just outside the edge */
        cr_item_invoke_paint(item, ct, FALSE, bx1 - 20, by1 - 20, 
                        bx1 - 1, by1 - 1);
        TEST(_paint_count == 0);

        /* Now paint on the device edge to make sure it gets picked up.*/
        cr_item_invoke_paint(item, ct, FALSE, bx1, by1, bx1 + 1, by1 + 1);
        TEST(_paint_count == 1);

        /* point is just outside device edge */
        cr_item_invoke_test(item, ct, bx1-1, by1-1);
        TEST(_test_count == 0);

        /* The test method should also pick up a point right on the device
         * edge.*/
        cr_item_invoke_test(item, ct, bx1+1, by1+1);
        TEST(_test_count == 1);
}

TEST_END()




