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
#include <glib/gstdio.h>
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

/* Component-wise Xrm-style resource matcher. A '*' component is a loose
   binding and may consume zero or more query components (this is the
   part flat-string globbing gets wrong: a pattern like
   "window.active.button.*.hover.bg" must match query
   "window.active.button.hover.bg" with '*' consuming zero components --
   a real theme, Onyx, relies on exactly this). Component comparison is
   case-insensitive, matching Xrm's tolerance for mixed name/class
   casing per component (e.g. Bear2's "border.Width"). */
static gboolean match_components(gchar **pat, guint pi, gchar **query, guint qi)
{
    if (pat[pi] == NULL)
        return query[qi] == NULL;

    if (strcmp(pat[pi], "*") == 0) {
        guint k = qi;
        for (;;) {
            if (match_components(pat, pi + 1, query, k))
                return TRUE;
            if (query[k] == NULL)
                return FALSE;
            ++k;
        }
    }

    if (query[qi] == NULL)
        return FALSE;
    if (g_ascii_strcasecmp(pat[pi], query[qi]) != 0)
        return FALSE;
    return match_components(pat, pi + 1, query, qi + 1);
}

static gint count_literal_components(gchar **pat)
{
    gint n = 0;
    guint i;
    for (i = 0; pat[i]; ++i)
        if (strcmp(pat[i], "*") != 0)
            ++n;
    return n;
}

const gchar *themerc_get(ThemeDoc *doc, const gchar *key)
{
    ThemeRcLine *l = g_hash_table_lookup(doc->canonical, key);
    ThemeRcLine *best;
    gint best_specificity;
    GList *it;
    gchar **qcomp;

    if (l)
        return l->value;

    /* No exact (case-sensitive) canonical line -- fall back to a full
       component-wise scan, which also catches canonical lines that
       differ only in per-component case (the hash-table fast path
       above is case-sensitive). Among all matches, prefer the one with
       the most literal (non-'*') components -- an exact match has every
       component literal, so it always outranks any wildcard line, no
       separate canonical-first-pass logic is needed here -- and break
       ties by taking the latest one in the file. */
    qcomp = g_strsplit(key, ".", -1);
    best = NULL;
    best_specificity = -1;
    for (it = doc->lines; it; it = g_list_next(it)) {
        ThemeRcLine *cand = it->data;
        gchar **pcomp;
        gint specificity;

        if (!cand->is_data)
            continue;

        pcomp = g_strsplit(cand->key, ".", -1);
        if (match_components(pcomp, 0, qcomp, 0)) {
            specificity = count_literal_components(pcomp);
            if (specificity >= best_specificity) {
                best_specificity = specificity;
                best = cand;
            }
        }
        g_strfreev(pcomp);
    }
    g_strfreev(qcomp);

    return best ? best->value : NULL;
}

/* Finds the canonical (non-wildcard) line for key, if any -- first by
   exact case (the fast hash-table path), then falling back to a
   case-insensitive scan (themerc authors sometimes mix component
   casing, e.g. Bear2's "border.Width"; see themerc_get). */
static ThemeRcLine *find_canonical_line(ThemeDoc *doc, const gchar *key)
{
    ThemeRcLine *l = g_hash_table_lookup(doc->canonical, key);
    GList *it;

    if (l)
        return l;

    for (it = doc->lines; it; it = g_list_next(it)) {
        ThemeRcLine *cand = it->data;
        if (cand->is_data && !cand->is_wildcard &&
            g_ascii_strcasecmp(cand->key, key) == 0)
            return cand;
    }
    return NULL;
}

void themerc_set(ThemeDoc *doc, const gchar *key, const gchar *value)
{
    ThemeRcLine *l = find_canonical_line(doc, key);

    if (l) {
        g_free(l->value);
        l->value = g_strdup(value);
        g_free(l->raw);
        l->raw = g_strdup_printf("%s: %s", l->key, l->value);
        return;
    }

    l = g_new0(ThemeRcLine, 1);
    l->is_data = TRUE;
    l->is_wildcard = FALSE;
    l->key = g_strdup(key);
    l->value = g_strdup(value);
    l->raw = g_strdup_printf("%s: %s", key, value);

    doc->lines = g_list_append(doc->lines, l);
    g_hash_table_insert(doc->canonical, l->key, l);
}

void themerc_unset(ThemeDoc *doc, const gchar *key)
{
    ThemeRcLine *l = find_canonical_line(doc, key);
    GList *node;

    if (!l)
        return;

    g_hash_table_remove(doc->canonical, l->key);
    node = g_list_find(doc->lines, l);
    if (node)
        doc->lines = g_list_delete_link(doc->lines, node);
    free_line(l);
}

gboolean themerc_save(ThemeDoc *doc, GError **error)
{
    gchar *path, *dir;
    GString *out;
    GList *it;
    gboolean ok;

    dir = g_build_filename(doc->theme_dir, "openbox-3", NULL);
    if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
        g_mkdir_with_parents(dir, 0755);
    g_free(dir);

    out = g_string_new(NULL);
    for (it = doc->lines; it; it = g_list_next(it)) {
        ThemeRcLine *l = it->data;
        g_string_append(out, l->raw);
        g_string_append_c(out, '\n');
    }

    path = g_build_filename(doc->theme_dir, "openbox-3", "themerc", NULL);
    ok = g_file_set_contents(path, out->str, out->len, error);
    g_free(path);
    g_string_free(out, TRUE);

    return ok;
}
