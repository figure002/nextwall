include include.mk

dist_extension_DATA = src/extension.js src/stylesheet.css src/metadata.json

zip-file: all
		-rm -fR $(builddir)/_build
		-rm -fR $(builddir)/zip-files
		mkdir $(builddir)/_build; \
		mkdir $(builddir)/zip-files; \
		$(MAKE) install DESTDIR="$(abs_builddir)/_build"; \
		cp -r "$(builddir)/_build$(extensiondir)" "$(builddir)/_build/"; \
		(cd "$(builddir)/_build/$(uuid)"; \
				zip -qr "$(abs_builddir)/zip-files/$(uuid).zip" .; \
		);

install-zip: zip-file
		mkdir -p $(localextensiondir) && unzip -o $(abs_builddir)/zip-files/$(uuid).zip -d $(localextensiondir);

uninstall-zip: Makefile
		-rm -fR $(localextensiondir)
