#ifndef __obtheme_preview_h
#define __obtheme_preview_h

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <obrender/render.h>
#include <obrender/theme.h>

/* Renders a preview of the theme at theme_dir (its "<name>" directory,
   parent of "openbox-3/themerc") — focused titlebar above unfocused
   titlebar, using titlelayout (e.g. "NDLSIMCOY"). Returns a new
   GdkPixbuf, or NULL if the theme fails to load. Caller g_object_unref()s
   the result. */
GdkPixbuf *preview_theme_window(const gchar *theme_dir,
                                 const gchar *titlelayout);

#endif
