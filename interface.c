#include <errno.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <gtk/gtk.h>

#include "interface.h"
#include "meteo.h"

/* Every TIMER_INTERVAL_MS real milliseconds, 10 simulated minutes pass. */
#define TIMER_INTERVAL_MS 200
#define EXPORT_FILE_NAME "weather_data.txt"
#define MINUTES_PER_DAY (24 * 60)

/* Which measurement a graph shows. */
typedef enum {
    GRAPH_TEMPERATURE,
    GRAPH_HUMIDITY
} GraphKind;

/* The simulated day and the widgets that must be updated. */
static WeatherDay day;
static GtkWidget *window;
static GtkWidget *clock_label;
static GtkWidget *temperature_label;
static GtkWidget *humidity_label;
static GtkWidget *temperature_graph;
static GtkWidget *humidity_graph;
static GtkWidget *export_button;

/* ---------- Text labels ---------- */

static void update_value_labels(void)
{
    char text[128];

    if (day.count == 0) {
        gtk_label_set_markup(GTK_LABEL(temperature_label), "<span size='xx-large' weight='bold'>-- °C</span>");
        gtk_label_set_markup(GTK_LABEL(humidity_label), "<span size='xx-large' weight='bold'>-- %</span>");
        return;
    }

    const Reading *latest = &day.readings[day.count - 1];

    g_snprintf(text, sizeof text, "<span size='xx-large' weight='bold'>%.1f °C</span>", latest->temperature);
    gtk_label_set_markup(GTK_LABEL(temperature_label), text);

    g_snprintf(text, sizeof text, "<span size='xx-large' weight='bold'>%.0f %%</span>", latest->humidity);
    gtk_label_set_markup(GTK_LABEL(humidity_label), text);
}

/* Shows simulated time and real time side by side so they cannot be confused. */
static void update_clock_label(void)
{
    char simulated[64];
    char real[16] = "--:--:--";
    char text[256];
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    if (local != NULL) {
        strftime(real, sizeof real, "%H:%M:%S", local);
    }

    if (day.count == 0) {
        g_snprintf(simulated, sizeof simulated, "--:--");
    } else if (day.count < READINGS_PER_DAY) {
        int minute = day.readings[day.count - 1].minute;
        g_snprintf(simulated, sizeof simulated, "%02d:%02d", minute / 60, minute % 60);
    } else {
        g_snprintf(simulated, sizeof simulated, "day complete");
    }

    g_snprintf(text, sizeof text,
               "Simulated time: %s (%d/%d readings)    |    Real time: %s\n"
               "10 simulated minutes = %.1f real seconds",
               simulated, day.count, READINGS_PER_DAY, real, TIMER_INTERVAL_MS / 1000.0);
    gtk_label_set_text(GTK_LABEL(clock_label), text);
}

/* ---------- Graphs (Cairo) ---------- */

static double reading_value(const Reading *reading, GraphKind kind)
{
    if (kind == GRAPH_TEMPERATURE) {
        return reading->temperature;
    }
    return reading->humidity;
}

/* Draw centered text whose middle is at (x, y). */
static void draw_centered_text(cairo_t *cr, double x, double y, const char *text)
{
    cairo_text_extents_t extents;

    cairo_text_extents(cr, text, &extents);
    cairo_move_to(cr, x - extents.width / 2 - extents.x_bearing, y - extents.height / 2 - extents.y_bearing);
    cairo_show_text(cr, text);
}

/* Called by GTK every time a graph needs to be (re)drawn. */
static gboolean on_draw_graph(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    GraphKind kind = GPOINTER_TO_INT(data);
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    /* Space around the plot for the title and axis labels. */
    double left = 60, right = 25, top = 35, bottom = 35;
    double plot_width = width - left - right;
    double plot_height = height - top - bottom;

    const char *title;
    double y_min, y_max, y_step;
    double value;
    char label[16];
    int i;

    if (plot_width <= 0 || plot_height <= 0) {
        return FALSE;
    }

    /* Vertical scale: humidity is always 0-100 %, temperature fits the data in 5 °C steps. */
    if (kind == GRAPH_TEMPERATURE) {
        title = "Temperature (°C)";
        y_step = 5;
        if (day.count == 0) {
            y_min = 0;
            y_max = 30;
        } else {
            y_min = TEMPERATURE_MAX;
            y_max = TEMPERATURE_MIN;
            for (i = 0; i < day.count; i++) {
                value = day.readings[i].temperature;
                if (value < y_min) y_min = value;
                if (value > y_max) y_max = value;
            }
            y_min = floor(y_min / y_step) * y_step;
            y_max = ceil(y_max / y_step) * y_step;
            if (y_max <= y_min) {
                y_max = y_min + y_step;
            }
        }
    } else {
        title = "Humidity (%)";
        y_min = HUMIDITY_MIN;
        y_max = HUMIDITY_MAX;
        y_step = 20;
    }

    /* Background */
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    /* Title (vertical axis name and unit) */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, left, 22);
    cairo_show_text(cr, title);

    cairo_set_font_size(cr, 11);
    cairo_set_line_width(cr, 1);

    /* Horizontal grid lines with value labels on the vertical axis */
    for (value = y_min; value <= y_max + 0.001; value += y_step) {
        double y = top + plot_height * (y_max - value) / (y_max - y_min);

        cairo_set_source_rgb(cr, 0.88, 0.88, 0.88);
        cairo_move_to(cr, left, y);
        cairo_line_to(cr, left + plot_width, y);
        cairo_stroke(cr);

        cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        g_snprintf(label, sizeof label, "%.0f", value);
        draw_centered_text(cr, left - 22, y, label);
    }

    /* Time labels on the horizontal axis, every 6 simulated hours */
    for (i = 0; i <= 24; i += 6) {
        double x = left + plot_width * i / 24.0;

        cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        cairo_move_to(cr, x, top + plot_height);
        cairo_line_to(cr, x, top + plot_height + 5);
        cairo_stroke(cr);

        g_snprintf(label, sizeof label, "%02d:00", i);
        draw_centered_text(cr, x, top + plot_height + 16, label);
    }

    /* Axes */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, left, top);
    cairo_line_to(cr, left, top + plot_height);
    cairo_line_to(cr, left + plot_width, top + plot_height);
    cairo_stroke(cr);

    /* Line connecting the measurements */
    if (kind == GRAPH_TEMPERATURE) {
        cairo_set_source_rgb(cr, 0.85, 0.3, 0.1);
    } else {
        cairo_set_source_rgb(cr, 0.1, 0.4, 0.85);
    }
    cairo_set_line_width(cr, 2);
    for (i = 0; i < day.count; i++) {
        double x = left + plot_width * day.readings[i].minute / MINUTES_PER_DAY;
        double y = top + plot_height * (y_max - reading_value(&day.readings[i], kind)) / (y_max - y_min);

        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_stroke(cr);

    return FALSE;
}

/* ---------- Events ---------- */

/* Called every TIMER_INTERVAL_MS: adds one reading until the day is complete. */
static gboolean on_timer(gpointer data)
{
    (void)data;

    if (weather_day_add_reading(&day)) {
        update_value_labels();
        gtk_widget_queue_draw(temperature_graph); /* asks GTK to call on_draw_graph again */
        gtk_widget_queue_draw(humidity_graph);

        if (day.count == READINGS_PER_DAY) {
            gtk_widget_set_sensitive(export_button, TRUE);
        }
    }

    update_clock_label(); /* the real clock keeps ticking after the day is complete */
    return G_SOURCE_CONTINUE;
}

static void on_export_clicked(GtkButton *button, gpointer data)
{
    GtkWidget *dialog;
    (void)button;
    (void)data;

    if (weather_day_export_txt(&day, EXPORT_FILE_NAME) == 0) {
        char *folder = g_get_current_dir();
        char *path = g_build_filename(folder, EXPORT_FILE_NAME, NULL);

        dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                        "%d readings exported to:\n%s", day.count, path);
        g_free(path);
        g_free(folder);
    } else {
        int error = errno; /* save it before other calls can change it */

        dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                        "Could not write %s:\n%s", EXPORT_FILE_NAME, strerror(error));
    }

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* ---------- Window construction ---------- */

/* A framed box with a big value label inside. Returns the value label. */
static GtkWidget *create_value_panel(GtkWidget *parent, const char *name)
{
    GtkWidget *frame = gtk_frame_new(name);
    GtkWidget *value = gtk_label_new("");

    gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5);
    gtk_widget_set_margin_top(value, 15);
    gtk_widget_set_margin_bottom(value, 15);
    gtk_container_add(GTK_CONTAINER(frame), value);
    gtk_box_pack_start(GTK_BOX(parent), frame, TRUE, TRUE, 0);
    return value;
}

/* A framed drawing area for one graph. Returns the drawing area. */
static GtkWidget *create_graph(GtkWidget *parent, GraphKind kind)
{
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *area = gtk_drawing_area_new();

    gtk_widget_set_size_request(area, 600, 220);
    g_signal_connect(area, "draw", G_CALLBACK(on_draw_graph), GINT_TO_POINTER(kind));
    gtk_container_add(GTK_CONTAINER(frame), area);
    gtk_box_pack_start(GTK_BOX(parent), frame, TRUE, TRUE, 0);
    return area;
}

void interface_create(void)
{
    GtkWidget *main_box;
    GtkWidget *title;
    GtkWidget *values_box;

    weather_day_init(&day);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Weather Station");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 800);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 15);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold'>WEATHER STATION</span>");
    gtk_box_pack_start(GTK_BOX(main_box), title, FALSE, FALSE, 0);

    clock_label = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(clock_label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(main_box), clock_label, FALSE, FALSE, 0);

    values_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(values_box), TRUE);
    temperature_label = create_value_panel(values_box, "TEMPERATURE");
    humidity_label = create_value_panel(values_box, "HUMIDITY");
    gtk_box_pack_start(GTK_BOX(main_box), values_box, FALSE, FALSE, 0);

    temperature_graph = create_graph(main_box, GRAPH_TEMPERATURE);
    humidity_graph = create_graph(main_box, GRAPH_HUMIDITY);

    /* Export is enabled once the 24 simulated hours are complete. */
    export_button = gtk_button_new_with_label("EXPORT TO TXT");
    gtk_widget_set_halign(export_button, GTK_ALIGN_CENTER);
    gtk_widget_set_sensitive(export_button, FALSE);
    g_signal_connect(export_button, "clicked", G_CALLBACK(on_export_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(main_box), export_button, FALSE, FALSE, 0);

    update_value_labels();
    update_clock_label();
    g_timeout_add(TIMER_INTERVAL_MS, on_timer, NULL);

    gtk_widget_show_all(window);
}
