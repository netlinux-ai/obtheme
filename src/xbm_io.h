#ifndef __obtheme_xbm_io_h
#define __obtheme_xbm_io_h

#include <glib.h>

/* Reads path as an X11 XBM bitmap. On success, *w/*h are the bitmap
   dimensions and *bits is a newly g_malloc'd buffer, one byte per
   pixel (0 or 1), row-major, caller g_free()s. Returns FALSE (bits
   untouched) if the file doesn't exist or isn't a valid XBM. */
gboolean xbm_read(const gchar *path, guint *w, guint *h, guchar **bits);

/* Writes an XBM file from a w*h byte-per-pixel buffer (same layout as
   xbm_read's output). name is used as the XBM's internal C identifier
   prefix (cosmetic only). Returns FALSE on failure. */
gboolean xbm_write(const gchar *path, guint w, guint h, const guchar *bits,
                    const gchar *name);

/* One of the 7 titlebar buttons' editable bitmap slots (button +
   state, mapping to a "<button>[_state][_toggled].xbm" filename in a
   theme's openbox-3/ directory, per obrender/theme.c's
   read_button_styles()). */
typedef struct {
    const gchar *button;      /* "max", "close", "desk", "shade",
                                  "iconify", "openbox_config", "layer" */
    const gchar *state;       /* "" (unpressed), "pressed", "disabled",
                                  "hover" -- filename suffix, "" means
                                  no suffix before ".xbm" */
    gboolean toggled;         /* TRUE for the "_toggled" variant --
                                  only valid (per theme.c) for
                                  max/desk/shade */
    const gchar *description;
} ButtonBitmapSpec;

extern const ButtonBitmapSpec BUTTON_BITMAP_SCHEMA[];
extern const guint BUTTON_BITMAP_SCHEMA_COUNT;

/* Builds the "<button>[_state][_toggled].xbm" filename for spec
   (caller g_free()s). */
gchar *button_bitmap_filename(const ButtonBitmapSpec *spec);

#endif
