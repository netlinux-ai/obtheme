#ifndef __obtheme_themerc_h
#define __obtheme_themerc_h

#include <glib.h>

/* One line from a themerc file. Comments and wildcard-pattern lines
   (e.g. "window.*.button.*.bg: ...") are kept as opaque, verbatim
   passthrough so they survive a load->edit->save round trip untouched
   unless the specific element they'd otherwise supply is itself
   edited (see themerc_set, which "promotes" a wildcard-covered key to
   an explicit canonical entry without touching the wildcard line). */
typedef struct {
    gchar *raw;       /* verbatim source line, no trailing newline */
    gboolean is_data;  /* FALSE for comments/blank lines: passthrough only */
    gboolean is_wildcard; /* TRUE if is_data and the key contains '*' */
    gchar *key;        /* only set if is_data && !is_wildcard */
    gchar *value;       /* only set if is_data && !is_wildcard (raw, unparsed) */
} ThemeRcLine;

struct _ThemeDoc {
    gchar *theme_dir;      /* "<themedir>/<name>", parent of openbox-3/ */
    GList *lines;            /* ordered list of ThemeRcLine*, as read */
    GHashTable *canonical;   /* key (gchar*) -> ThemeRcLine* (owned by lines) */
};
typedef struct _ThemeDoc ThemeDoc;

ThemeDoc *themerc_load(const gchar *theme_dir, GError **error);
void themerc_free(ThemeDoc *doc);

/* Raw string value for key, or NULL if not explicitly present (caller
   should fall back to the ThemeKeySpec default in that case). Does not
   resolve wildcard-covered keys to a synthesized value in Phase 2 (read-
   only inspector) -- that resolution is added in Phase 3 alongside the
   writer's promotion logic. */
const gchar *themerc_get(ThemeDoc *doc, const gchar *key);

#endif
