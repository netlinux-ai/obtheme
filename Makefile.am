ACLOCAL_AMFLAGS = -I m4

# NOTE: hardcoded to this dev machine's sibling openbox install, matching
# the same dev-environment convention already used by _pkgconfig/*.pc.
# Revisit for real packaging.
themedir = /Data/openbox/_local-install/share/themes

bin_PROGRAMS = src/obtheme

src_obtheme_SOURCES = \
	src/main.c \
	src/main.h \
	src/schema.c \
	src/schema.h \
	src/theme_browser.c \
	src/theme_browser.h

src_obtheme_CPPFLAGS = \
	$(OPENBOX_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS) \
	-DRESOURCEDIR=\"$(datadir)/obtheme\" \
	-DTHEMEDIR_SYSTEM=\"$(themedir)\" \
	-DG_LOG_DOMAIN=\"Obtheme\"

src_obtheme_LDADD = \
	$(OPENBOX_LIBS) \
	$(GTK_LIBS) \
	$(GDK_PIXBUF_LIBS)

uidir = $(datadir)/obtheme
ui_DATA = src/obtheme.ui

EXTRA_DIST = $(ui_DATA) README FAQ.md COPYING
