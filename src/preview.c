/* preview.c for ObTheme
   Copyright (c) 2026  NetLinux

   Live theme-preview rendering technique adapted from ObConf's
   preview.c (preview_window/theme_pixmap_paint/theme_window_min_width),
   Copyright (c) 2003-2007 Dana Jansens, Copyright (c) 2003 Tim Riley,
   GPLv2, /Data/obconf/src/preview.c. Simplified to a single
   focused+unfocused titlebar pair (no menu gallery, no multi-theme
   thumbnail cache) since this app only ever previews the one theme
   currently open for editing.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License (version 2) as
   published by the Free Software Foundation.
*/

#include "main.h"
#include "preview.h"
#include <obrender/theme.h>
#include <gdk/gdkx.h>
#include <cairo-xlib.h>
#include <string.h>

#define GAP 4

static void theme_pixmap_paint(RrAppearance *a, gint w, gint h)
{
    Pixmap out = RrPaintPixmap(a, w, h);
    if (out) XFreePixmap(RrDisplay(a->inst), out);
}

static guint32 rr_color_pixel(const RrColor *c)
{
    return (guint32)((RrColorRed(c) << 24) + (RrColorGreen(c) << 16) +
                      (RrColorBlue(c) << 8) + 0xff);
}

static void blit(Display *xdisplay, Visual *xvisual, RrAppearance *a,
                  gint w, gint h, GdkPixbuf *dest, gint x, gint y)
{
    cairo_surface_t *surface;
    GdkPixbuf *tmp;

    theme_pixmap_paint(a, w, h);
    surface = cairo_xlib_surface_create(xdisplay, a->pixmap, xvisual, w, h);
    tmp = gdk_pixbuf_get_from_surface(surface, 0, 0, w, h);
    cairo_surface_destroy(surface);
    gdk_pixbuf_copy_area(tmp, 0, 0, w, h, dest, x, y);
    g_object_unref(tmp);
}

static gint theme_label_width(RrTheme *theme, gboolean active)
{
    RrAppearance *label;

    label = active ? theme->a_focused_label : theme->a_unfocused_label;
    label->texture[0].data.text.string = active ? "Active" : "Inactive";
    return RrMinWidth(label);
}

static gint theme_window_min_width(RrTheme *theme, const gchar *titlelayout)
{
    gint numbuttons = strlen(titlelayout);
    gint w = 2 * theme->fbwidth + (numbuttons + 3) * (theme->paddingx + 1);

    if (strchr(titlelayout, 'L')) {
        numbuttons--;
        w += MAX(theme_label_width(theme, TRUE),
                 theme_label_width(theme, FALSE));
    }
    w += theme->button_size * numbuttons;
    return w;
}

static GdkPixbuf *preview_window(RrTheme *theme, const gchar *titlelayout,
                                  gboolean focus, gint width, gint height,
                                  Display *xdisplay, Visual *xvisual)
{
    RrAppearance *title, *a;
    GdkPixbuf *pixbuf;
    gint w, h, x, y, label_w;
    const gchar *layout;

    title = focus ? theme->a_focused_title : theme->a_unfocused_title;

    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
    gdk_pixbuf_fill(pixbuf, rr_color_pixel(focus ?
                     theme->frame_focused_border_color :
                     theme->frame_unfocused_border_color));

    w = width - 2 * theme->fbwidth;
    h = theme->title_height;
    x = y = theme->fbwidth;
    blit(xdisplay, xvisual, title, w, h, pixbuf, x, y);

    label_w = width - (theme->paddingx + theme->fbwidth + 1) * 2;
    for (layout = titlelayout; *layout; layout++) {
        switch (*layout) {
        case 'N':
            label_w -= theme->button_size + 2 + theme->paddingx + 1;
            break;
        case 'D': case 'S': case 'I': case 'M': case 'C': case 'O': case 'Y':
            label_w -= theme->button_size + theme->paddingx + 1;
            break;
        default:
            break;
        }
    }

    x = theme->paddingx + theme->fbwidth + 1;
    y += theme->paddingy;
    for (layout = titlelayout; *layout; layout++) {
        if (*layout == 'N') {
            a = theme->a_icon;
            a->texture[0].type = RR_TEXTURE_RGBA;
            a->texture[0].data.rgba.width = 48;
            a->texture[0].data.rgba.height = 48;
            a->texture[0].data.rgba.alpha = 0xff;
            a->texture[0].data.rgba.data = theme->def_win_icon;
            a->surface.parent = title;
            a->surface.parentx = x - theme->fbwidth;
            a->surface.parenty = theme->paddingy;
            w = h = theme->button_size + 2;
            blit(xdisplay, xvisual, a, w, h, pixbuf, x, y);
            x += theme->button_size + 2 + theme->paddingx + 1;
        } else if (*layout == 'L') {
            a = focus ? theme->a_focused_label : theme->a_unfocused_label;
            a->texture[0].data.text.string = focus ? "Active" : "Inactive";
            a->surface.parent = title;
            a->surface.parentx = x - theme->fbwidth;
            a->surface.parenty = theme->paddingy;
            w = label_w;
            h = theme->label_height;
            blit(xdisplay, xvisual, a, w, h, pixbuf, x, y);
            x += w + theme->paddingx + 1;
        } else {
            switch (*layout) {
            case 'D':
                a = focus ? theme->btn_desk->a_focused_unpressed
                          : theme->btn_desk->a_unfocused_unpressed;
                break;
            case 'S':
                a = focus ? theme->btn_shade->a_focused_unpressed
                          : theme->btn_shade->a_unfocused_unpressed;
                break;
            case 'I':
                a = focus ? theme->btn_iconify->a_focused_unpressed
                          : theme->btn_iconify->a_unfocused_unpressed;
                break;
            case 'M':
                a = focus ? theme->btn_max->a_focused_unpressed
                          : theme->btn_max->a_unfocused_unpressed;
                break;
            case 'C':
                a = focus ? theme->btn_close->a_focused_unpressed
                          : theme->btn_close->a_unfocused_unpressed;
                break;
            case 'O':
                a = focus ? theme->btn_openbox_config->a_focused_unpressed
                          : theme->btn_openbox_config->a_unfocused_unpressed;
                break;
            case 'Y':
                a = focus ? theme->btn_layer->a_focused_unpressed
                          : theme->btn_layer->a_unfocused_unpressed;
                break;
            default:
                continue;
            }
            a->surface.parent = title;
            a->surface.parentx = x - theme->fbwidth;
            a->surface.parenty = theme->paddingy + 1;
            w = h = theme->button_size;
            blit(xdisplay, xvisual, a, w, h, pixbuf, x, y + 1);
            x += theme->button_size + theme->paddingx + 1;
        }
    }

    return pixbuf;
}

GdkPixbuf *preview_theme_window(const gchar *theme_dir,
                                 const gchar *titlelayout)
{
    RrTheme *theme;
    GdkPixbuf *preview, *win;
    GdkScreen *screen;
    Display *xdisplay;
    Visual *xvisual;
    gint w, h;

    theme = RrThemeNew(rrinst, theme_dir, FALSE,
                        NULL, NULL, NULL, NULL, NULL, NULL);
    if (!theme)
        return NULL;

    screen = gdk_screen_get_default();
    xdisplay = gdk_x11_get_default_xdisplay();
    xvisual = gdk_x11_visual_get_xvisual(gdk_screen_get_system_visual(screen));

    w = theme_window_min_width(theme, titlelayout);
    h = theme->fbwidth + theme->title_height + theme->fbwidth;

    preview = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, 2 * h + GAP);
    gdk_pixbuf_fill(preview, 0);

    win = preview_window(theme, titlelayout, TRUE, w, h, xdisplay, xvisual);
    gdk_pixbuf_copy_area(win, 0, 0, w, h, preview, 0, 0);
    g_object_unref(win);

    win = preview_window(theme, titlelayout, FALSE, w, h, xdisplay, xvisual);
    gdk_pixbuf_copy_area(win, 0, 0, w, h, preview, 0, h + GAP);
    g_object_unref(win);

    RrThemeFree(theme);
    return preview;
}
