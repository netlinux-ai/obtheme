#ifndef __obtheme_info_panel_h
#define __obtheme_info_panel_h

#include "schema.h"

/* Updates the value/type/default/description/see-also widgets for spec
   (may be NULL to clear the panel), using raw_value (may be NULL if the
   key isn't explicitly present in the current theme's themerc). */
void info_panel_show(const ThemeKeySpec *spec, const gchar *raw_value);

#endif
