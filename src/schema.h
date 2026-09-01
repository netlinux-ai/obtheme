#ifndef __obtheme_schema_h
#define __obtheme_schema_h

#include <glib.h>

typedef enum {
    TK_INT,
    TK_COLOR,
    TK_TEXTURE,
    TK_JUSTIFY,
    TK_FONT_SHADOW,
    TK_STRING
} ThemeKeyType;

typedef struct {
    const char *key;
    ThemeKeyType type;
    const char *default_str;
    int int_min;
    int int_max;
    const char *description;
    const char *see_also[4];
} ThemeKeySpec;

extern const ThemeKeySpec THEME_SCHEMA[];
extern const guint THEME_SCHEMA_COUNT;

const ThemeKeySpec *schema_find(const char *key);

#endif
