#include "cr-item.h"
#include <string.h>
#include "unit-test.h"

CrItem *_root, *_level1, *_level2;
CrItem *_item0, *_item1, *_item2;
static int _need_update_count = 0;
static int _invalidate_count = 0;
static int _remove_count = 0;
static int _add_count = 0;

static int _test_count = 0;
static CrItem *_test_order[5];
static CrItem *_test_item = NULL;

static CrItem *_event_item = NULL;
static CrItem *_event_order[5];
static int _event_count = 0;
static cairo_matrix_t _matrix;

static CrItem *
on_test(CrItem *i, cairo_t *c, double x, double y)
{
        g_assert(_test_count < 5);

        _test_order[_test_count] = i;

        _test_count++;

        if (i == _test_item) return i;

        return NULL;
}

static gboolean 
on_event(CrItem *i, GdkEvent *e)
{
        g_assert(_event_count < 5);

        _event_order[_event_count] = i;

        _event_count++;

        if (i == _event_item) return TRUE;

        return FALSE;
}

static void
setup()
{
        _root = g_object_new(CR_TYPE_ITEM, NULL);
        _level1 = g_object_new(CR_TYPE_ITEM, NULL);
        _level2 = g_object_new(CR_TYPE_ITEM, NULL);

        _item0 = g_object_new(CR_TYPE_ITEM, NULL);
        _item1 = g_object_new(CR_TYPE_ITEM, NULL);
        _item2 = g_object_new(CR_TYPE_ITEM, NULL);

        /*
                         root
                        /   \
                   item0     level1
                             /    \
                        item1      level2
                                  /    
                             item2
        */

        // test is wrong: root,item2,item1, item0:
        // need this test case explicitly.

        cr_item_add(_root, _item0);
        g_object_unref(_item0);
        cr_item_add(_root, CR_ITEM(_level1));
        g_object_unref(_level1);

        cr_item_add(_level1, _item1);
        g_object_unref(_item1);
        cr_item_add(_level1, CR_ITEM(_level2));
        g_object_unref(_level2);
        
        cr_item_add(_level2, _item2);
        g_object_unref(_item2);

        _invalidate_count = _need_update_count = _add_count = _remove_count = 0;
        cairo_matrix_init_identity(&_matrix);
}

static void
teardown()
{
        if (_root) {
                g_object_unref(_root);
        }
        _root = _level1 = _level2 = NULL;
        _item0 = _item1 = _item2 = NULL;
}

static void 
need_update_count(CrItem *i)
{
        _need_update_count++;
}
static void 
add_count(CrItem *i)
{
        _add_count++;
}
static void 
remove_count(CrItem *i)
{
        _remove_count++;
}

static void
invalidate_count(CrItem *i)
{
        _invalidate_count++;
}


TEST_BEGIN(test-group, setup, teardown)

TEST_NEW(reference_counting)
{
        GObject *object;
        
        object = g_object_new(CR_TYPE_ITEM, NULL);
        cr_item_add(_root, CR_ITEM(object));
        TEST(object->ref_count == 2);
        cr_item_remove(_root, CR_ITEM(object));
        TEST(object->ref_count == 1);
        g_object_unref(object);

        object = G_OBJECT(_item2);

        g_object_ref(object);
        TEST(object->ref_count == 2);

        /* The whole canvas hierarchy goes away */
        g_object_unref(_root);
        _root = NULL;

        /* verify this object is still alive since I hold a reference.*/
        TEST(object->ref_count == 1);
        g_object_unref(object);
}

TEST_NEW(add_remove_signal)
{
        CrItem *item;

        item = g_object_new(CR_TYPE_ITEM, NULL);

        /* 'added' and 'removed' are unusual signals in that they propagate up
         * the canvas tree through the root item.  So we should expect that a
         * change to _level2 will generate a signal at _root. */

        g_signal_connect(_root, "added", (GCallback)add_count, NULL);
        g_signal_connect(_root, "removed", (GCallback)remove_count, NULL);

        cr_item_add(_level2, item);
        g_object_unref(item);

        TEST(_add_count == 1);
        TEST(_remove_count == 0);

        cr_item_remove(_level2, item);

        TEST(_add_count == 1);
        TEST(_remove_count == 1);
}

TEST_NEW(remove_from_top_of_tree_and_signal_disconnect)
{
        g_signal_connect(_level2, "request_update",
                        (GCallback)need_update_count, NULL);
        g_signal_connect(_root, "invalidate",
                        (GCallback)invalidate_count, NULL);

        g_object_ref(_item2);
        /* make it think it has bounds to report */
        _item2->bounds = cr_bounds_new();

        cr_item_remove(_root, _item2);
        TEST(G_OBJECT(_item2)->ref_count == 1);

        /* should trigger an update because an item was removed. */
        TEST(_invalidate_count == 1);

        /* A request to a disconnected item should not trigger an 
         * update or invalidate. */
        cr_item_request_update(_item2);
        TEST(_need_update_count == 0);

        g_object_unref(_item2);
}


TEST_NEW(test_method_z_order_and_propagation)
{
        CrItem *item;
        cairo_surface_t *surface;
        cairo_t *ct;

        surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,100,100);
        ct = cairo_create(surface);

        _test_count = 0;
        _test_item = NULL;
        memset(_test_order, 0, 5 * sizeof(CrItem *));

        g_signal_connect(_item0, "test", (GCallback)on_test, NULL);
        g_signal_connect(_item1, "test", (GCallback)on_test, NULL);
        g_signal_connect(_item2, "test", (GCallback)on_test, NULL);

        item = cr_item_invoke_test(CR_ITEM(_root), ct, 0, 0);
        TEST(_test_order[0] == _item2);
        TEST(_test_order[1] == _item1);
        TEST(_test_order[2] == _item0);
        TEST(_test_count == 3);

        _test_count = 0;

        _test_item = _item1;

        item = cr_item_invoke_test(CR_ITEM(_root), ct, 0, 0);
        TEST(_test_count == 2);
        TEST(item == _test_item);
        cairo_destroy(ct);
        cairo_surface_destroy(surface);
}

TEST_NEW(item_event_selection_and_z_order)
{
        GdkEvent event;

        _event_count = 0;
        _event_item = NULL;
        memset(_event_order, 0, 5 * sizeof(CrItem *));

        g_signal_connect(_item1, "event", (GCallback)on_event, FALSE);
        g_signal_connect(_root, "event", (GCallback)on_event, FALSE);
        g_signal_connect(_level1, "event", (GCallback)on_event, FALSE);
        g_signal_connect(_level2, "event", (GCallback)on_event, FALSE);

        cr_item_invoke_event(_item1, &event, &_matrix, _item1);

        TEST(_event_count == 3);
        TEST(_event_order[0] == _item1);
        TEST(_event_order[1] == CR_ITEM(_level1));
        TEST(_event_order[2] == CR_ITEM(_root));

        _event_count = 0;
        _event_item = _item1;

        cr_item_invoke_event(_item1, &event, &_matrix, _item1);

        TEST(_event_count == 1);
}

TEST_END()
