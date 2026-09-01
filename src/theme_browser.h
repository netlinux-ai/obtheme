#ifndef __obtheme_theme_browser_h
#define __obtheme_theme_browser_h

#include <glib.h>

/* one entry per installed theme: display name + full "<dir>/<name>" path
   (the parent of "openbox-3/themerc") */
typedef struct {
    gchar *name;
    gchar *dir;
} ThemeBrowserEntry;

void theme_browser_setup(void);
/* re-scan installed themes; returns a newly-allocated, name-sorted,
   duplicate-name-free list of ThemeBrowserEntry* (caller frees with
   theme_browser_free_list) */
GList *theme_browser_scan(void);
void theme_browser_free_list(GList *list);

#endif
