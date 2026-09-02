/* info_panel.c for ObTheme
   Copyright (c) 2026  NetLinux

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License (version 2) as
   published by the Free Software Foundation.
*/

#include "main.h"
#include "info_panel.h"
#include "element_list.h"
#include "themerc.h"
#include <string.h>
#include <stdlib.h>

/* the element currently shown/editable in the panel */
static const ThemeKeySpec *cur_spec = NULL;
static gboolean populating = FALSE; /* guards edit handlers while we
                                        set widget values programmatically */

static void save_and_refresh(void);

/* Openbox/X11 themes commonly use the X11 "rgb:RR/GG/BB" hex-triple
   notation alongside plain "#rrggbb" and CSS/X11 color names.
   gdk_rgba_parse() understands the latter two but not the X11 slash
   form, so translate it to "#rrggbb" first. Returns a newly-allocated
   string; caller g_free()s. If v isn't in "rgb:" form, returns a copy
   of v unchanged. */
static gchar *normalize_color_string(const gchar *v)
{
    gchar **parts;
    gchar *out;

    if (!v || strncmp(v, "rgb:", 4) != 0)
        return g_strdup(v);

    parts = g_strsplit(v + 4, "/", 3);
    if (parts[0] && parts[1] && parts[2] && !parts[3]) {
        out = g_strdup_printf("#%02x%02x%02x",
                               (gint)strtol(parts[0], NULL, 16),
                               (gint)strtol(parts[1], NULL, 16),
                               (gint)strtol(parts[2], NULL, 16));
    } else {
        out = g_strdup(v);
    }
    g_strfreev(parts);
    return out;
}

/* ---- texture value grammar: parse/serialize ----
   Mirrors the strstr-based, order-dependent rules in
   /Data/openbox/obrender/theme.c's parse_appearance() (researched and
   cataloged earlier in this project). Our own serializer always
   produces clean canonical tokens, so ambiguity in the *parser* only
   matters for reading pre-existing themerc files written by other
   tools/by hand. */
typedef struct {
    gint grad;   /* 0=solid, 1=gradient, 2=parentrelative */
    gint dir;    /* 0=horizontal 1=vertical 2=diagonal 3=crossdiagonal
                    4=pyramid 5=mirrorhorizontal 6=splitvertical */
    gint relief; /* 0=flat 1=raised 2=sunken */
    gboolean border;
    gint bevel;  /* 1 or 2 */
    gboolean interlaced;
} TextureParsed;

static void parse_texture(const gchar *raw, TextureParsed *t)
{
    gchar *low;

    t->grad = 0; t->dir = 1; t->relief = 1;
    t->border = FALSE; t->bevel = 1; t->interlaced = FALSE;

    if (!raw)
        return;

    low = g_ascii_strdown(raw, -1);

    if (strstr(low, "parentrelative")) {
        t->grad = 2;
    } else if (strstr(low, "gradient")) {
        t->grad = 1;
        if (strstr(low, "crossdiagonal"))       t->dir = 3;
        else if (strstr(low, "pyramid"))         t->dir = 4;
        else if (strstr(low, "mirrorhorizontal")) t->dir = 5;
        else if (strstr(low, "horizontal"))       t->dir = 0;
        else if (strstr(low, "splitvertical"))    t->dir = 6;
        else if (strstr(low, "vertical"))          t->dir = 1;
        else                                        t->dir = 2; /* diagonal */
    } else {
        t->grad = 0;
    }

    if (strstr(low, "sunken"))       t->relief = 2;
    else if (strstr(low, "flat"))     t->relief = 0;
    else if (strstr(low, "raised"))   t->relief = 1;
    else                                t->relief = (t->grad == 2) ? 0 : 1;

    t->border = (strstr(low, "border") != NULL);
    t->bevel = strstr(low, "bevel2") ? 2 : 1;
    t->interlaced = (strstr(low, "interlaced") != NULL);

    g_free(low);
}

static gchar *serialize_texture(const TextureParsed *t)
{
    static const char *relief_names[] = { "Flat", "Raised", "Sunken" };
    static const char *dir_names[] = {
        "Horizontal", "Vertical", "Diagonal", "CrossDiagonal",
        "Pyramid", "MirrorHorizontal", "SplitVertical"
    };
    GString *s = g_string_new(NULL);

    if (t->grad == 2) {
        g_string_append(s, "ParentRelative");
        return g_string_free(s, FALSE);
    }

    g_string_append(s, relief_names[t->relief]);
    if (t->relief == 0 && t->border)
        g_string_append(s, " Border");
    if (t->grad == 1) {
        g_string_append(s, " Gradient ");
        g_string_append(s, dir_names[t->dir]);
    } else {
        g_string_append(s, " Solid");
    }
    if (t->relief != 0 && t->bevel == 2)
        g_string_append(s, " Bevel2");
    if (t->interlaced)
        g_string_append(s, " Interlaced");

    return g_string_free(s, FALSE);
}

static void set_color_button(GtkColorButton *btn, const gchar *raw)
{
    GdkRGBA rgba;
    gchar *v = normalize_color_string(raw ? raw : "black");
    if (!gdk_rgba_parse(&rgba, v))
        gdk_rgba_parse(&rgba, "black");
    g_free(v);
    gtk_color_button_set_rgba(btn, &rgba);
}

static gchar *get_color_button_hex(GtkColorButton *btn)
{
    GdkRGBA rgba;
    gtk_color_button_get_rgba(btn, &rgba);
    return g_strdup_printf("#%02x%02x%02x",
                            (gint)(rgba.red * 255),
                            (gint)(rgba.green * 255),
                            (gint)(rgba.blue * 255));
}

static void update_texture_widget_visibility(const TextureParsed *t)
{
    GtkWidget *dir = get_widget("texture_direction_combo");
    GtkWidget *bevel = get_widget("texture_bevel_combo");
    GtkWidget *border_chk = get_widget("texture_border_check");
    GtkWidget *border_color = get_widget("texture_border_color_button");
    GtkWidget *color2 = get_widget("texture_color2_button");
    GtkWidget *interlaced = get_widget("texture_interlaced_check");

    gtk_widget_set_visible(dir, t->grad == 1);
    gtk_widget_set_visible(color2, t->grad == 1);
    gtk_widget_set_visible(bevel, t->grad != 2 && t->relief != 0);
    gtk_widget_set_visible(border_chk, t->grad != 2 && t->relief == 0);
    gtk_widget_set_visible(border_color,
                            t->grad != 2 && t->relief == 0 && t->border);
    gtk_widget_set_visible(interlaced, t->grad != 2);
}

static void populate_texture_widgets(const gchar *key)
{
    TextureParsed t;
    const gchar *raw = current_doc ? themerc_get(current_doc, key) : NULL;
    gchar *color_key, *colorto_key, *bcolor_key;

    parse_texture(raw, &t);

    gtk_combo_box_set_active(GTK_COMBO_BOX(get_widget("texture_type_combo")),
                              t.grad);
    gtk_combo_box_set_active(GTK_COMBO_BOX(get_widget("texture_direction_combo")),
                              t.dir);
    gtk_combo_box_set_active(GTK_COMBO_BOX(get_widget("texture_relief_combo")),
                              t.relief);
    gtk_combo_box_set_active(GTK_COMBO_BOX(get_widget("texture_bevel_combo")),
                              t.bevel - 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(get_widget("texture_border_check")),
                                  t.border);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(get_widget("texture_interlaced_check")),
                                  t.interlaced);

    color_key = g_strconcat(key, ".color", NULL);
    colorto_key = g_strconcat(key, ".colorTo", NULL);
    bcolor_key = g_strconcat(key, ".border.color", NULL);

    set_color_button(GTK_COLOR_BUTTON(get_widget("texture_color1_button")),
                      current_doc ? themerc_get(current_doc, color_key) : NULL);
    set_color_button(GTK_COLOR_BUTTON(get_widget("texture_color2_button")),
                      current_doc ? themerc_get(current_doc, colorto_key) : NULL);
    set_color_button(GTK_COLOR_BUTTON(get_widget("texture_border_color_button")),
                      current_doc ? themerc_get(current_doc, bcolor_key) : NULL);

    g_free(color_key);
    g_free(colorto_key);
    g_free(bcolor_key);

    update_texture_widget_visibility(&t);
}

void on_texture_widget_changed(GtkWidget *w, gpointer data)
{
    TextureParsed t;
    gchar *value, *color_key, *colorto_key, *bcolor_key, *color_hex;

    if (populating || !cur_spec || !current_doc)
        return;

    t.grad = gtk_combo_box_get_active(GTK_COMBO_BOX(get_widget("texture_type_combo")));
    t.dir = gtk_combo_box_get_active(GTK_COMBO_BOX(get_widget("texture_direction_combo")));
    t.relief = gtk_combo_box_get_active(GTK_COMBO_BOX(get_widget("texture_relief_combo")));
    t.bevel = gtk_combo_box_get_active(GTK_COMBO_BOX(get_widget("texture_bevel_combo"))) + 1;
    t.border = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(get_widget("texture_border_check")));
    t.interlaced = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(get_widget("texture_interlaced_check")));
    if (t.grad < 0) t.grad = 0;
    if (t.dir < 0) t.dir = 0;
    if (t.relief < 0) t.relief = 0;
    if (t.bevel < 1) t.bevel = 1;

    value = serialize_texture(&t);
    themerc_set(current_doc, cur_spec->key, value);
    g_free(value);

    color_key = g_strconcat(cur_spec->key, ".color", NULL);
    colorto_key = g_strconcat(cur_spec->key, ".colorTo", NULL);
    bcolor_key = g_strconcat(cur_spec->key, ".border.color", NULL);

    color_hex = get_color_button_hex(GTK_COLOR_BUTTON(get_widget("texture_color1_button")));
    themerc_set(current_doc, color_key, color_hex);
    g_free(color_hex);

    if (t.grad == 1) {
        color_hex = get_color_button_hex(GTK_COLOR_BUTTON(get_widget("texture_color2_button")));
        themerc_set(current_doc, colorto_key, color_hex);
        g_free(color_hex);
    }
    if (t.relief == 0 && t.border) {
        color_hex = get_color_button_hex(GTK_COLOR_BUTTON(get_widget("texture_border_color_button")));
        themerc_set(current_doc, bcolor_key, color_hex);
        g_free(color_hex);
    }

    g_free(color_key);
    g_free(colorto_key);
    g_free(bcolor_key);

    update_texture_widget_visibility(&t);
    save_and_refresh();
    info_panel_show(cur_spec, themerc_get(current_doc, cur_spec->key));
}

static const gchar *type_name(ThemeKeyType t)
{
    switch (t) {
    case TK_INT:         return "integer";
    case TK_COLOR:        return "color";
    case TK_TEXTURE:      return "texture";
    case TK_JUSTIFY:       return "justification";
    case TK_FONT_SHADOW:  return "text shadow string";
    default:               return "string";
    }
}

gboolean on_info_see_also_activate_link(GtkLabel *label, const gchar *uri,
                                         gpointer data)
{
    element_list_select_key(uri);
    return TRUE; /* handled -- don't try to open it as a URL */
}

static void save_and_refresh(void)
{
    GError *error = NULL;

    if (!current_doc)
        return;

    if (!themerc_save(current_doc, &error)) {
        g_printerr("Failed to save themerc: %s\n", error->message);
        g_error_free(error);
        return;
    }
    refresh_preview();
}

void on_edit_color_set(GtkColorButton *w, gpointer data)
{
    GdkRGBA rgba;
    gchar *hex;

    if (populating || !cur_spec || !current_doc)
        return;

    gtk_color_button_get_rgba(w, &rgba);
    hex = g_strdup_printf("#%02x%02x%02x",
                           (gint)(rgba.red * 255),
                           (gint)(rgba.green * 255),
                           (gint)(rgba.blue * 255));
    themerc_set(current_doc, cur_spec->key, hex);
    g_free(hex);

    save_and_refresh();
    info_panel_show(cur_spec, themerc_get(current_doc, cur_spec->key));
}

void on_edit_int_changed(GtkSpinButton *w, gpointer data)
{
    gchar buf[32];
    gint v;

    if (populating || !cur_spec || !current_doc)
        return;

    v = gtk_spin_button_get_value_as_int(w);
    g_snprintf(buf, sizeof(buf), "%d", v);
    themerc_set(current_doc, cur_spec->key, buf);

    save_and_refresh();
    info_panel_show(cur_spec, themerc_get(current_doc, cur_spec->key));
}

void on_edit_reset_clicked(GtkButton *w, gpointer data)
{
    if (!cur_spec || !current_doc)
        return;

    /* "reset to default" -- since our schema defaults are often
       cross-references to another key rather than a literal value
       (e.g. "border.color" for window.active.border.color), the
       correct reset is to remove the explicit override so it falls
       back through the normal chain again. themerc_set has no
       "unset" yet in Phase 3 -- approximate by setting it to the
       literal default string, which is correct whenever default_str
       is itself a literal (color hex, plain int); cross-reference
       defaults are a known rough edge, flagged for Phase 4/5 cleanup. */
    themerc_set(current_doc, cur_spec->key, cur_spec->default_str);
    save_and_refresh();
    info_panel_show(cur_spec, themerc_get(current_doc, cur_spec->key));
}

void info_panel_show(const ThemeKeySpec *spec, const gchar *raw_value)
{
    GtkWidget *key_label, *type_label, *value_label, *default_label,
              *desc_label, *see_also_label,
              *color_btn, *int_spin, *reset_btn, *texture_grid;
    GString *see_also_markup;
    gint i;

    cur_spec = spec;

    key_label = get_widget("info_key_label");
    type_label = get_widget("info_type_label");
    value_label = get_widget("info_value_label");
    default_label = get_widget("info_default_label");
    desc_label = get_widget("info_desc_label");
    see_also_label = get_widget("info_see_also_label");
    color_btn = get_widget("edit_color_button");
    int_spin = get_widget("edit_int_spin");
    reset_btn = get_widget("edit_reset_button");
    texture_grid = get_widget("texture_edit_grid");

    populating = TRUE;

    if (!spec) {
        gtk_label_set_text(GTK_LABEL(key_label), "");
        gtk_label_set_text(GTK_LABEL(type_label), "");
        gtk_label_set_text(GTK_LABEL(value_label), "");
        gtk_label_set_text(GTK_LABEL(default_label), "");
        gtk_label_set_text(GTK_LABEL(desc_label), "Select an element on the left to see its documentation.");
        gtk_label_set_text(GTK_LABEL(see_also_label), "");
        gtk_widget_hide(color_btn);
        gtk_widget_hide(int_spin);
        gtk_widget_hide(reset_btn);
        gtk_widget_hide(texture_grid);
        populating = FALSE;
        return;
    }

    gtk_label_set_text(GTK_LABEL(key_label), spec->key);
    gtk_label_set_text(GTK_LABEL(type_label), type_name(spec->type));
    gtk_label_set_text(GTK_LABEL(value_label),
                        raw_value ? raw_value : "(using default)");
    gtk_label_set_text(GTK_LABEL(default_label), spec->default_str);
    gtk_label_set_text(GTK_LABEL(desc_label), spec->description);

    see_also_markup = g_string_new(NULL);
    for (i = 0; i < 4 && spec->see_also[i]; ++i) {
        if (i > 0)
            g_string_append(see_also_markup, ", ");
        if (schema_find(spec->see_also[i])) {
            gchar *esc = g_markup_escape_text(spec->see_also[i], -1);
            g_string_append_printf(see_also_markup, "<a href=\"%s\">%s</a>",
                                    esc, esc);
            g_free(esc);
        } else {
            gchar *esc = g_markup_escape_text(spec->see_also[i], -1);
            g_string_append(see_also_markup, esc);
            g_free(esc);
        }
    }
    gtk_label_set_markup(GTK_LABEL(see_also_label), see_also_markup->str);
    g_string_free(see_also_markup, TRUE);

    gtk_widget_hide(color_btn);
    gtk_widget_hide(int_spin);
    gtk_widget_hide(reset_btn);
    gtk_widget_hide(texture_grid);

    if (current_doc && spec->type == TK_TEXTURE) {
        gtk_widget_show(texture_grid);
        populate_texture_widgets(spec->key);
    }

    if (current_doc && (spec->type == TK_COLOR || spec->type == TK_INT)) {
        gtk_widget_show(reset_btn);

        if (spec->type == TK_COLOR) {
            GdkRGBA rgba;
            const gchar *raw = raw_value ? raw_value : spec->default_str;
            gchar *v = normalize_color_string(raw);
            if (!gdk_rgba_parse(&rgba, v))
                gdk_rgba_parse(&rgba, "black");
            g_free(v);
            gtk_color_button_set_rgba(GTK_COLOR_BUTTON(color_btn), &rgba);
            gtk_widget_show(color_btn);
        } else { /* TK_INT */
            const gchar *v = raw_value;
            gint iv;
            gtk_spin_button_set_range(GTK_SPIN_BUTTON(int_spin),
                                       spec->int_min, spec->int_max);
            gtk_spin_button_set_increments(GTK_SPIN_BUTTON(int_spin), 1, 10);
            if (!v || !g_ascii_isdigit(v[0] == '-' ? v[1] : v[0]))
                iv = atoi(spec->default_str);
            else
                iv = atoi(v);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(int_spin), iv);
            gtk_widget_show(int_spin);
        }
    }

    populating = FALSE;
}
