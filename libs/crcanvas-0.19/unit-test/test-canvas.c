#include <gtk/gtk.h>
#include <string.h>
#include <cr-canvas.h>
#include "unit-test.h"

CrCanvas *_canvas;
CrItem *_root;
GtkWidget *_window;
double _x1, _y_1, _x2, _y2;

typedef struct {
        double x1, y1, x2, y2;
        GtkAnchorType anchor;
        double device_x1, device_y1, device_x2, device_y2;
        int paint_count;
} UpdateTestData;

static void
setup()
{
        _canvas = g_object_new(CR_TYPE_CANVAS, "repaint_mode", TRUE, NULL);

        _root = _canvas->root;

        _window = NULL;
}

static void 
teardown()
{
        if (_window) gtk_widget_destroy(_window);
        else if (_canvas) {
                gtk_widget_destroy(GTK_WIDGET(_canvas));
        }
}

static void
on_paint(CrItem *item, cairo_t *ct)
{
        GtkWidget *widget;

        widget = GTK_WIDGET(_canvas);
        _x2 = widget->allocation.width;
        _y2 = widget->allocation.height;

        _x1 = _y_1 = 0;
        cairo_device_to_user(ct, &_x1, &_y_1);
        cairo_device_to_user(ct, &_x2, &_y2);
}

static gboolean
update_test_calculate_bounds(CrItem *i, cairo_t *c,
                CrBounds *bounds,
                CrDeviceBounds *device,
                UpdateTestData *data)
{
        bounds->x1 = data->x1;
        bounds->y1 = data->y1;
        bounds->x2 = data->x2;
        bounds->y2 = data->y2;
        return TRUE;
}

static gboolean
update_test_calculate_bounds_device(CrItem *i, cairo_t *c,
                CrBounds *bounds,
                CrDeviceBounds *device,
                UpdateTestData *data)
{
        bounds->x1 = data->x1;
        bounds->y1 = data->y1;
        bounds->x2 = data->x2;
        bounds->y2 = data->y2;
        device->anchor = data->anchor;
        device->x1 = data->device_x1;
        device->y1 = data->device_y1;
        device->x2 = data->device_x2;
        device->y2 = data->device_y2;
        return TRUE;
}

static void
update_test_paint(CrItem *item, cairo_t *c, UpdateTestData *data)
{
        data->paint_count++;
}

static void
setup_window(gboolean show, int w, int h)
{
        _window = gtk_widget_new (GTK_TYPE_WINDOW, "type",
                                GTK_WINDOW_TOPLEVEL,
                                "width_request", w,
                                "height_request", h,
                                NULL);

        gtk_container_add (GTK_CONTAINER (_window),
                        GTK_WIDGET(_canvas));

        if (show) {
                gtk_widget_show_all(_window);
                TEST_EMPTY_QUEUE();
        }
}

typedef struct {
        int test_count, motion_count, enter_count, leave_count, press_count,
            release_count;
        double x, y;
        gboolean event_return;
} EventData;

static CrItem *
on_test(CrItem *item, cairo_t *c, double x, double y, EventData *data)
{
        data->test_count ++;
        printf("test -- x = %g, y = %g, data = (%g, %g)\n",x,y,data->x,data->y); 

        return fabs(data->x - x) < .01 && fabs(data->y - y) < .01 ? item : NULL;
}

static gboolean
on_event(CrItem *item, GdkEvent *event, cairo_matrix_t *matrix, 
                CrItem *pick_item, EventData *data)
{
        switch (event->type) {
                case GDK_BUTTON_PRESS:
                case GDK_2BUTTON_PRESS:
                case GDK_3BUTTON_PRESS:
                        data->press_count ++;
                        break;
                case GDK_BUTTON_RELEASE:
                        data->release_count ++;
                        break;
                case GDK_MOTION_NOTIFY:
                        data->motion_count ++;
                        break;
                case GDK_ENTER_NOTIFY:
                        data->enter_count ++;
                        break;
                case GDK_LEAVE_NOTIFY:
                        data->leave_count ++;
                        break;
        }
        g_print ("event %p, %d\n", item, event->type);
        gdk_event_get_coords(event, &data->x, &data->y);

        return data->event_return;
}

TEST_BEGIN(CrCanvas, setup, teardown)

TEST_NEW(scroll_factors)
{
        cr_canvas_set_scroll_factor(_canvas, 4, 8);

        TEST(_canvas->scroll_factor_x == 4);
        TEST(_canvas->scroll_factor_y == 8);
        TEST(!(_canvas->flags & CR_CANVAS_SCROLL_WORLD));

        TEST(_canvas->hadjustment->upper == 4);
        TEST(_canvas->vadjustment->upper == 8);
}

TEST_NEW(scroll_region)
{
        double page_h, page_v;

        setup_window(TRUE, 200, 200);

        TEST_EMPTY_QUEUE();

        cr_canvas_set_scroll_region(_canvas, -1000, -2000, 1000, 2000);

        TEST(_canvas->flags & CR_CANVAS_SCROLL_WORLD);

        TEST(_canvas->hadjustment->upper == 2000);
        TEST(_canvas->hadjustment->lower == 0);
        TEST(_canvas->vadjustment->upper == 4000);
        TEST(_canvas->vadjustment->lower == 0);

        page_h = _canvas->hadjustment->page_size;
        page_v = _canvas->vadjustment->page_size;

        /* scroll bar is initially in the center */
        TESTFL(_canvas->hadjustment->value, _canvas->hadjustment->upper / 2);
        TESTFL(_canvas->vadjustment->value, _canvas->vadjustment->upper / 2);

        /* verifies signal gets propagated to GtkAdjustment */
        cairo_matrix_scale(cr_item_get_matrix(CR_ITEM(_root)), 100, 100);
        cr_item_request_update(CR_ITEM(_root));
        TEST_EMPTY_QUEUE();
        TESTFL(_canvas->hadjustment->page_size, page_h);
        TESTFL(_canvas->vadjustment->page_size, page_v);
        TEST(_canvas->hadjustment->upper == 200000);
        TEST(_canvas->hadjustment->lower == 0);
        TEST(_canvas->vadjustment->upper == 400000);
        TEST(_canvas->vadjustment->lower == 0);

        /* verifies scroll area gets significantly shrunken as a result of the
         * scroll factor method. */
        cr_canvas_set_scroll_factor(_canvas, 3, 3);
        page_h = _canvas->hadjustment->page_size;
        page_v = _canvas->vadjustment->page_size;
        TESTFL(_canvas->hadjustment->upper - _canvas->hadjustment->lower,
                        page_h * 3);
        TESTFL(_canvas->vadjustment->upper - _canvas->vadjustment->lower,
                        page_v * 3);
        TEST(_canvas->hadjustment->value >= _canvas->hadjustment->lower &&
                        _canvas->hadjustment->value <=
                        _canvas->hadjustment->upper);
        TEST(_canvas->vadjustment->value >= _canvas->vadjustment->lower &&
                        _canvas->vadjustment->value <=
                        _canvas->vadjustment->upper);
}

TEST_NEW(maintain_center)
{
        double cx, cy, ncx, ncy;

        g_object_set(_canvas, "maintain_center", TRUE, NULL);
        cr_canvas_set_scroll_factor(_canvas, 4, 4);
        g_signal_connect(_root, "paint", (GCallback)on_paint, NULL);

        setup_window(TRUE, 200, 200);

        TEST_EMPTY_QUEUE();

        cx = _x1 + (_x2 - _x1) / 2;
        cy = _y_1 + (_y2 - _y_1) / 2;


        gtk_decorated_window_move_resize_window(_window, 0, 0, 400, 400);

        TEST_EMPTY_QUEUE();

        ncx = _x1 + (_x2 - _x1) / 2;
        ncy = _y_1 + (_y2 - _y_1) / 2;

        TESTFL(cx, ncx);
        TESTFL(cy, ncy);
}

TEST_NEW(auto_scale)
{
        double cx, cy, ncx, ncy, oldw, oldh, neww, newh;

        g_object_set(_canvas, "auto_scale", TRUE, "maintain_aspect", FALSE,
                        NULL);
        g_signal_connect(_root, "paint", (GCallback)on_paint, NULL);
        cr_canvas_set_scroll_factor(_canvas, 4, 4);

        setup_window(TRUE, 200, 200);

        TEST_EMPTY_QUEUE();

        oldw = _x2 - _x1;
        oldh = _y2 - _y_1;
        cx = _x1 + (oldw) / 2;
        cy = _y_1 + (oldh) / 2;


        gtk_decorated_window_move_resize_window(_window, 0, 0, 400, 300);

        TEST_EMPTY_QUEUE();

        neww = _x2 - _x1;
        newh = _y2 - _y_1;
        ncx = _x1 + (neww) / 2;
        ncy = _y_1 + (newh) / 2;

        TESTFL(neww, oldw);
        TESTFL(newh, oldh);
        TESTFL(cx, ncx);
        TESTFL(cy, ncy);
}

TEST_NEW(auto_scale_aspect)
{
        double cx, cy, ncx, ncy, oldw, oldh, neww, newh;

        g_object_set(_canvas, "auto_scale", TRUE, "maintain_center", TRUE,
                        "maintain_aspect", TRUE, NULL);

        g_signal_connect(_root, "paint", (GCallback)on_paint, NULL);
        cr_canvas_set_scroll_factor(_canvas, 4, 4);

        setup_window(TRUE, 200, 200);

        TEST_EMPTY_QUEUE();

        oldw = _x2 - _x1;
        oldh = _y2 - _y_1;
        cx = _x1 + (oldw) / 2;
        cy = _y_1 + (oldh) / 2;


        gtk_decorated_window_move_resize_window(_window, 0, 0, 400, 300);

        TEST_EMPTY_QUEUE();

        neww = _x2 - _x1;
        newh = _y2 - _y_1;
        ncx = _x1 + (neww) / 2;
        ncy = _y_1 + (newh) / 2;

        /* See cr-canvas.c show-less property
         * This test expects to show less on a resize to different aspect:
        TESTFL(neww, oldw);
        TEST(newh <= oldh + 50);
        */
        /* This test expects to show more on a resize to different aspect: */
        TEST(neww > oldw + 50);
        TESTFL(newh, oldh);

        TESTFL(cx, ncx);
        TESTFL(cy, ncy);
}

TEST_NEW(center_on)
{
        double cx, cy;

        cr_canvas_set_scroll_factor(_canvas, 4, 4);
        g_signal_connect(_root, "paint", (GCallback)on_paint, NULL);

        setup_window(TRUE, 200, 200);

        TEST_EMPTY_QUEUE();

        cx = _x2;
        cy = _y2;

        cr_canvas_center_on(_canvas, cx, cy);

        TEST_EMPTY_QUEUE();

        TESTFL(cx, _x1 + (_x2 - _x1)/2);
        TESTFL(cy, _y_1 + (_y2 - _y_1)/2);
}

TEST_NEW(center_scale)
{
        double cx, cy, w, h;

        cr_canvas_set_scroll_factor(_canvas, 4, 4);
        g_signal_connect(_root, "paint", (GCallback)on_paint, NULL);

        setup_window(TRUE, 200, 200);

        TEST_EMPTY_QUEUE();

        cx = _x1;
        cy = _y_1;
        w = 50;
        h = 50;

        cr_canvas_center_scale(_canvas, cx, cy, w, h);

        TEST_EMPTY_QUEUE();

        TESTFL(cx, _x1 + (_x2 - _x1)/2);
        TESTFL(cy, _y_1 + (_y2 - _y_1)/2);
        TESTFL(_x2 - _x1, w);
        TESTFL(_y2 - _y_1, h);

        cx = cy = w = h = 0;
        cr_canvas_get_center_scale(_canvas, &cx, &cy, &w, &h);
        TESTFL(cx, _x1 + (_x2 - _x1)/2);
        TESTFL(cy, _y_1 + (_y2 - _y_1)/2);
        TESTFL(_x2 - _x1, w);
        TESTFL(_y2 - _y_1, h);
}

/* Proves that the center and scale can be set-up before the widget is rendered
 * for the first time */
TEST_NEW(center_scale_before_realize)
{
        double cx, cy, w, h;

        cr_canvas_set_scroll_factor(_canvas, 4, 4);
        g_object_set(_canvas, "maintain_center", TRUE, "auto_scale", TRUE,NULL);
        g_signal_connect(_root, "paint", (GCallback)on_paint, NULL);

        setup_window(FALSE, 200, 200);

        cx = -20;
        cy = 10;
        w = 50;
        h = 50;

        /* note that this function will work prior ro realizing the widget only
         * if auto-scale is set to true.  This is because the canvas does not
         * know what size the widget will eventually be. */
        cr_canvas_center_scale(_canvas, cx, cy, w, h);

        gtk_widget_show_all(_window);

        TEST_EMPTY_QUEUE();

        TESTFL(cx, _x1 + (_x2 - _x1)/2);
        TESTFL(cy, _y_1 + (_y2 - _y_1)/2);
        TESTFL(_x2 - _x1, w);
        TESTFL(_y2 - _y_1, h);
}

TEST_NEW(zoom)
{
        double x, y, w, h;

        cr_canvas_set_scroll_factor(_canvas, 4, 4);
        g_signal_connect(_root, "paint", (GCallback)on_paint, NULL);

        setup_window(TRUE, 200, 200);

        TEST_EMPTY_QUEUE();

        w = _x2 - _x1;
        h = _y2 - _y_1;

        x = _x1;
        y = _y_1;

        /* zoom upper left to lower right.*/
        cr_canvas_zoom(_canvas, 2, 2);

        TEST_EMPTY_QUEUE();

        TESTFL(w/2, _x2 - _x1);
        TESTFL(h/2, _y2 - _y_1);
        TESTFL(x, _x1);
        TESTFL(y, _y_1);

        g_object_set(_canvas, "maintain_center", TRUE, NULL);

        x = _x1 + (_x2 - _x1)/2;
        y = _y_1 + (_y2 - _y_1)/2;

        /* zoom about the center point */
        cr_canvas_zoom(_canvas, .5, .5);

        TEST_EMPTY_QUEUE();

        TESTFL(w, _x2 - _x1);
        TESTFL(h, _y2 - _y_1);
        TESTFL(x, _x1 + (_x2 - _x1)/2);
        TESTFL(y, _y_1 + (_y2 - _y_1)/2);
}

TEST_NEW(item_events)
{
        CrItem *item;
        GdkEvent *event;
        EventData data;

        item = g_object_new(CR_TYPE_ITEM, NULL);
        memset(&data, 0, sizeof(data));
        data.x = data.y = 200;
        data.event_return = TRUE;

        cairo_matrix_scale(cr_item_get_matrix(item), 2, 2);
        cr_item_request_update(item);

        g_signal_connect(item, "test", (GCallback)on_test, &data);
        g_signal_connect(item, "event", (GCallback)on_event, &data);
        g_signal_connect(_root, "event", (GCallback)on_event, &data);

        cr_item_add(_root, item);

        setup_window(TRUE, 200, 200);

        event = gdk_event_new(GDK_MOTION_NOTIFY);
        gdk_event_set_screen(event, gdk_screen_get_default());
        event->type = GDK_MOTION_NOTIFY;
        event->motion.window = GTK_WIDGET(_canvas)->window;
        event->motion.x = event->motion.y = 50;


        /* pointer not on item so no event.*/

        // or gtk_main_do_event (same thing)
        gtk_widget_event(GTK_WIDGET(_canvas), event);
        TEST(data.test_count == 1);
        TEST(data.motion_count == 0);
        TEST(data.enter_count == 0);

        /* pointer on item so we get an ENTER event.*/
        event->motion.x = event->motion.y = 400;

        gtk_widget_event(GTK_WIDGET(_canvas), event);
        TEST(data.test_count == 2);
        TEST(data.motion_count == 0);
        TEST(data.enter_count == 1);
        TESTFL(data.x, 200.);
        TESTFL(data.y, 200.);

        /* press button - we should get a button press event. */
        event->type = GDK_BUTTON_PRESS;
        event->button.button = 1;
        gtk_widget_event(GTK_WIDGET(_canvas), event);
        TEST(data.press_count == 1);

        /* move cursor */
        event->type = GDK_MOTION_NOTIFY;
        gtk_widget_event(GTK_WIDGET(_canvas), event);
        TEST(data.motion_count == 1);

        /* release button */
        event->type = GDK_BUTTON_RELEASE;
        gtk_widget_event(GTK_WIDGET(_canvas), event);
        TEST(data.release_count == 1);

        /* move cursor - expect a leave event */
        event->motion.x = event->motion.y = 25;
        event->type = GDK_MOTION_NOTIFY;
        gtk_widget_event(GTK_WIDGET(_canvas), event);
        TEST(data.leave_count == 1);


        /* event return is false so event gets propagated to root item. */
        /* this means that event count will increase by two. */
        event->motion.x = event->motion.y = 400;
        data.event_return = FALSE;
        data.enter_count = data.leave_count = data.test_count = 0;
        data.x = data.y = 200;
        
        gtk_widget_event(GTK_WIDGET(_canvas), event);
        TEST(data.test_count == 1);
        TEST(data.enter_count == 2);

        /* the item scaling is not included here since it was not applied to the
         * root item. */
        TESTFL(data.x, 400.);
        TESTFL(data.y, 400.);
        data.event_return = TRUE;

        gdk_event_free(event);
}

TEST_NEW(limited_expose)
{
        CrItem *item1, *item2, *item3;
        UpdateTestData data1, data2, data3;
        GdkRectangle rect;

        g_object_set(_canvas, "repaint_mode", FALSE, NULL);
        item1 = g_object_new(CR_TYPE_ITEM, NULL);
        item2 = g_object_new(CR_TYPE_ITEM, NULL);
        item3 = g_object_new(CR_TYPE_ITEM, NULL);
        memset(&data1, 0, sizeof(data1));
        memset(&data2, 0, sizeof(data2));
        memset(&data3, 0, sizeof(data3));

        data1.x1 = 0;
        data1.y1 = 0;
        data1.x2 = 20;
        data1.y2 = 20;

        data2.x1 = 40;
        data2.y1 = 40;
        data2.x2 = 60;
        data2.y2 = 60;

        data3.x1 = data3.y1 = data3.x2 = data3.y2 = 80;
        data3.anchor = GTK_ANCHOR_NW;
        data3.device_x1 = data3.device_y1 = 0;
        data3.device_x2 = 100;
        data3.device_y2 = 20;

        g_signal_connect(item1, "calculate_bounds", (GCallback)
                        update_test_calculate_bounds, &data1);
        g_signal_connect(item1, "paint", (GCallback)
                        update_test_paint, &data1);

        g_signal_connect(item2, "calculate_bounds", (GCallback)
                        update_test_calculate_bounds, &data2);
        g_signal_connect(item2, "paint", (GCallback)
                        update_test_paint, &data2);

        /* Device coordinates, but no item coords.*/
        g_signal_connect(item3, "calculate_bounds", (GCallback)
                        update_test_calculate_bounds_device, &data3);
        g_signal_connect(item3, "paint", (GCallback)
                        update_test_paint, &data3);

        cr_item_add(_root, item1);
        g_object_unref(item1);
        cr_item_add(_root, item2);
        g_object_unref(item2);
        cr_item_add(_root, item3);
        g_object_unref(item3);
        setup_window(TRUE, 200, 200);

        TEST_EMPTY_QUEUE();

        TEST(data1.paint_count == 1);
        TEST(data2.paint_count == 1);
        TEST(data3.paint_count == 1);

        cr_item_request_update(item1);

        TEST_EMPTY_QUEUE();

        /* paints only the item that requested an update.*/
        TEST(data1.paint_count == 2);
        TEST(data2.paint_count == 1);
        TEST(data3.paint_count == 1);

        /* dirty one of the items.  only this one should re-paint.*/
        rect.x = rect.y = 45;
        rect.width = rect.height = 10;
        gdk_window_invalidate_rect(GTK_WIDGET(_canvas)->window, &rect, FALSE);
        TEST_EMPTY_QUEUE();
        TEST(data1.paint_count == 2);
        TEST(data2.paint_count == 2);
        TEST(data3.paint_count == 1);

        /* Test the item that has device-only width and height */

        /* no-paint outside bounds. */
        rect.x = rect.y = data3.x1 - 5;
        rect.width = rect.height = 4;
        gdk_window_invalidate_rect(GTK_WIDGET(_canvas)->window, &rect, FALSE);
        TEST_EMPTY_QUEUE();
        TEST(data3.paint_count == 1);

        /* paint inside bounds. */
        rect.x = rect.y = data3.x1;
        rect.width = rect.height = 4;
        gdk_window_invalidate_rect(GTK_WIDGET(_canvas)->window, &rect, FALSE);
        TEST_EMPTY_QUEUE();
        TEST(data3.paint_count == 2);
#if 0
        gdk_window_scroll(GTK_WIDGET(_canvas)->window, -10, -10);
        /* this part fails. I can't predict what gdk_window_scroll
         * will invalidate.*/

        /* move the scroll bar and verify that nothing get painted. */
        cr_canvas_scroll_to(_canvas, 10, 10);
        TEST_EMPTY_QUEUE();
        TEST(data1.paint_count == 2);
        TEST(data2.paint_count == 2);

        /*move it back.  data1 should get painted.*/
        cr_canvas_scroll_to(_canvas, 0, 0);
        TEST_EMPTY_QUEUE();
        TEST(data1.paint_count == 3);
        TEST(data2.paint_count == 2);
#endif
}

TEST_END()

