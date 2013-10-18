include ../include.mk

dist_extension_DATA = convenience.js extension.js stylesheet.css \
	metadata.json prefs.js

nobase_extension_DATA = \
	icons/nextwall.svg \
	icons/nextwall-symbolic.svg

gschemas_in = $(gschemabase).$(extensionname).gschema.xml

EXTRA_DIST = $(nobase_extension_DATA) $(gschemas_in)
CLEANFILES = $(gschemas_in:.xml=.valid)

# gsettings_SCHEMAS is a list of all the schemas you want to install
gsettings_SCHEMAS = $(gschemas_in)

# include the appropriate makefile rules for schema handling
@GSETTINGS_RULES@

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

