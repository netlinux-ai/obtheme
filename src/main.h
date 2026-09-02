#ifndef __obtheme_main_h
#define __obtheme_main_h

#include <gtk/gtk.h>
#include <obt/paths.h>
#include <obrender/render.h>

typedef struct _ThemeDoc ThemeDoc; /* themerc.h */

extern GtkBuilder *builder;
extern ObtPaths *paths;
extern RrInstance *rrinst;
extern ThemeDoc *current_doc;

/* re-renders preview_image from current_doc's on-disk state; call
   after any themerc_save() so edits show up live */
void refresh_preview(void);

#define get_widget(s) GTK_WIDGET(gtk_builder_get_object(builder, s))

#endif
