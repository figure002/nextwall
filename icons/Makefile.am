## Makefile.am -- Process this file with automake to produce Makefile.in

icondir = $(datadir)/icons/hicolor

nobase_icon_DATA = \
	16x16/status/nextwall.svg \
	16x16/status/nextwall-symbolic.svg \
	22x22/status/nextwall.svg \
	24x24/status/nextwall.svg \
	48x48/apps/nextwall.png \
	scalable/apps/nextwall.svg

EXTRA_DIST = $(nobase_icon_DATA)

# Post-install hooks for updating the GTK icon cache
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor/

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

