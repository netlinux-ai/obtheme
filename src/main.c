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
#include <obrender/render.h>
#include <obt/paths.h>
#include <gdk/gdkx.h>
#include <stdlib.h>

GtkBuilder *builder = NULL;
ObtPaths *paths = NULL;

void on_main_window_destroy(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

int main(int argc, char **argv)
{
    GError *error = NULL;
    GtkWidget *mainwin;
    gchar *ui_path;

    gtk_init(&argc, &argv);

    paths = obt_paths_new();

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

    mainwin = get_widget("main_window");
    gtk_widget_show_all(mainwin);

    gtk_main();

    g_object_unref(builder);
    obt_paths_unref(paths);
    return 0;
}
