#ifndef __obtheme_main_h
#define __obtheme_main_h

#include <gtk/gtk.h>
#include <obt/paths.h>
#include <obrender/render.h>

extern GtkBuilder *builder;
extern ObtPaths *paths;
extern RrInstance *rrinst;

#define get_widget(s) GTK_WIDGET(gtk_builder_get_object(builder, s))

#endif
