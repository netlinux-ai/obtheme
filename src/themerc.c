/* themerc.c for ObTheme
   Copyright (c) 2026  NetLinux

   Hand-rolled themerc reader. Deliberately not XrmDatabase-based: this
   editor resolves and writes back one canonical key at a time (as
   selected in the element list), and XrmDatabase gives no way to map a
   resolved value back to the specific line that supplied it, nor does
   XrmPutFileDatabase preserve comments/ordering on write. See the
   project plan for the full rationale.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License (version 2) as
   published by the Free Software Foundation.
*/

#include "themerc.h"
#include <string.h>
#include <stdio.h>

static ThemeRcLine *parse_line(const gchar *raw)
{
    ThemeRcLine *l = g_new0(ThemeRcLine, 1);
    const gchar *colon;
    gchar *stripped;

    l->raw = g_strdup(raw);

    stripped = g_strdup(raw);
    g_strstrip(stripped);

    if (stripped[0] == '\0' || stripped[0] == '!' || stripped[0] == '#') {
        l->is_data = FALSE;
        g_free(stripped);
        return l;
    }

    colon = strchr(stripped, ':');
    if (!colon) {
        /* malformed line -- keep as opaque passthrough rather than
           dropping it */
        l->is_data = FALSE;
        g_free(stripped);
        return l;
    }

    l->is_data = TRUE;
    l->key = g_strndup(stripped, colon - stripped);
    g_strstrip(l->key);
    l->value = g_strdup(colon + 1);
    g_strstrip(l->value);
    l->is_wildcard = (strchr(l->key, '*') != NULL);

    g_free(stripped);
    return l;
}

static void free_line(ThemeRcLine *l)
{
    g_free(l->raw);
    g_free(l->key);
    g_free(l->value);
    g_free(l);
}

ThemeDoc *themerc_load(const gchar *theme_dir, GError **error)
{
    gchar *path, *contents;
    gchar **rawlines;
    ThemeDoc *doc;
    gint i;

    path = g_build_filename(theme_dir, "openbox-3", "themerc", NULL);
    if (!g_file_get_contents(path, &contents, NULL, error)) {
        g_free(path);
        return NULL;
    }
    g_free(path);

    doc = g_new0(ThemeDoc, 1);
    doc->theme_dir = g_strdup(theme_dir);
    doc->canonical = g_hash_table_new(g_str_hash, g_str_equal);

    rawlines = g_strsplit(contents, "\n", -1);
    g_free(contents);

    for (i = 0; rawlines[i]; ++i) {
        ThemeRcLine *l = parse_line(rawlines[i]);
        doc->lines = g_list_append(doc->lines, l);
        if (l->is_data && !l->is_wildcard)
            g_hash_table_insert(doc->canonical, l->key, l);
    }
    g_strfreev(rawlines);

    return doc;
}

void themerc_free(ThemeDoc *doc)
{
    GList *it;

    if (!doc)
        return;

    for (it = doc->lines; it; it = g_list_next(it))
        free_line((ThemeRcLine *)it->data);
    g_list_free(doc->lines);
    g_hash_table_destroy(doc->canonical);
    g_free(doc->theme_dir);
    g_free(doc);
}

const gchar *themerc_get(ThemeDoc *doc, const gchar *key)
{
    ThemeRcLine *l = g_hash_table_lookup(doc->canonical, key);
    return l ? l->value : NULL;
}
