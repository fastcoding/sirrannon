#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <cr-canvas.h>
#include <cr-pixbuf.h>
#include <cr-text.h>
#include <cr-line.h>
#include <cr-rectangle.h>
#include <cr-ellipse.h>
#include <cr-vector.h>
#include <cr-zoomer.h>
#include <cr-panner.h>
#include <cr-rotator.h>
#include <cr-inverse.h>
#include <cr-arrow.h>

static gboolean
item_event(CrItem *item, GdkEvent *event, cairo_matrix_t *matrix, 
                CrItem *pick_item)
{
        static CrItem *last_item = NULL;
        static double init_x, init_y;
        static int last_msec = 0;

        switch (event->type) {
                case GDK_BUTTON_PRESS:
                        if (event->button.button == 1) {
                                last_item = item;
                                init_x = event->button.x;
                                init_y = event->button.y;
                                return TRUE;
                        }
                        break;
                case GDK_MOTION_NOTIFY:
                        if (last_item && event->motion.time - last_msec >= 100) 
                        {
                                cairo_matrix_translate(cr_item_get_matrix(item),
                                                event->motion.x - init_x,
                                                event->motion.y - init_y);
                                cr_item_request_update(item);
                                last_msec = event->motion.time;
                                return TRUE;
                        }
                        break;
                case GDK_BUTTON_RELEASE:
                        if (last_item) {
                                last_item = NULL;
                                return TRUE;
                        }
                        break;
        }

        return FALSE;
}


static void
add_pixbuf(CrItem *group)
{
        double x, y;
        CrItem *item;
        GdkPixbuf *pixbuf;
        GError *error;

        x = g_random_double_range(0, 380);
        y = g_random_double_range(0, 380);

        error = NULL;
        pixbuf = gdk_pixbuf_new_from_file("bug.png", &error);

        g_return_if_fail(pixbuf);

        item = cr_pixbuf_new(group, x, y, 
                        "pixbuf", pixbuf,
                        "scaleable", FALSE, 
                        "anchor", GTK_ANCHOR_CENTER, NULL);

        g_object_unref(pixbuf);

        g_signal_connect(item, "event", (GCallback) item_event, NULL);
}

static void
add_text(CrItem *group)
{
        double x, y;
        gboolean scaleable;
        const char *text;
        CrItem *item;

        x = g_random_double_range(0, 380);
        y = g_random_double_range(0, 380);
        scaleable = g_random_boolean();

        if (scaleable)
                text = "This text will <b>scale</b>.";
        else 
                text = "This text will <b>not</b>.";

        item = cr_text_new(group, x, y, text,
                        "scaleable", scaleable, 
                        "font", "Sans 12",
                        "anchor", GTK_ANCHOR_NW,
                        "use-markup", TRUE,
                        "fill_color_rgba", 0x000000ffL, NULL);

        g_signal_connect(item, "event", (GCallback) item_event, NULL);
}

static void
add_ellipse(CrItem *group)
{
        double x, y, width, height, orientation, tmp1, tmp2;
        GArray *array;
        CrItem *item;
        guint color;

        x = g_random_double_range(0, 360);
        y = g_random_double_range(0, 360);
        tmp1 = g_random_double_range(10, 200);
        tmp2 = g_random_double_range(10, 200);
        width = MAX(tmp1, tmp2);
        height = MIN(tmp1, tmp2);
        orientation = g_random_double_range(0, G_PI);
        color = g_random_int_range(0,255) << 24 |
                g_random_int_range(0,255) << 16 |
                g_random_int_range(0,255) << 8 |
                0xff;

        item = cr_ellipse_new(group, x, y, width, height, orientation,
                        "outline_color_rgba", 0x000000ff,
                        "line_scaleable", FALSE,
                        "line_width", 2.0,
                        "fill_color_rgba", color,
                        NULL);

        g_signal_connect(item, "event", (GCallback) item_event, NULL);
}

static void
add_rectangle(CrItem *group)
{
        double x, y, w, h;
        CrItem *item;
        guint color;

        x = g_random_double_range(0, 360);
        y = g_random_double_range(0, 360);
        w = g_random_double_range(10, 100);
        h = g_random_double_range(10, 100);
        color = g_random_int_range(0,255) << 24 |
                g_random_int_range(0,255) << 16 |
                g_random_int_range(0,255) << 8 |
                0xff;

        item = cr_rectangle_new(group, x, y, w, h,
                        "outline_color_rgba", 0x000000ff,
                        "line_scaleable", TRUE,
                        "line_width", 2.0,
                        "fill_color_rgba", color,
                        NULL);

        g_signal_connect(item, "event", (GCallback) item_event, NULL);
}

static void
add_line(CrItem *group)
{
        int i, total;
        double angle, dist, cx, cy, x, y;
        GArray *array;
        CrItem *line;
        guint color;
        cairo_pattern_t *pattern;

        array = g_array_new(FALSE, FALSE, sizeof(double));

        total = g_random_int_range(3, 30);

        cx = g_random_double_range(40, 360);
        cy = g_random_double_range(40, 360);

        for (i = 0, angle = 0; i < total; angle += 2. * M_PI/total, i++) {

                dist = g_random_double_range(10, 40);

                x = cx + dist * sin(angle);
                y = cy + dist * cos(angle);
                g_array_append_val(array, x);
                g_array_append_val(array, y);
        }

        line = cr_line_new(group, "array", array, 
                        "outline_color_rgba", 0x000000ff,
                        "line_scaleable", TRUE,
                        "line_width", 3.0,
                        NULL);

        if (g_random_boolean()) {

                color = g_random_int_range(0,255) << 24 |
                        g_random_int_range(0,255) << 16 |
                        g_random_int_range(0,255) << 8 |
                        0xff;

                g_object_set(line, "fill_color_rgba", color, NULL);
        }
        else {
                pattern = cairo_pattern_create_linear(0, 0, 
                                g_random_double_range(5, 20), 
                                g_random_double_range(5, 20));
                cairo_pattern_add_color_stop_rgb(pattern, 0, g_random_double(), 
                                g_random_double(), g_random_double());
                cairo_pattern_add_color_stop_rgb(pattern, 10, g_random_double(),
                                g_random_double(), g_random_double());
                cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
                g_object_set(line, "pattern", pattern, NULL);
                cairo_pattern_destroy(pattern);
        }

        g_signal_connect(line, "event", (GCallback) item_event, NULL);
}

static void
add_arrow(CrItem *group)
{
        double x, y, depth;
        gboolean scaleable;
        const char *color;
        int i;
        CrItem *item;
        GArray *array;

        array = g_array_new(FALSE, FALSE, sizeof(double));
        x = g_random_double_range(0, 380);
        y = g_random_double_range(0, 380);

        for (i = 0; i < 4; i++) {
                g_array_append_val(array, x);
                g_array_append_val(array, y);
                x += g_random_double_range(5, 50);
                y += g_random_double_range(5, 50);
        }
        item = cr_line_new(group, "array", array,
                        "outline_color", "darkgreen", "close", FALSE, NULL);

        cr_arrow_new(item, 0, NULL);
        cr_arrow_new(item, 1, NULL);
        cr_arrow_new(item, -2, NULL);
        cr_arrow_new(item, -1, NULL);

        g_signal_connect(item, "event", (GCallback) item_event, NULL);
}

static void
add_vector(CrItem *group)
{
        double x, y, x2, y2;
        GArray *array;
        CrItem *item;
        guint color;

        x = g_random_double_range(0, 360);
        y = g_random_double_range(0, 360);
        x2 = g_random_double_range(10, 100);
        y2 = g_random_double_range(10, 100);

        item = cr_vector_new(group, x, y, x2, y2,
                        "outline_color_rgba", 0x000000ff,
                        "end_scaleable", FALSE,
                        "line_scaleable", FALSE,
                        "line_width", 2.0,
                        NULL);

        cr_arrow_new(item, -1, NULL);
        cr_arrow_new(item, 0, NULL);

        g_signal_connect(item, "event", (GCallback) item_event, NULL);
}

static void
add_grab(CrItem *group)
{
        double x, y, w, h;
        GArray *array;
        CrItem *outlined_thing;

        /* This demonstrates how to use a rectangle (or any other path) 
         * to grab something that is normally hard to grab
         */

        x = g_random_double_range(0, 360);
        y = g_random_double_range(0, 360);
        w = g_random_double_range(20, 100);
        h = g_random_double_range(20, 100);

        /* normally you could grab this only by the outline which in this case
         * is specified to be one pixel wide. */
        outlined_thing = cr_rectangle_new(group, x, y, w, h,
                        "outline_color_rgba", 0x000000ff,
                        "line_scaleable", FALSE,
                        "line_width", 1.0,
                        NULL);

        /* An invisible grabbable path is added as a child of the item that is
         * difficult to grab. It is important to remember the x,y positions
         * of child items are referenced against their parent, so 0, 0 is 
         * used here.*/
        cr_rectangle_new(outlined_thing, 0, 0, w, h,
                        /* don't set fill or outline color and the item
                         * will not paint, but will have bounds. */
                        "test-fill", TRUE,
                        NULL);

        /* The event of the child item is pushed up to the parent since it is
         * not handled by the child item.*/
        g_signal_connect(outlined_thing, "event", (GCallback) item_event, NULL);
}

static void
on_zoom_in(GtkButton *button, CrCanvas *canvas)
{
        cr_canvas_zoom(canvas, 2, 2);
}

static void
on_zoom_out(GtkButton *button, CrCanvas *canvas)
{
        cr_canvas_zoom(canvas, .5, .5);
}

static void
do_remove(CrItem *child, CrItem *parent)
{
        cr_item_remove(parent, child);
}

static void
on_clear(GtkButton *button, CrItem *group)
{
        g_list_foreach(group->items, (GFunc) do_remove, group);
}

static GtkWidget *
create_extra_view(CrItem *top_group)
{
        GtkWidget *window, *canvas;

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title (GTK_WINDOW (window), "Slave View");
        g_signal_connect (G_OBJECT (window), "delete-event",
                        G_CALLBACK (gtk_main_quit), NULL);
        canvas = cr_canvas_new(NULL);
        cr_item_add(CR_CANVAS(canvas)->root, top_group);
        gtk_widget_set_size_request(canvas, 400,400);

        gtk_container_add (GTK_CONTAINER (window), canvas);
        gtk_widget_show_all(window);
        gtk_window_move(GTK_WINDOW(window), gdk_screen_width() - 
                        window->allocation.width, 0);
}

/*
static void
log_func(const gchar *log_domain, GLogLevelFlags log_level, const gchar
                                *message, gpointer user_data)
{
        g_print("XXXXXX %s\n", message);
}
*/

int 
main(int argc, char **argv)
{
        GtkWidget *window, *scrolled_window, *canvas, *vbox, *hbox, 
                  *hbox2, *label, *button;
        CrItem *top_group;
        CrZoomer *zoomer;
        CrPanner *panner;
        CrRotator *rotator;

        gtk_init(&argc, &argv);

        /* use for breakpoints:
        g_log_set_handler ("GLib-GObject", G_LOG_LEVEL_WARNING |
                        G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL |
                        G_LOG_FLAG_RECURSION, log_func, NULL);
        g_log_set_handler ("GLib", G_LOG_LEVEL_WARNING |
                        G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL |
                        G_LOG_FLAG_RECURSION, log_func, NULL);
                        */


        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title (GTK_WINDOW (window), "CR Custom Items Demo");
        g_signal_connect (G_OBJECT (window), "delete-event",
                        G_CALLBACK (gtk_main_quit), NULL);

        vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add (GTK_CONTAINER (window), vbox);

        canvas = cr_canvas_new("maintain_aspect", TRUE,
                        "auto_scale", TRUE,
                        "maintain_center", TRUE, NULL);
        cr_canvas_set_scroll_factor(CR_CANVAS(canvas), 3, 3);

        /* by using a canvas group item just below the root, we can open a
         * separate slave window that will not respond to the zooming operations
         * made on the main window. */
        top_group = g_object_new(CR_TYPE_ITEM, NULL);
        cr_item_add(CR_CANVAS(canvas)->root, top_group);

        scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scrolled_window), canvas);

        gtk_widget_set_size_request(canvas, 400,400);
        gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

        hbox = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Ellipse");
        g_signal_connect_swapped(button, "clicked", G_CALLBACK( add_ellipse),
                        top_group);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Rect");
        g_signal_connect_swapped(button, "clicked", G_CALLBACK( add_rectangle),
                        top_group);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Line");
        g_signal_connect_swapped(button, "clicked", G_CALLBACK( add_line),
                        top_group);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Text");
        g_signal_connect_swapped(button, "clicked", G_CALLBACK( add_text),
                        top_group);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Pixbuf");
        g_signal_connect_swapped(button, "clicked", G_CALLBACK( add_pixbuf),
                        top_group);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Vector");
        g_signal_connect_swapped(button, "clicked", G_CALLBACK( add_vector),
                        top_group);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Arrows");
        g_signal_connect_swapped(button, "clicked", G_CALLBACK( add_arrow),
                        top_group);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Grab");
        g_signal_connect_swapped(button, "clicked", G_CALLBACK( add_grab),
                        top_group);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

        hbox2 = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);

        zoomer = cr_zoomer_new(CR_CANVAS(canvas), "fill_color_rgba",
                        0xaa00aa77, "outline_color_rgba", 0xffffffff, NULL);
        button = gtk_button_new_with_label("Zoom Box");
        g_signal_connect_swapped(button, "clicked", 
                        G_CALLBACK(cr_zoomer_activate), zoomer);
        gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Zoom X2");
        g_signal_connect(button, "clicked", G_CALLBACK(on_zoom_in), canvas);
        gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Zoom /2");
        g_signal_connect(button, "clicked", G_CALLBACK(on_zoom_out), canvas);
        gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);

        panner = cr_panner_new(CR_CANVAS(canvas), "button", 1, NULL);
        button = gtk_button_new_with_label("Pan");
        g_signal_connect_swapped(button, "clicked", 
                        G_CALLBACK(cr_panner_activate), panner);
        gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);

        rotator = cr_rotator_new(CR_CANVAS(canvas), NULL);
        button = gtk_button_new_with_label("Rotate");
        gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
        g_signal_connect_swapped(button, "clicked", G_CALLBACK
                        (cr_rotator_activate), rotator);

        button = gtk_button_new_with_label("Clear");
        g_signal_connect(button, "clicked", G_CALLBACK( on_clear ), top_group);
        gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Quit");
        g_signal_connect(button, "clicked", G_CALLBACK( gtk_main_quit), NULL);
        gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);

        create_extra_view(top_group);

        g_object_unref(top_group);

        gtk_widget_show_all(window);

        gtk_main();
        return 0;
}







