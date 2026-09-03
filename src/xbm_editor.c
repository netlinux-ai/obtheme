/* xbm_editor.c for ObTheme
   Copyright (c) 2026  NetLinux

   Secondary window: a pixel-grid editor for the titlebar buttons'
   XBM icon bitmaps, modeled on the original ObTheme's xbm-editor
   screenshot (bitmap picker + grid + resize + description + reset),
   built programmatically rather than via GtkBuilder since a dynamic
   pixel grid has little to gain from declarative UI.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License (version 2) as
   published by the Free Software Foundation.
*/

#include "main.h"
#include "xbm_editor.h"
#include "xbm_io.h"
#include "themerc.h"
#include <string.h>

#define CELL 20 /* pixels per bitmap cell, on screen */

static GtkWidget *win = NULL;
static GtkWidget *combo, *area, *desc_label, *rows_label, *cols_label;
static guint cur_w = 6, cur_h = 6;
static guchar *cur_bits = NULL; /* cur_w * cur_h, row-major, 0/1 */
static gint painting_value = -1; /* value being painted during a drag */

static gchar *current_theme_dir(void)
{
    return current_doc ? current_doc->theme_dir : NULL;
}

static gchar *bitmap_path_for(guint idx)
{
    gchar *fn, *path;
    gchar *dir = current_theme_dir();

    if (!dir || idx >= BUTTON_BITMAP_SCHEMA_COUNT)
        return NULL;

    fn = button_bitmap_filename(&BUTTON_BITMAP_SCHEMA[idx]);
    path = g_build_filename(dir, "openbox-3", fn, NULL);
    g_free(fn);
    return path;
}

static void resize_bits(guint neww, guint newh)
{
    guchar *nb = g_malloc0(neww * newh);
    guint x, y;

    for (y = 0; y < MIN(cur_h, newh); ++y)
        for (x = 0; x < MIN(cur_w, neww); ++x)
            nb[y * neww + x] = cur_bits[y * cur_w + x];

    g_free(cur_bits);
    cur_bits = nb;
    cur_w = neww;
    cur_h = newh;
}

static void refresh_geometry_widgets(void);

static void load_current_selection(void)
{
    gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    gchar *path;
    const ButtonBitmapSpec *spec;

    g_free(cur_bits);
    cur_bits = NULL;

    if (idx < 0)
        return;

    spec = &BUTTON_BITMAP_SCHEMA[idx];
    gtk_label_set_text(GTK_LABEL(desc_label), spec->description);

    path = bitmap_path_for((guint)idx);
    if (path && xbm_read(path, &cur_w, &cur_h, &cur_bits)) {
        g_free(path);
        refresh_geometry_widgets();
        return;
    }
    g_free(path);

    /* No file for this exact state -- fall back through the same chain
       obrender/theme.c itself uses: a pressed/disabled/hover state
       without its own file copies whatever the *base* (state="") icon
       resolved to, and the base state falls back to a hardcoded
       built-in shape (xbm_hardcoded_default) if it too has no file.
       Showing this instead of a blank grid means Apply-without-editing
       can't silently overwrite a theme's real default icon with a
       blank one. */
    if (spec->state[0]) {
        ButtonBitmapSpec basespec = { spec->button, "", spec->toggled, NULL };
        gchar *basefn = button_bitmap_filename(&basespec);
        gchar *dir = current_theme_dir();
        gchar *basepath = dir ? g_build_filename(dir, "openbox-3", basefn, NULL)
                              : NULL;
        gboolean got_base;

        g_free(basefn);
        got_base = basepath && xbm_read(basepath, &cur_w, &cur_h, &cur_bits);
        g_free(basepath);

        if (got_base) {
            refresh_geometry_widgets();
            return;
        }
    }

    if (xbm_hardcoded_default(spec->button, spec->toggled, &cur_bits)) {
        cur_w = 6;
        cur_h = 6;
    } else {
        cur_w = 6;
        cur_h = 6;
        cur_bits = g_malloc0(cur_w * cur_h);
    }

    refresh_geometry_widgets();
}

static void on_combo_changed(GtkComboBox *w, gpointer data)
{
    load_current_selection();
}

static gboolean on_draw(GtkWidget *w, cairo_t *cr, gpointer data)
{
    guint x, y;

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    if (!cur_bits)
        return FALSE;

    for (y = 0; y < cur_h; ++y) {
        for (x = 0; x < cur_w; ++x) {
            if (cur_bits[y * cur_w + x])
                cairo_set_source_rgb(cr, 0, 0, 0);
            else
                cairo_set_source_rgb(cr, 0.92, 0.92, 0.92);
            cairo_rectangle(cr, x * CELL, y * CELL, CELL - 1, CELL - 1);
            cairo_fill(cr);
        }
    }
    return FALSE;
}

static void paint_at(gdouble ex, gdouble ey, gint set_to)
{
    gint x = (gint)(ex / CELL);
    gint y = (gint)(ey / CELL);

    if (x < 0 || y < 0 || (guint)x >= cur_w || (guint)y >= cur_h || !cur_bits)
        return;

    if (set_to < 0)
        set_to = !cur_bits[y * cur_w + x];

    if (cur_bits[y * cur_w + x] != set_to) {
        cur_bits[y * cur_w + x] = set_to;
        gtk_widget_queue_draw(area);
    }
}

static gboolean on_button_press(GtkWidget *w, GdkEventButton *e, gpointer d)
{
    gint x = (gint)(e->x / CELL), y = (gint)(e->y / CELL);
    if (x >= 0 && y >= 0 && (guint)x < cur_w && (guint)y < cur_h && cur_bits)
        painting_value = !cur_bits[y * cur_w + x];
    paint_at(e->x, e->y, painting_value);
    return TRUE;
}

static gboolean on_motion(GtkWidget *w, GdkEventMotion *e, gpointer d)
{
    if (painting_value >= 0 && (e->state & GDK_BUTTON1_MASK))
        paint_at(e->x, e->y, painting_value);
    return TRUE;
}

static gboolean on_button_release(GtkWidget *w, GdkEventButton *e, gpointer d)
{
    painting_value = -1;
    return TRUE;
}

static void refresh_geometry_widgets(void)
{
    gchar buf[16];

    g_snprintf(buf, sizeof(buf), "%u", cur_w);
    gtk_label_set_text(GTK_LABEL(cols_label), buf);
    g_snprintf(buf, sizeof(buf), "%u", cur_h);
    gtk_label_set_text(GTK_LABEL(rows_label), buf);

    gtk_widget_set_size_request(area, (gint)(cur_w * CELL),
                                 (gint)(cur_h * CELL));
    gtk_widget_queue_draw(area);
}

static void on_cols_plus(GtkButton *b, gpointer d)
{ resize_bits(cur_w + 1, cur_h); refresh_geometry_widgets(); }
static void on_cols_minus(GtkButton *b, gpointer d)
{ if (cur_w > 1) resize_bits(cur_w - 1, cur_h); refresh_geometry_widgets(); }
static void on_rows_plus(GtkButton *b, gpointer d)
{ resize_bits(cur_w, cur_h + 1); refresh_geometry_widgets(); }
static void on_rows_minus(GtkButton *b, gpointer d)
{ if (cur_h > 1) resize_bits(cur_w, cur_h - 1); refresh_geometry_widgets(); }

static void on_revert(GtkButton *b, gpointer d)
{
    load_current_selection();
}

static void on_apply(GtkButton *b, gpointer d)
{
    gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    gchar *path, *dir;

    if (idx < 0 || !cur_bits)
        return;

    path = bitmap_path_for((guint)idx);
    dir = g_path_get_dirname(path);
    g_mkdir_with_parents(dir, 0755);
    g_free(dir);

    if (xbm_write(path, cur_w, cur_h, cur_bits,
                  BUTTON_BITMAP_SCHEMA[idx].button))
        refresh_preview();
    else
        g_printerr("Failed to write %s\n", path);

    g_free(path);
}

static gboolean on_delete(GtkWidget *w, GdkEvent *e, gpointer d)
{
    gtk_widget_hide(w);
    return TRUE; /* don't destroy -- keep state for next open */
}

void xbm_editor_open(void)
{
    GtkWidget *vbox, *hbox, *ctrl;
    guint i;

    if (!current_doc)
        return;

    if (win) {
        gtk_widget_show_all(win);
        gtk_window_present(GTK_WINDOW(win));
        load_current_selection();
        return;
    }

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "ObTheme - Button Icons");
    gtk_window_set_default_size(GTK_WINDOW(win), 360, 420);
    g_signal_connect(win, "delete-event", G_CALLBACK(on_delete), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    combo = gtk_combo_box_text_new();
    for (i = 0; i < BUTTON_BITMAP_SCHEMA_COUNT; ++i) {
        gchar *fn = button_bitmap_filename(&BUTTON_BITMAP_SCHEMA[i]);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), fn);
        g_free(fn);
    }
    g_signal_connect(combo, "changed", G_CALLBACK(on_combo_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);

    area = gtk_drawing_area_new();
    gtk_widget_add_events(area, GDK_BUTTON_PRESS_MASK |
                                 GDK_BUTTON_RELEASE_MASK |
                                 GDK_POINTER_MOTION_MASK);
    g_signal_connect(area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(area, "button-press-event",
                      G_CALLBACK(on_button_press), NULL);
    g_signal_connect(area, "motion-notify-event",
                      G_CALLBACK(on_motion), NULL);
    g_signal_connect(area, "button-release-event",
                      G_CALLBACK(on_button_release), NULL);
    {
        GtkWidget *frame = gtk_frame_new(NULL);
        gtk_container_add(GTK_CONTAINER(frame), area);
        gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
    }

    ctrl = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    {
        GtkWidget *b;
        b = gtk_button_new_with_label("-");
        g_signal_connect(b, "clicked", G_CALLBACK(on_cols_minus), NULL);
        gtk_box_pack_start(GTK_BOX(ctrl), b, FALSE, FALSE, 0);
        cols_label = gtk_label_new("6");
        gtk_box_pack_start(GTK_BOX(ctrl), gtk_label_new("cols"), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(ctrl), cols_label, FALSE, FALSE, 0);
        b = gtk_button_new_with_label("+");
        g_signal_connect(b, "clicked", G_CALLBACK(on_cols_plus), NULL);
        gtk_box_pack_start(GTK_BOX(ctrl), b, FALSE, FALSE, 0);

        b = gtk_button_new_with_label("-");
        g_signal_connect(b, "clicked", G_CALLBACK(on_rows_minus), NULL);
        gtk_box_pack_start(GTK_BOX(ctrl), b, FALSE, FALSE, 12);
        rows_label = gtk_label_new("6");
        gtk_box_pack_start(GTK_BOX(ctrl), gtk_label_new("rows"), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(ctrl), rows_label, FALSE, FALSE, 0);
        b = gtk_button_new_with_label("+");
        g_signal_connect(b, "clicked", G_CALLBACK(on_rows_plus), NULL);
        gtk_box_pack_start(GTK_BOX(ctrl), b, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(vbox), ctrl, FALSE, FALSE, 0);

    desc_label = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(desc_label), TRUE);
    gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), desc_label, FALSE, FALSE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    {
        GtkWidget *revert = gtk_button_new_with_label("Revert");
        GtkWidget *apply = gtk_button_new_with_label("Apply");
        g_signal_connect(revert, "clicked", G_CALLBACK(on_revert), NULL);
        g_signal_connect(apply, "clicked", G_CALLBACK(on_apply), NULL);
        gtk_box_pack_start(GTK_BOX(hbox), revert, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(hbox), apply, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    gtk_widget_show_all(win);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
}
