#include <time.h>
#include <libcrcanvas.h>
#include <gtk/gtk.h>

#define WIDTH 200
#define HEIGHT 200
#define EPS 5.0
#define PATH_LENGTH 2000

/*
 * BUGS:
 *
 * On some hardware, this does not work properly and in some cases
 * causes the window manager to crash.  On other hardware it works perfectly.
 */

CrItem *_blit, *_no_blit;

static CrPoints *
create_walk(double *x1, double *y1, double *x2, double *y2)
{
        CrPoints *points;
        GArray *array;
        double x, y;
        int i;

        *x1 = *y1 = G_MAXDOUBLE;
        *x2 = *y2 = -G_MAXDOUBLE;

        points = cr_points_new();
        array = points->array;

        g_random_set_seed(time(0));
        x = WIDTH / 2.;
        g_array_append_val(array, x);
        y = HEIGHT / 2.;
        g_array_append_val(array, y);

        for (i = 0; i < PATH_LENGTH; i++) {
                x += g_random_boolean() ? EPS : -EPS;
                *x1 = MIN(x, *x1);
                *x2 = MAX(x, *x2);
                g_array_append_val(array, x);
                y += g_random_boolean() ? EPS : -EPS;
                *y1 = MIN(y, *y1);
                *y2 = MAX(y, *y2);
                g_array_append_val(array, y);

        }
        return points;
}

static gboolean
on_timeout(CrLine *line)
{
        CrPoints *points;
        GArray *walk;
        int next_index, amount;
        double x, y;

        /* a trail of where its been */
        walk = (GArray*) g_object_get_data(G_OBJECT(line), "walk");
        next_index = GPOINTER_TO_INT(g_object_get_data(
                                G_OBJECT(line), "next_index"));

        g_object_get(line, "points", &points, NULL);
        cr_points_ref(points);

        if (next_index >= walk->len) {
                next_index = 0;
        }
        else {
                amount = points->array->len-18;
                if (amount > 0)
                        g_array_remove_range(points->array, 0, amount);
        }

        x = g_array_index(walk, double, next_index);
        next_index++;
        y = g_array_index(walk, double, next_index);
        next_index++;
        g_object_set_data(G_OBJECT(line), "next_index", 
                        GINT_TO_POINTER(next_index));
        g_array_append_val(points->array, x);
        g_array_append_val(points->array, y);
        g_object_set(line, "points", points, NULL);
        cr_points_unref(points);
        return TRUE;
}

static void 
setup_tracer(CrItem *group, GArray *walk)
{
        CrPoints *points;
        CrItem *line;

        points = cr_points_new();
        line = cr_line_new(group, "points", points, "outline-color", "yellow",
                        "close", FALSE, NULL);
        cr_arrow_new(line, -1, NULL);
        g_object_set(CR_ITEM(line)->items->data,"visible", 1, NULL);
        g_object_set_data(G_OBJECT(line), "walk", walk);
        g_object_set_data(G_OBJECT(line), "next_index", 0);
        g_timeout_add(200, (GSourceFunc) on_timeout, line);
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
on_use_blit(GtkToggleButton *button, CrCanvas *canvas)
{
        if (gtk_toggle_button_get_active(button)) {
                cr_item_show(_blit);
                cr_item_hide(_no_blit);
        }
        else {
                cr_item_show(_no_blit);
                cr_item_hide(_blit);
        }
}

int 
main(int argc, char **argv)
{
        GtkWidget *window, *scrolled_window, *canvas, *vbox,
                  *hbox2, *label, *button, *textview;
        CrItem *walk;
        CrZoomer *zoomer;
        CrPanner *panner;
        CrRotator *rotator;
        CrPoints *points;
        GtkTooltips *tooltips;
        GtkTextBuffer *buffer;
        double minx, miny, maxx, maxy;

        gtk_init(&argc, &argv);

        tooltips = gtk_tooltips_new ();
        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title (GTK_WINDOW (window), "CR Blit Demo");
        g_signal_connect (G_OBJECT (window), "delete-event",
                        G_CALLBACK (gtk_main_quit), NULL);

        vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add (GTK_CONTAINER (window), vbox);

        canvas = cr_canvas_new("maintain_aspect", TRUE,
                        "auto_scale", TRUE,
                        "maintain_center", TRUE, NULL);
        cr_canvas_set_scroll_factor(CR_CANVAS(canvas), 3, 3);

        scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scrolled_window), canvas);

        gtk_widget_set_size_request(canvas, WIDTH,HEIGHT);
        gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

        buffer = gtk_text_buffer_new(NULL);
        gtk_text_buffer_set_text(buffer,
                "Using the CrBlit grouping item should provide a noticeable"
                " improvement in CPU performance because it stores a the "
                " most complex portion of the item tree into an offscreen "
                "image buffer.", -1);
        textview = gtk_text_view_new_with_buffer(buffer);
        g_object_set(textview, "wrap-mode", GTK_WRAP_WORD, NULL);
        gtk_box_pack_start (GTK_BOX (vbox), textview, FALSE, FALSE, 0);

        button = gtk_check_button_new_with_label("Use CrBlit group for walk.");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
        gtk_tooltips_set_tip (tooltips, button, 
                "Using the CrBlit item is the same as creating an off-screen "
                "image and rendering it with CrPixbuf except that it does "
                "all of the book-keeping for you.", NULL);
        gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
        g_signal_connect(button, "toggled", G_CALLBACK( on_use_blit ), canvas);

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


        button = gtk_button_new_with_label("Quit");
        g_signal_connect(button, "clicked", G_CALLBACK( gtk_main_quit), NULL);
        gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);



        _blit = cr_blit_new(CR_CANVAS(canvas)->root,
                        "canvas", canvas, NULL);
                        //"device-width", 400.,
                        //"device-height", 400., NULL);

        _no_blit = cr_item_new(CR_CANVAS(canvas)->root, CR_TYPE_ITEM,
                        "visible", FALSE, NULL);

        points = create_walk(&minx, &miny, &maxx, &maxy);
        walk = g_object_new(CR_TYPE_LINE, "points", points,
                        "outline_color", "gray",
                        "line-width", 5.0,
                        "close", FALSE,
                        NULL);
        cr_item_add(_blit, walk);
        cr_item_add(_no_blit, walk);
        g_object_unref(walk);

        cr_canvas_set_viewport(CR_CANVAS(canvas), minx, miny, maxx, maxy);

        setup_tracer(CR_CANVAS(canvas)->root, points->array);

        gtk_widget_show_all(window);

        gtk_main();
        return 0;
}


