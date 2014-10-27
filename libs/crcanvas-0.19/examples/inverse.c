#include <math.h>
#include <gtk/gtk.h>
#include <cr-canvas.h>
#include <cr-text.h>
#include <cr-line.h>
#include <cr-vector.h>
#include <cr-arrow.h>
#include <cr-inverse.h>
#include <cr-rectangle.h>
#include <cr-rotator.h>

static void
make_items(CrItem *group) 
{
        GArray *array;
        CrItem *inverse, *item;
        double vals[] = {-7., 0., 
                -150., 0., 
                -150., -50.,
                -250., -50., 
                -50., -50. };

        cr_rectangle_new(group, 0, 0, 100, 100, "fill_color", "slate blue",
                        "outline-color", "black", NULL);
        cr_vector_new(group, -50, -50, 100, 100, 
                        "outline-color", "black", NULL);
        cr_vector_new(group, 50, -50, -100, 100, 
                        "outline-color", "black", NULL);

        /* Inverse #1 - default mode - reverses both scaling and rotation */
        inverse = cr_inverse_new(group, -50, -50, NULL);

        array = g_array_new(0, 0, sizeof(double));
        g_array_append_vals(array, vals, 10);

        item = cr_line_new(inverse, "array", array, 
                        "outline-color", "black", 
                        "close", FALSE,
                        NULL);
        cr_arrow_new(item, 0, NULL);

        cr_text_new(inverse, -150, -50, "This label will not scale or rotate.",
                        "anchor", GTK_ANCHOR_S, 
                        "width", 200., NULL);

        /* Inverse #2 - rotates but does not scale. */
        inverse = cr_inverse_new(group, 50, 50, "preserve-rotation", TRUE,
                        NULL);
        item = cr_vector_new(inverse, 4, 4, 46, 46, "outline-color", 
                        "black", NULL);
        cr_arrow_new(item, 0, NULL);
        cr_text_new(inverse, 50, 50, "This label will <b>rotate</b>.\n"
                        "It will <b>not</b> scale.",
                        "fill-color", "black",
                        "use-markup", TRUE, NULL); 

        /* Inverse #3 - scales but does not rotate. */
        inverse = cr_inverse_new(group, 50, -50, "preserve-scale", TRUE,
                        NULL);
        item = cr_vector_new(inverse, 4, -4, 46, -46, "outline-color",
                        "black", NULL);
        cr_arrow_new(item, 0, NULL);
        cr_text_new(inverse, 50, -50, "This label will <b>scale</b>.\n"
                        "It will <b>not</b> rotate.",
                        "fill-color", "black",
                        "use-markup", TRUE,
                        "anchor", GTK_ANCHOR_SW, NULL);
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

int 
main(int argc, char **argv)
{
        GtkWidget *window, *scrolled_window, *canvas, *vbox, *hbox, 
                  *button;
        CrRotator *rotator;

        gtk_init(&argc, &argv);

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title (GTK_WINDOW (window), "CR Inverse Item Demo");
        g_signal_connect (G_OBJECT (window), "delete-event",
                        G_CALLBACK (gtk_main_quit), NULL);

        vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add (GTK_CONTAINER (window), vbox);

        canvas = cr_canvas_new("maintain_center", TRUE, NULL);
        cr_canvas_set_scroll_region(CR_CANVAS(canvas), -600, -600, 600, 600);
        cr_canvas_center_on(CR_CANVAS(canvas), 0, 0);

        make_items(CR_CANVAS(canvas)->root);

        scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scrolled_window), canvas);

        gtk_widget_set_size_request(canvas, 650, 650);
        gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

        hbox = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Zoom X2");
        g_signal_connect(button, "clicked", G_CALLBACK(on_zoom_in), canvas);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Zoom /2");
        g_signal_connect(button, "clicked", G_CALLBACK(on_zoom_out), canvas);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

        rotator = cr_rotator_new(CR_CANVAS(canvas), NULL);
        button = gtk_button_new_with_label("Rotate");
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
        g_signal_connect_swapped(button, "clicked", G_CALLBACK
                        (cr_rotator_activate), rotator);

        button = gtk_button_new_with_label("Quit");
        g_signal_connect(button, "clicked", G_CALLBACK( gtk_main_quit), NULL);
        gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

        gtk_widget_show_all(window);

        gtk_main();
        return 0;
}




