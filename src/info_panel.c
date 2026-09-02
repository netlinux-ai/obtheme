/* info_panel.c for ObTheme
   Copyright (c) 2026  NetLinux

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License (version 2) as
   published by the Free Software Foundation.
*/

#include "main.h"
#include "info_panel.h"
#include "element_list.h"
#include <string.h>

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

void info_panel_show(const ThemeKeySpec *spec, const gchar *raw_value)
{
    GtkWidget *key_label, *type_label, *value_label, *default_label,
              *desc_label, *see_also_label;
    GString *see_also_markup;
    gint i;

    key_label = get_widget("info_key_label");
    type_label = get_widget("info_type_label");
    value_label = get_widget("info_value_label");
    default_label = get_widget("info_default_label");
    desc_label = get_widget("info_desc_label");
    see_also_label = get_widget("info_see_also_label");

    if (!spec) {
        gtk_label_set_text(GTK_LABEL(key_label), "");
        gtk_label_set_text(GTK_LABEL(type_label), "");
        gtk_label_set_text(GTK_LABEL(value_label), "");
        gtk_label_set_text(GTK_LABEL(default_label), "");
        gtk_label_set_text(GTK_LABEL(desc_label), "Select an element on the left to see its documentation.");
        gtk_label_set_text(GTK_LABEL(see_also_label), "");
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
        /* only render as a clickable link if it's a real schema key --
           a few see-also entries transcribed from the original
           documentation reference section headers ("titlebar colors")
           rather than actual keys */
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
}
