/* main.c for ObTheme, a theme editor for the Openbox window manager
   Copyright (c) 2026  NetLinux
   Documentation text in schema.c is adapted, with credit, from the
   original ObTheme by Xyne (2009) and its ohitsdylan/l-4-l forks,
   credited to ikem-krueger. This is a full rewrite in C/GTK3; no
   code from those projects is reused, only reference documentation.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License (version 2) as
   published by the Free Software Foundation.
*/

#include "main.h"
#include "theme_browser.h"
#include "preview.h"
#include "themerc.h"
#include "element_list.h"
#include "info_panel.h"
#include <obrender/render.h>
#include <obt/paths.h>
#include <gdk/gdkx.h>
#include <stdlib.h>

GtkBuilder *builder = NULL;
ObtPaths *paths = NULL;
RrInstance *rrinst = NULL;
ThemeDoc *current_doc = NULL;

static const gchar *DEFAULT_TITLELAYOUT = "NDLSIMCOY";

void on_main_window_destroy(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

void refresh_preview(void)
{
    GtkWidget *image;
    GdkPixbuf *preview;

    if (!current_doc)
        return;

    preview = preview_theme_window(current_doc->theme_dir,
                                    DEFAULT_TITLELAYOUT);
    image = get_widget("preview_image");
    if (preview) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(image), preview);
        g_object_unref(preview);
    } else {
        gtk_image_clear(GTK_IMAGE(image));
    }
}

void on_theme_selection_changed(GtkTreeSelection *sel, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *name, *dir;
    GtkWidget *dir_label;
    GError *error = NULL;

    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &name, 1, &dir, -1);

    dir_label = get_widget("theme_dir_label");
    gtk_label_set_text(GTK_LABEL(dir_label), dir);

    if (current_doc) {
        themerc_free(current_doc);
        current_doc = NULL;
    }
    current_doc = themerc_load(dir, &error);
    if (!current_doc) {
        g_printerr("Failed to load themerc for %s: %s\n", dir,
                   error->message);
        g_error_free(error);
    }

    refresh_preview();

    /* re-run the element list's own selection handler against the
       newly-loaded doc so the info panel reflects the new theme's
       value for whatever element is currently selected */
    {
        GtkTreeSelection *esel =
            gtk_tree_view_get_selection(
                GTK_TREE_VIEW(get_widget("element_list_view")));
        on_element_selection_changed(esel, NULL);
    }

    g_free(name);
    g_free(dir);
}

static void populate_theme_list(void)
{
    GtkListStore *store;
    GList *themes, *it;

    store = GTK_LIST_STORE(get_widget("theme_list_store"));
    themes = theme_browser_scan();

    for (it = themes; it; it = g_list_next(it)) {
        ThemeBrowserEntry *e = it->data;
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, e->name, 1, e->dir, -1);
    }

    theme_browser_free_list(themes);
}

int main(int argc, char **argv)
{
    GError *error = NULL;
    GtkWidget *mainwin;
    gchar *ui_path;

    gtk_init(&argc, &argv);

    paths = obt_paths_new();
    rrinst = RrInstanceNew(gdk_x11_get_default_xdisplay(),
                            gdk_x11_get_default_screen());

    ui_path = g_build_filename(RESOURCEDIR, "obtheme.ui", NULL);
    if (!g_file_test(ui_path, G_FILE_TEST_EXISTS)) {
        g_free(ui_path);
        ui_path = g_strdup("src/obtheme.ui");
    }

    builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(builder, ui_path, &error)) {
        g_printerr("Failed to load %s: %s\n", ui_path, error->message);
        g_error_free(error);
        g_free(ui_path);
        return 1;
    }
    g_free(ui_path);

    gtk_builder_connect_signals(builder, NULL);

    element_list_populate();
    info_panel_show(NULL, NULL);
    populate_theme_list();

    mainwin = get_widget("main_window");
    gtk_widget_show_all(mainwin);

    gtk_main();

    if (current_doc)
        themerc_free(current_doc);
    RrInstanceFree(rrinst);
    g_object_unref(builder);
    obt_paths_unref(paths);
    return 0;
}
