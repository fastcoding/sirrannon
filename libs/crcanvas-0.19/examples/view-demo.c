/* view-demo.c
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
#include <gtk/gtk.h>
#include <cr-canvas.h>
#include <cr-item.h>
#include <cr-zoomer.h>
#include <cr-panner.h>
#include <cr-rotator.h>

static void
draw(cairo_t *ct)
{
        int i;
        char text[16];

        cairo_new_path(ct);

        /* plotting grid */
        cairo_set_source_rgb(ct, .75, .75, .75);
        cairo_set_line_width(ct, 1);
        for (i = -1000; i <= 1000; i += 100) {
                cairo_move_to(ct, -1000, i);
                cairo_line_to(ct, 1000, i);
                cairo_move_to(ct, i, -1000);
                cairo_line_to(ct, i, 1000);
        }
        cairo_stroke(ct);

        /* X/Y axis. */
        cairo_set_line_width(ct, 2);
        cairo_set_source_rgb(ct, .75, .75, .75);
        cairo_move_to(ct, -1000, 0);
        cairo_line_to(ct, 1000, 0);
        cairo_move_to(ct, 0, -1000);
        cairo_line_to(ct, 0, 1000);

        /* tick marks and labels */
        cairo_set_font_size (ct, 20);
        for (i = -1000; i <= 1000; i += 10) {
                if (i % 100 == 0) {
                        cairo_move_to(ct, i, -10);
                        sprintf(text, "%d", i);
                        cairo_show_text(ct, text);
                        cairo_move_to(ct, i, -10);
                        cairo_line_to(ct, i, 10);
                        cairo_move_to(ct, -10, i);
                        cairo_line_to(ct, 10, i);
                        cairo_show_text(ct, text);
                }
                else {
                        cairo_set_line_width(ct, 2);
                        cairo_move_to(ct, i, -5);
                        cairo_line_to(ct, i, 5);
                        cairo_move_to(ct, -5, i);
                        cairo_line_to(ct, 5, i);
                }
        }

        cairo_stroke(ct);


        /* some circles centered about the origin */
        for (i = 50; i <= 800; i += 300) {
                cairo_set_source_rgb(ct, 1, 0, 0);
                cairo_arc(ct, 0, 0, i, 0, 2 * M_PI);
                cairo_stroke(ct);
                cairo_set_source_rgb(ct, 0, 1, 0);
                cairo_arc(ct, 0, 0, i + 100, 0, 2 * M_PI);
                cairo_stroke(ct);
                cairo_set_source_rgb(ct, 0, 0, 1);
                cairo_arc(ct, 0, 0, i + 200, 0, 2 * M_PI);
                cairo_stroke(ct);
        }
}

/*
 * This is just as slow as drawing every time.
static cairo_surface_t *surf = NULL;
static void
build_surf(cairo_t *ct)
{
        cairo_t *cr;

        surf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 1000, 1000);
        cr = cairo_create(surf);
        cairo_scale(cr, .5, .5);
        cairo_translate(cr, 1000, 1000);
        draw(cr);
        cairo_destroy(cr);
}
*/

static void
paint(CrItem *item, cairo_t *ct)
{

        cairo_save(ct);

        /*
        if (!surf) build_surf(ct);
        cairo_set_source_surface(ct, surf, -500, -500);
        cairo_paint(ct);
        */
        draw(ct);

        cairo_restore(ct);
}

static void
on_set_property(GtkButton *button, CrCanvas *canvas)
{
        const gchar *label;
        gboolean active;
        GSList *none_list;

        label = gtk_button_get_label(button);

        if (!GTK_IS_TOGGLE_BUTTON(button)) {
                g_object_set(canvas,
                                "maintain_center", FALSE,
                                "auto_scale", FALSE,
                                "maintain_aspect", FALSE,
                                NULL);
                for (none_list = 
                        g_object_get_data(G_OBJECT(button), "none_list");
                        none_list; none_list = g_slist_next(none_list)) {

                        gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(
                                                none_list->data), FALSE);
                }
                return;
        }

        active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

        if (g_strrstr(label, "Center")) {
                g_object_set(canvas, "maintain_center", active, NULL);
        }
        else if (g_strrstr(label, "Scale")) {
                g_object_set(canvas, "auto_scale", active, NULL);
        }
        else if (g_strrstr(label, "Aspect"))
                g_object_set(canvas, 
                                "maintain_aspect", active,
                                NULL);
}

static void
on_set_scroll_factor(GtkButton *button, CrCanvas *canvas)
{
        gboolean active;

        active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
        if (active) {
                cr_canvas_set_scroll_factor(canvas, 10, 10);
                cr_canvas_center_on(canvas, 0, 0);
        }
}

static void
on_set_scroll_region(GtkButton *button, CrCanvas *canvas)
{
        gboolean active;

        active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
        if (active) {
                cr_canvas_set_scroll_region(canvas, -1000, -1000, 1000, 1000);
                cr_canvas_center_on(canvas, 0, 0);
        }
}

static gboolean
on_center_selected(CrCanvas *canvas, GdkEvent *event)
{
        GdkCursor *cursor;
        GdkWindow *window;
        double x, y;

        if (event->type != GDK_BUTTON_PRESS) return FALSE;

        x = event->button.x;
        y = event->button.y;

        /* These are device coordinates, but we want the user coordinates
         * for the root canvas item.  So transform them. */
        if (canvas->root->matrix)
                cairo_matrix_transform_point(
                                cr_item_get_inverse_matrix(canvas->root),
                                &x, &y);

        window = gtk_widget_get_parent_window(GTK_WIDGET( canvas));
        cursor = g_object_get_data(G_OBJECT(canvas), "cursor");
        if (cursor) gdk_cursor_unref(cursor);
        g_object_set_data(G_OBJECT(canvas), "cursor", NULL);
        gdk_window_set_cursor(window, NULL);

        g_signal_handlers_disconnect_by_func(canvas,
                        G_CALLBACK(on_center_selected), NULL);

        cr_canvas_center_on(canvas, x, y);

        return TRUE; 
}

static void
on_center_clicked(GtkButton *button, CrCanvas *canvas)
{
        GdkCursor *cursor;
        GdkWindow *window;

        g_signal_connect(canvas, "event", G_CALLBACK(on_center_selected), NULL);
        cursor = g_object_get_data(G_OBJECT(canvas), "cursor");
        if (cursor) gdk_cursor_unref(cursor);
        window = gtk_widget_get_parent_window(GTK_WIDGET( canvas));
        cursor = gdk_cursor_new(GDK_CIRCLE);
        g_object_set_data(G_OBJECT(canvas), "cursor", cursor);
        gdk_window_set_cursor(window, cursor);
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

static GtkWidget *
setup_canvas()
{
        GtkWidget *window1, *canvas, *scrolled_window;
        GdkColor color;

        window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title (GTK_WINDOW (window1), "CR Canvas Demo");
        g_signal_connect (G_OBJECT (window1), "delete-event",
                        G_CALLBACK (gtk_main_quit), NULL);


        canvas = cr_canvas_new(
                                "maintain_center", TRUE,
                                "repaint_mode", TRUE,
                                "show_less", FALSE,
                                NULL);
        gtk_widget_set_size_request(canvas, 400, 400);
        cr_canvas_set_scroll_factor(CR_CANVAS(canvas), 3,3);

        color.pixel = color.red = color.green = color.blue = 0;
        gtk_widget_modify_bg(canvas, GTK_STATE_NORMAL, &color);

        /* If a child has native scrolling, use gtk_container_add() instead of
         * this function.*/
        scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scrolled_window), canvas);

        gtk_container_add (GTK_CONTAINER (window1), scrolled_window);

        /* It is possible to do this sort of thing before the window is
         * realized, but the end result will depend on how the properties of the
         * canvas are set. */
        cr_canvas_center_on(CR_CANVAS(canvas), 0,0);
        cr_canvas_zoom(CR_CANVAS(canvas), .5, .5);

        gtk_widget_show_all(window1);
        gtk_window_move(GTK_WINDOW(window1), 0, 0);

        return canvas;
}

static void
setup_controls(GtkWidget *canvas)
{

        GtkWidget *window2, *vbox1, *frame1, *frame2, *vbox2, *hbox1, *hbbox,
                  *frame3, *hbox2, *rb, *cb, *button, *label, *alignment1;
        GSList *rb_group;
        GSList *none_list;
        GtkTooltips *tooltips;
        CrZoomer *zoomer;
        CrPanner *panner;
        CrRotator *rotator;

        tooltips = gtk_tooltips_new ();
        zoomer = cr_zoomer_new(CR_CANVAS(canvas), "fill_color_rgba", 
                        0xaa00aa77, "outline_color_rgba", 0xffffffff, NULL);
        panner = cr_panner_new(CR_CANVAS(canvas), "button", 1, NULL);
        rotator = cr_rotator_new(CR_CANVAS(canvas), NULL);


        window2 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title (GTK_WINDOW (window2), "CR Canvas Demo");
        g_signal_connect (G_OBJECT (window2), "delete-event",
                        G_CALLBACK (gtk_main_quit), NULL);

        vbox1 = gtk_vbox_new(FALSE, 10);
        gtk_container_add (GTK_CONTAINER (window2), vbox1);

        frame1 = gtk_frame_new (NULL);
        gtk_box_pack_start (GTK_BOX (vbox1), frame1, TRUE, TRUE, 0);

        vbox2 = gtk_vbox_new (FALSE, 0);
        gtk_container_add (GTK_CONTAINER (frame1), vbox2);

        hbox1 = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox2), hbox1, TRUE, TRUE, 0);

        cb = gtk_check_button_new_with_label ("Maintain Center");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), TRUE);
        gtk_box_pack_start (GTK_BOX (hbox1), cb, FALSE, FALSE, 0);
        gtk_tooltips_set_tip (tooltips, cb, 
                        "On a resize or scaling operation the center point"
                        " is maintained.", NULL);
        g_signal_connect(cb, "toggled", G_CALLBACK (on_set_property),
                        canvas);
        none_list = g_slist_append(NULL, cb);

        cb = gtk_check_button_new_with_label ("Auto Scale");
        gtk_box_pack_start (GTK_BOX (hbox1), cb, FALSE, FALSE, 0);
        gtk_tooltips_set_tip (tooltips, cb, 
                        "On a resize or scaling operation the contents of the "
                        "viewable area are maintained.",
                        NULL);
        g_signal_connect(cb, "toggled", G_CALLBACK (on_set_property),
                        canvas);
        none_list = g_slist_append(none_list, cb);

        cb = gtk_check_button_new_with_label("Maintain Aspect");
        gtk_box_pack_start (GTK_BOX (hbox1), cb, FALSE, FALSE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), TRUE);
        gtk_tooltips_set_tip (tooltips, cb, 
                        "On any scaling operation the relative scaling between"
                        " top and bottom and left and right are maintained.",
                        NULL);
        g_signal_connect(cb, "toggled", G_CALLBACK (on_set_property),
                        canvas);
        none_list = g_slist_append(none_list, cb);

        button = GTK_WIDGET( g_object_new(GTK_TYPE_BUTTON, 
                                "label", "None (document style)",
                                NULL));
        gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
        gtk_tooltips_set_tip (tooltips, button, 
                        "On a resize or scaling operation the contents at the "
                        "upper left of the viewable area are maintained.",
                        NULL);
        g_signal_connect(button, "clicked", G_CALLBACK (on_set_property),
                        canvas);
        g_object_set_data_full(G_OBJECT(button), "none_list", none_list,
                        (GDestroyNotify) g_slist_free);

        label = gtk_label_new ("<b>Model (Canvas Properties)</b>");
        gtk_frame_set_label_widget (GTK_FRAME (frame1), label);
        gtk_label_set_use_markup (GTK_LABEL (label), TRUE);


        frame3 = gtk_frame_new (NULL);
        gtk_box_pack_start (GTK_BOX (vbox1), frame3, TRUE, TRUE, 0);

        hbox2 = gtk_hbox_new (FALSE, 0);
        gtk_container_add (GTK_CONTAINER (frame3), hbox2);

        rb = gtk_radio_button_new_with_label (NULL, "Scroll Factor (infinite)");
        gtk_box_pack_start (GTK_BOX (hbox2), rb, FALSE, FALSE, 0);
        rb_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (rb));
        gtk_tooltips_set_tip (tooltips, rb, 
                        "World is assumed to be infinite.  Scrollbars allow "
                        "scrolling a distance that is a factor of the present "
                        "viewport size.", 
                        NULL);
        g_signal_connect(rb, "toggled", G_CALLBACK (on_set_scroll_factor),
                        canvas);

        rb = gtk_radio_button_new_with_label(rb_group, "Scroll Region (fixed)");
        gtk_box_pack_start (GTK_BOX (hbox2), rb, FALSE, FALSE, 0);
        rb_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (rb));
        gtk_tooltips_set_tip (tooltips, rb, 
                        "World is set to a pre-defined extent.  It is "
                        "impossible to scroll outside this region.",
                        NULL);
        g_signal_connect(rb, "toggled", G_CALLBACK (on_set_scroll_region),
                        canvas);

        label = gtk_label_new ("<b>World Definition</b>");
        gtk_frame_set_label_widget (GTK_FRAME (frame3), label);
        gtk_label_set_use_markup (GTK_LABEL (label), TRUE);


        frame2 = gtk_frame_new (NULL);
        gtk_box_pack_start (GTK_BOX (vbox1), frame2, TRUE, TRUE, 0);

        alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
        gtk_container_add (GTK_CONTAINER (frame2), alignment1);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 12, 0);

        hbbox = gtk_hbutton_box_new ();
        gtk_container_add (GTK_CONTAINER (alignment1), hbbox);

        button = GTK_WIDGET( g_object_new(GTK_TYPE_BUTTON, 
                                "label", "Zoom X2",
                                NULL));
        gtk_container_add (GTK_CONTAINER (hbbox), button);
        g_signal_connect(button, "clicked", G_CALLBACK (on_zoom_in),
                        canvas);

        button = GTK_WIDGET( g_object_new(GTK_TYPE_BUTTON, 
                                "label", "Zoom /2",
                                NULL));
        gtk_container_add (GTK_CONTAINER (hbbox), button);
        g_signal_connect(button, "clicked", G_CALLBACK (on_zoom_out),
                        canvas);

        button = GTK_WIDGET( g_object_new(GTK_TYPE_BUTTON, 
                                "label", "Box Zoom",
                                NULL));
        gtk_container_add (GTK_CONTAINER (hbbox), button);
        g_signal_connect_swapped(button, "clicked", G_CALLBACK
                        (cr_zoomer_activate), zoomer);

        button = GTK_WIDGET( g_object_new(GTK_TYPE_BUTTON, 
                                "label", "Center on",
                                NULL));
        gtk_container_add (GTK_CONTAINER (hbbox), button);
        g_signal_connect(button, "clicked", G_CALLBACK (on_center_clicked),
                        canvas);

        button = GTK_WIDGET( g_object_new(GTK_TYPE_BUTTON, 
                                "label", "Pan",
                                NULL));
        gtk_container_add (GTK_CONTAINER (hbbox), button);
        g_signal_connect_swapped(button, "clicked", G_CALLBACK
                        (cr_panner_activate), panner);

        button = GTK_WIDGET( g_object_new(GTK_TYPE_BUTTON, 
                                "label", "Rotate",
                                NULL));
        gtk_container_add (GTK_CONTAINER (hbbox), button);
        g_signal_connect_swapped(button, "clicked", G_CALLBACK
                        (cr_rotator_activate), rotator);

        label = gtk_label_new ("<b>Navigation (Canvas Functions)</b>");
        gtk_frame_set_label_widget (GTK_FRAME (frame2), label);
        gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

        gtk_widget_show_all(window2);
        gtk_window_move(GTK_WINDOW(window2), 0, gdk_screen_height() -
                        window2->allocation.height * 2);

}

int
main(int argc, char **argv)
{
        GdkColor color;
        CrItem *item;
        CrCanvas *canvas;
        double w, h, cx,cy;

        gtk_init(&argc, &argv);

        canvas = CR_CANVAS(setup_canvas());

        item = g_object_new(CR_TYPE_ITEM, NULL);
        g_signal_connect(item, "paint", G_CALLBACK(paint), NULL);
        cr_item_add(canvas->root, item);
        g_object_unref(item);

        setup_controls(GTK_WIDGET(canvas));


        gtk_main();
        return 0;
}





