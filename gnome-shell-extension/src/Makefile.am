include ../include.mk

dist_extension_DATA = convenience.js extension.js stylesheet.css \
	metadata.json prefs.js

gschemas_in = $(gschemabase).$(extensionname).gschema.xml

EXTRA_DIST = $(gschemas_in)
CLEANFILES = $(gschemas_in:.xml=.valid)

# gsettings_SCHEMAS is a list of all the schemas you want to install
gsettings_SCHEMAS = $(gschemas_in)

# include the appropriate makefile rules for schema handling
@GSETTINGS_RULES@


