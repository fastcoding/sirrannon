#include <gtk/gtk.h>
#include <cr-canvas.h>
#include <cr-item.h>

typedef struct {
        double x1, y1, x2, y2, width_units;
        double x, y;
        double alpha;
        const char *color;
        const char *shape;
} ItemProperties;

static void
properties_free(ItemProperties *properties)
{
        g_free(properties);
}

static const char*
get_color()
{
        int i;
        const char *colors[] = {"red",
                "yellow",
                "green",
                "cyan",
                "blue",
                "magenta"};

        i = g_random_int_range(0, 6);

        return colors[i];
}

static gboolean
item_event(CrItem *item, GdkEvent *event, cairo_matrix_t *matrix, 
                CrItem *pick_item, CrItem *root)
{
        static int last_msec;
        gboolean state;
        double new_x, new_y;
        const char *new_color;
        ItemProperties *properties;

        properties = g_object_get_data(G_OBJECT(item), "properties");
        state = FALSE;

        if (event->type == GDK_BUTTON_PRESS) {
                if (event->button.button == 1) {
                        properties->x = event->button.x;
                        properties->y = event->button.y;
                }
                else if (event->button.button == 3) {
                        cr_item_remove(root, item);
                }
                state = TRUE;
        }
        else if (event->type == GDK_2BUTTON_PRESS) {
                while (strcmp((new_color = get_color()), properties->color)
                                == 0);
                properties->color = new_color;
                cr_item_request_update(item);
                state = TRUE;
        }
        else if (event->type == GDK_MOTION_NOTIFY) {
                if (event->motion.time - last_msec < 100) return FALSE;

                if (event->motion.state & GDK_BUTTON1_MASK) {

                        new_x = event->motion.x;
                        new_y = event->motion.y;

                        cairo_matrix_translate(cr_item_get_matrix(item),
                                        new_x - properties->x, 
                                        new_y - properties->y);
                        cr_item_request_update(item);

                        last_msec = event->motion.time;

                        state = TRUE;
                }
        }
        else if (event->type == GDK_ENTER_NOTIFY) {
                properties->width_units = 3;
                cr_item_request_update(item);
                state = TRUE;
        }
        else if (event->type == GDK_LEAVE_NOTIFY) {
                properties->width_units = 1;
                cr_item_request_update(item);
                state = TRUE;
        }
        return state;
}

static void 
draw_ellipse(cairo_t *c, double x1, double y1, double x2, double y2)
{
        double yc;

        yc = y1 + (y2 - y1) / 2;

        cairo_move_to(c, x1, yc);
        cairo_curve_to(c, x1, y1, x2, y1, x2, yc);
        cairo_curve_to(c, x2, y2, x1, y2, x1, yc);
        cairo_close_path (c);
}

static void
draw(ItemProperties *properties, cairo_t *c)
{
        if (strcmp(properties->shape, "rectangle") == 0)

                cairo_rectangle(c, properties->x1, properties->y1,
                                properties->x2 - properties->x1, properties->y2
                                - properties->y1);

        else
                draw_ellipse(c, properties->x1, properties->y1, properties->x2,
                                properties->y2);

}

static void
on_paint(CrItem *item, cairo_t *c)
{
        GdkColor color;
        ItemProperties *properties;

        properties = g_object_get_data(G_OBJECT(item), "properties");

        gdk_color_parse(properties->color, &color);

        cairo_new_path(c);

        draw(properties, c);

        cairo_set_source_rgba(c, (double)color.red / G_MAXSHORT,
                        (double)color.green / G_MAXSHORT, (double)color.blue /
                        G_MAXSHORT, properties->alpha);

        cairo_fill_preserve(c);

        cairo_set_source_rgb(c, 0, 0, 0);

        cairo_set_line_width(c, properties->width_units);

        cairo_stroke(c);
}

static CrItem *
on_test(CrItem *item, cairo_t *c, double x, double y)
{
        ItemProperties *properties;
        gboolean inside;

        properties = g_object_get_data(G_OBJECT(item), "properties");

        cairo_save(c);
        cairo_new_path(c);

        draw(properties, c);

        inside = cairo_in_fill(c, x, y);

        cairo_restore(c);
        return inside ? item : NULL;
}

static gboolean
on_calculate_bounds(CrItem *item, cairo_t *c, CrBounds *bounds,
                CrDeviceBounds *device)
{
        ItemProperties *properties;

        properties = g_object_get_data(G_OBJECT(item), "properties");

        cairo_save(c);
        cairo_set_line_width(c, properties->width_units);
        cairo_new_path(c);

        draw(properties, c);

        cairo_stroke_extents(c, &bounds->x1, &bounds->y1, &bounds->x2, 
                        &bounds->y2);

        cairo_restore(c);

        /*
        *x1 = properties->x1;
        *y1 = properties->y1;
        *x2 = properties->x2;
        *y2 = properties->y2;
        */
        return TRUE;
}

static void
add_item(CrItem *root)
{
        double x1, y1, x2, y2, tmp;
        ItemProperties *properties;
        GObject *item;

        properties = g_new0(ItemProperties, 1);
        properties->color = get_color();
        properties->width_units = 1;

        x1 = g_random_double_range(0, 400);
        y1 = g_random_double_range(0, 400);
        x2 = g_random_double_range(0, 400);
        y2 = g_random_double_range(0, 400);

        if (x1 > x2) {
                tmp = x2;
                x2 = x1;
                x1 = tmp;
        }
        if (y1 > y2) {
                tmp = y2;
                y2 = y1;
                y1 = tmp;
        }

        if (x2 - x1 < 10)
                x2 += 10;

        if (y2 - y1 < 10)
                y2 += 10;

        properties->x1 = x1;
        properties->y1 = y1;
        properties->x2 = x2;
        properties->y2 = y2;

        if (g_random_boolean()) 
                properties->shape = "rectangle";
        else
                properties->shape = "ellipse";

        properties->alpha = g_random_double_range(.3, 1.0);

        item = g_object_new(CR_TYPE_ITEM, NULL);
        g_object_set_data_full(item, "properties", properties, (GDestroyNotify)
                        properties_free);

        g_signal_connect(item, "event", (GCallback) item_event, root);
        g_signal_connect(item, "paint", (GCallback) on_paint, NULL);
        g_signal_connect(item, "test", (GCallback) on_test, NULL);
        g_signal_connect(item, "calculate_bounds", (GCallback) 
                        on_calculate_bounds, NULL);


        cr_item_add(root, CR_ITEM(item));
        g_object_unref(item);
}

int 
main(int argc, char **argv)
{
        GtkWidget *window, *canvas, *vbox, *hbox, *label, *button1, *button2;

        gtk_init(&argc, &argv);

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title (GTK_WINDOW (window), "CR Canvas Items Demo");
        g_signal_connect (G_OBJECT (window), "delete-event",
                        G_CALLBACK (gtk_main_quit), NULL);

        vbox = gtk_vbox_new(FALSE, 10);
        gtk_container_add (GTK_CONTAINER (window), vbox);

        label = gtk_label_new("Drag - move object.\n"
                        "Double click - change colour\n"
                        "Right click - delete object");
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        canvas = cr_canvas_new(NULL);
        gtk_widget_set_size_request(canvas, 400,400);
        gtk_box_pack_start (GTK_BOX (vbox), canvas, TRUE, TRUE, 0);

        hbox = gtk_hbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

        button1 = gtk_button_new_with_label("Add an object");
        g_signal_connect_swapped(button1, "clicked", G_CALLBACK( add_item),
                        CR_CANVAS(canvas)->root);
        gtk_box_pack_start (GTK_BOX (hbox), button1, FALSE, FALSE, 0);

        button2 = gtk_button_new_with_label("Quit");
        g_signal_connect(button2, "clicked", G_CALLBACK( gtk_main_quit), NULL);
        gtk_box_pack_start (GTK_BOX (hbox), button2, FALSE, FALSE, 0);

        gtk_widget_show_all(window);

        gtk_main();
        return 0;
}







