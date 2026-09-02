/* element_list.c for ObTheme
   Copyright (c) 2026  NetLinux

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License (version 2) as
   published by the Free Software Foundation.
*/

#include "main.h"
#include "element_list.h"
#include "info_panel.h"
#include "themerc.h"
#include <string.h>

extern ThemeDoc *current_doc; /* main.c: the currently-open theme, or NULL */

static const gchar *type_name(ThemeKeyType t)
{
    switch (t) {
    case TK_INT:          return "integer";
    case TK_COLOR:         return "color";
    case TK_TEXTURE:       return "texture";
    case TK_JUSTIFY:        return "justification";
    case TK_FONT_SHADOW:   return "text shadow string";
    default:                return "string";
    }
}

void element_list_populate(void)
{
    GtkListStore *store;
    guint i;

    store = GTK_LIST_STORE(get_widget("element_list_store"));
    gtk_list_store_clear(store);

    for (i = 0; i < THEME_SCHEMA_COUNT; ++i) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                            0, THEME_SCHEMA[i].key,
                            1, type_name(THEME_SCHEMA[i].type),
                            -1);
    }
}

void element_list_select_key(const gchar *key)
{
    GtkTreeView *view;
    GtkTreeModel *model;
    GtkTreeIter iter;

    view = GTK_TREE_VIEW(get_widget("element_list_view"));
    model = gtk_tree_view_get_model(view);

    if (!gtk_tree_model_get_iter_first(model, &iter))
        return;

    do {
        gchar *rowkey;
        gtk_tree_model_get(model, &iter, 0, &rowkey, -1);
        if (!strcmp(rowkey, key)) {
            GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
            gtk_tree_view_set_cursor(view, path, NULL, FALSE);
            gtk_tree_view_scroll_to_cell(view, path, NULL, TRUE, 0.5, 0.0);
            gtk_tree_path_free(path);
            g_free(rowkey);
            return;
        }
        g_free(rowkey);
    } while (gtk_tree_model_iter_next(model, &iter));
}

void on_element_selection_changed(GtkTreeSelection *sel, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *key;
    const ThemeKeySpec *spec;
    const gchar *raw_value = NULL;

    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) {
        info_panel_show(NULL, NULL);
        return;
    }

    gtk_tree_model_get(model, &iter, 0, &key, -1);
    spec = schema_find(key);

    if (current_doc)
        raw_value = themerc_get(current_doc, key);

    info_panel_show(spec, raw_value);

    g_free(key);
}
