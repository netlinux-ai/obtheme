#ifndef __obtheme_info_panel_h
#define __obtheme_info_panel_h

#include "schema.h"
#include <gtk/gtk.h>

gboolean on_info_see_also_activate_link(GtkLabel *label, const gchar *uri,
                                         gpointer data);
void on_edit_color_set(GtkColorButton *w, gpointer data);
void on_edit_int_changed(GtkSpinButton *w, gpointer data);
void on_edit_reset_clicked(GtkButton *w, gpointer data);

/* Updates the value/type/default/description/see-also widgets for spec
   (may be NULL to clear the panel), using raw_value (may be NULL if the
   key isn't explicitly present in the current theme's themerc). */
void info_panel_show(const ThemeKeySpec *spec, const gchar *raw_value);

#endif
