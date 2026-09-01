/* theme_browser.c for ObTheme
   Copyright (c) 2026  NetLinux

   Theme-directory enumeration, ported from ObConf's theme.c
   (theme_load_all/add_theme_dir), Copyright (c) 2003-2007 Dana Jansens,
   Copyright (c) 2003 Tim Riley, GPLv2, /Data/obconf/src/theme.c.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License (version 2) as
   published by the Free Software Foundation.
*/

#include "main.h"
#include "theme_browser.h"
#include <obt/paths.h>
#include <string.h>

static void add_theme_dir(GList **themes, const gchar *dirname)
{
    GDir *dir;
    const gchar *n;

    if (!(dir = g_dir_open(dirname, 0, NULL)))
        return;

    while ((n = g_dir_read_name(dir))) {
        gchar *full = g_build_filename(dirname, n, "openbox-3", "themerc",
                                        NULL);
        if (g_file_test(full, G_FILE_TEST_IS_REGULAR |
                               G_FILE_TEST_IS_SYMLINK)) {
            ThemeBrowserEntry *e = g_new0(ThemeBrowserEntry, 1);
            e->name = g_strdup(n);
            e->dir = g_build_filename(dirname, n, NULL);
            *themes = g_list_append(*themes, e);
        }
        g_free(full);
    }
    g_dir_close(dir);
}

static gint compare_entry_name(gconstpointer a, gconstpointer b)
{
    const ThemeBrowserEntry *ea = a, *eb = b;
    return g_ascii_strcasecmp(ea->name, eb->name);
}

GList *theme_browser_scan(void)
{
    GList *themes = NULL;
    GSList *it;
    gchar *p;

    p = g_build_filename(g_get_home_dir(), ".themes", NULL);
    add_theme_dir(&themes, p);
    g_free(p);

    for (it = obt_paths_data_dirs(paths); it; it = g_slist_next(it)) {
        p = g_build_filename((gchar *)it->data, "themes", NULL);
        add_theme_dir(&themes, p);
        g_free(p);
    }

    add_theme_dir(&themes, THEMEDIR_SYSTEM);

    themes = g_list_sort(themes, compare_entry_name);

    /* drop duplicate names, keeping the first (highest-priority dir) */
    {
        GList *it2, *next;
        for (it2 = themes; it2; it2 = next) {
            next = g_list_next(it2);
            if (next) {
                ThemeBrowserEntry *cur = it2->data, *nx = next->data;
                if (!strcmp(cur->name, nx->name)) {
                    g_free(nx->name);
                    g_free(nx->dir);
                    g_free(nx);
                    themes = g_list_delete_link(themes, next);
                    next = it2;
                }
            }
        }
    }

    return themes;
}

void theme_browser_free_list(GList *list)
{
    GList *it;
    for (it = list; it; it = g_list_next(it)) {
        ThemeBrowserEntry *e = it->data;
        g_free(e->name);
        g_free(e->dir);
        g_free(e);
    }
    g_list_free(list);
}

void theme_browser_setup(void)
{
    /* wired up to the GtkTreeView + selection handler in Phase 1's
       preview integration; kept minimal here since the widget IDs are
       defined in obtheme.ui, not yet authored at this point in the
       build */
}
