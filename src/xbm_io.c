/* xbm_io.c for ObTheme
   Copyright (c) 2026  NetLinux

   Reading uses Xlib's XReadBitmapFileData directly -- this is exactly
   what obrender/theme.c's read_mask() uses to load button icon
   bitmaps, so using the same call guarantees byte-identical read
   semantics to what the real renderer will load.

   Writing hand-rolls the well-known plain-text XBM format instead of
   calling Xlib's XWriteBitmapFile (which requires first uploading the
   bits into a server-side Pixmap via XPutImage -- unnecessary
   complexity here since the text format itself is simple and is
   exactly what XReadBitmapFileData parses back on the next load).

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License (version 2) as
   published by the Free Software Foundation.
*/

#include "xbm_io.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <ctype.h>

const ButtonBitmapSpec BUTTON_BITMAP_SCHEMA[] = {
    { "max", "", FALSE, "Maximize button, normal state" },
    { "max", "pressed", FALSE, "Maximize button, pressed" },
    { "max", "disabled", FALSE, "Maximize button, disabled" },
    { "max", "hover", FALSE, "Maximize button, mouse over" },
    { "max", "", TRUE, "Maximize button, toggled (window is maximized)" },
    { "max", "pressed", TRUE, "Maximize button, toggled + pressed" },
    { "max", "hover", TRUE, "Maximize button, toggled + mouse over" },

    { "close", "", FALSE, "Close button, normal state" },
    { "close", "pressed", FALSE, "Close button, pressed" },
    { "close", "disabled", FALSE, "Close button, disabled" },
    { "close", "hover", FALSE, "Close button, mouse over" },

    { "desk", "", FALSE, "Omnipresent (all desktops) button, normal state" },
    { "desk", "pressed", FALSE, "Omnipresent button, pressed" },
    { "desk", "disabled", FALSE, "Omnipresent button, disabled" },
    { "desk", "hover", FALSE, "Omnipresent button, mouse over" },
    { "desk", "", TRUE, "Omnipresent button, toggled (on all desktops)" },
    { "desk", "pressed", TRUE, "Omnipresent button, toggled + pressed" },
    { "desk", "hover", TRUE, "Omnipresent button, toggled + mouse over" },

    { "shade", "", FALSE, "Shade button, normal state" },
    { "shade", "pressed", FALSE, "Shade button, pressed" },
    { "shade", "disabled", FALSE, "Shade button, disabled" },
    { "shade", "hover", FALSE, "Shade button, mouse over" },
    { "shade", "", TRUE, "Shade button, toggled (window is shaded)" },
    { "shade", "pressed", TRUE, "Shade button, toggled + pressed" },
    { "shade", "hover", TRUE, "Shade button, toggled + mouse over" },

    { "iconify", "", FALSE, "Iconify (minimize) button, normal state" },
    { "iconify", "pressed", FALSE, "Iconify button, pressed" },
    { "iconify", "disabled", FALSE, "Iconify button, disabled" },
    { "iconify", "hover", FALSE, "Iconify button, mouse over" },

    { "openbox_config", "", FALSE, "Openbox Config button, normal state" },
    { "openbox_config", "pressed", FALSE, "Openbox Config button, pressed" },
    { "openbox_config", "disabled", FALSE, "Openbox Config button, disabled" },
    { "openbox_config", "hover", FALSE, "Openbox Config button, mouse over" },

    { "layer", "", FALSE, "Layer button, normal state" },
    { "layer", "pressed", FALSE, "Layer button, pressed" },
    { "layer", "disabled", FALSE, "Layer button, disabled" },
    { "layer", "hover", FALSE, "Layer button, mouse over" },
};
const guint BUTTON_BITMAP_SCHEMA_COUNT = G_N_ELEMENTS(BUTTON_BITMAP_SCHEMA);

gchar *button_bitmap_filename(const ButtonBitmapSpec *spec)
{
    GString *s = g_string_new(spec->button);
    if (spec->state[0])
        g_string_append_printf(s, "_%s", spec->state);
    if (spec->toggled)
        g_string_append(s, "_toggled");
    g_string_append(s, ".xbm");
    return g_string_free(s, FALSE);
}

gboolean xbm_read(const gchar *path, guint *w, guint *h, guchar **bits)
{
    unsigned int width, height;
    unsigned char *data;
    int hot_x, hot_y;
    Display *d;
    guint row_bytes, x, y;
    guchar *out;

    d = XOpenDisplay(NULL);
    if (!d)
        return FALSE;

    if (XReadBitmapFileData(path, &width, &height, &data,
                             &hot_x, &hot_y) != BitmapSuccess) {
        XCloseDisplay(d);
        return FALSE;
    }
    XCloseDisplay(d);

    row_bytes = (width + 7) / 8;
    out = g_malloc(width * height);
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            guchar byte = data[y * row_bytes + x / 8];
            out[y * width + x] = (byte >> (x % 8)) & 1;
        }
    }
    XFree(data);

    *w = width;
    *h = height;
    *bits = out;
    return TRUE;
}

static gchar *sanitize_ident(const gchar *name)
{
    gchar *s = g_strdup(name);
    gchar *p;
    for (p = s; *p; ++p)
        if (!isalnum((guchar)*p))
            *p = '_';
    return s;
}

gboolean xbm_write(const gchar *path, guint w, guint h, const guchar *bits,
                    const gchar *name)
{
    GString *out;
    gchar *ident;
    guint row_bytes, x, y, i, total;
    gboolean ok;
    GError *error = NULL;

    ident = sanitize_ident(name);
    row_bytes = (w + 7) / 8;
    total = row_bytes * h;

    out = g_string_new(NULL);
    g_string_append_printf(out, "#define %s_width %u\n", ident, w);
    g_string_append_printf(out, "#define %s_height %u\n", ident, h);
    g_string_append_printf(out, "static unsigned char %s_bits[] = {\n  ",
                            ident);

    i = 0;
    for (y = 0; y < h; ++y) {
        guchar byte = 0;
        for (x = 0; x < w; ++x) {
            if (bits[y * w + x])
                byte |= (1 << (x % 8));
            if (x % 8 == 7 || x == w - 1) {
                g_string_append_printf(out, "0x%02x", byte);
                ++i;
                if (i < total)
                    g_string_append(out, ", ");
                if (i % 12 == 0)
                    g_string_append(out, "\n  ");
                byte = 0;
            }
        }
    }

    g_string_append(out, "};\n");

    ok = g_file_set_contents(path, out->str, out->len, &error);
    if (!ok) {
        g_printerr("Failed to write %s: %s\n", path, error->message);
        g_error_free(error);
    }

    g_string_free(out, TRUE);
    g_free(ident);
    return ok;
}
