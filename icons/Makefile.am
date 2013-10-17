## Makefile.am -- Process this file with automake to produce Makefile.in

themedir = $(datadir)/icons/hicolor

iconsscalabledir = $(themedir)/scalable/apps
iconsscalable_DATA = scalable/apps/nextwall.svg

icons48appsdir = $(themedir)/48x48/apps
icons48apps_DATA = 48x48/apps/nextwall.png

icons16statusdir = $(themedir)/16x16/status
icons16status_DATA = 16x16/status/nextwall.svg 16x16/status/nextwall-symbolic.svg

icons22statusdir = $(themedir)/22x22/status
icons22status_DATA = 22x22/status/nextwall.svg

icons24statusdir = $(themedir)/24x24/status
icons24status_DATA = 24x24/status/nextwall.svg

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

