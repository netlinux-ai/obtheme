#ifndef __obtheme_element_list_h
#define __obtheme_element_list_h

#include <gtk/gtk.h>

void on_element_selection_changed(GtkTreeSelection *sel, gpointer data);

void element_list_populate(void);
/* called from main.c once obtheme.ui is loaded */

/* selects the row for key in the element list, if it exists, causing
   the selection-changed handler to fire and update the info/prop
   panels -- used by info_panel's see-also cross-reference links */
void element_list_select_key(const gchar *key);

#endif
