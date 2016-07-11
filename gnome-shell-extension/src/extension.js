const BoxPointer = imports.ui.boxpointer;
const Clutter = imports.gi.Clutter;
const Gio = imports.gi.Gio;
const GLib = imports.gi.GLib;
const GnomeDesktop = imports.gi.GnomeDesktop;
const Lang = imports.lang;
const Main = imports.ui.main;
const ModalDialog = imports.ui.modalDialog;
const PanelMenu = imports.ui.panelMenu;
const Pango = imports.gi.Pango;
const Params = imports.misc.params;
const PopupMenu = imports.ui.popupMenu;
const St = imports.gi.St;
const Util = imports.misc.util;

const ExtensionUtils = imports.misc.extensionUtils;
const Me = ExtensionUtils.getCurrentExtension();
const Convenience = Me.imports.convenience;

const BACKGROUND_SCHEMA = 'org.gnome.desktop.background';
const NEXTWALL_SCHEMA = 'org.gnome.shell.extensions.nextwall';
const THUMBNAIL_ICON_SIZE = 64;
const DIALOG_ICON_SIZE = 32;

let nextwallMenu;

/* Error dialog */
const ErrorDialog = new Lang.Class({
    Name: 'ErrorDialog',
    Extends: ModalDialog.ModalDialog,

    _init: function(message) {
        this.parent({ styleClass: 'message-dialog' });

        this.setButtons([{ label: "OK",
                           action: Lang.bind(this, this._onOkButtonPressed),
                           default: true
                         }]);

        let box = new St.BoxLayout();
        this.contentLayout.add(box);

        let icon = new St.Icon({
            icon_name: 'dialog-error-symbolic',
            icon_size: DIALOG_ICON_SIZE,
            style_class: 'dialog-icon'
        });
        box.add(icon);

        let label = new St.Label({ text: message });
        label.clutter_text.set_ellipsize(Pango.EllipsizeMode.NONE);
        label.clutter_text.set_line_wrap(true);
        label.clutter_text.set_line_wrap_mode(Pango.WrapMode.WORD_CHAR);
        box.add(label);
    },

    _onOkButtonPressed: function(button, event) {
        this.close(global.get_current_time());
    }
});

/* Delete wallpaper confirmation dialog */
const DeleteWallpaperDialog = new Lang.Class({
    Name: 'DeleteWallpaperDialog',
    Extends: ModalDialog.ModalDialog,

    _init: function(file) {
        this._file = file;

        this.parent({ styleClass: 'message-dialog' });

        this.setButtons([{ label: "Cancel",
                           action: Lang.bind(this, this._onCancelButtonPressed),
                           key: Clutter.Escape,
                           default: true
                         },
                         { label: "Yes",
                           action: Lang.bind(this, this._onYesButtonPressed)
                         }]);

        let box = new St.BoxLayout();
        this.contentLayout.add(box);

        let icon = new St.Icon({
            icon_name: 'dialog-question-symbolic',
            icon_size: DIALOG_ICON_SIZE,
            style_class: 'dialog-icon'
        });
        box.add(icon);

        let message = "Move the following wallpaper to trash?\n\n%s\n".format(file.get_path());

        let label = new St.Label({ text: message });
        label.clutter_text.set_ellipsize(Pango.EllipsizeMode.NONE);
        label.clutter_text.set_line_wrap(true);
        label.clutter_text.set_line_wrap_mode(Pango.WrapMode.WORD_CHAR);
        box.add(label);
    },

    _onCancelButtonPressed: function(button, event) {
        this.close(global.get_current_time());
    },

    _onYesButtonPressed: function(button, event) {
        this._file.trash(null, null);
        this.close(global.get_current_time());
        nextwallMenu.onNextWallpaper();
    }
});

/* The thumbnail widget displays the thumbnail for an image */
const ThumbnailWidget = new Lang.Class({
    Name: 'ThumbnailWidget',

    _init: function(image_path, params) {
        this._image_path = image_path;
        params = Params.parse(params, { reactive: false,
                                        iconSize: THUMBNAIL_ICON_SIZE,
                                        styleClass: 'thumbnail-widget' });
        this._iconSize = params.iconSize;

        this.actor = new St.Bin({ style_class: params.styleClass,
                                  track_hover: params.reactive,
                                  reactive: params.reactive });

        this._thumbnail_factory = new GnomeDesktop.DesktopThumbnailFactory();
    },

    setSensitive: function(sensitive) {
        this.actor.can_focus = sensitive;
        this.actor.reactive = sensitive;
    },

    update: function(image_path) {
        let thumbnail_path;

        if (!GLib.file_test(image_path, GLib.FileTest.EXISTS))
            image_path = null;
        else
            thumbnail_path = this.getThumbnailPath(this._thumbnail_factory, image_path);

        if (image_path && thumbnail_path) {
            if (GLib.file_test(thumbnail_path, GLib.FileTest.EXISTS)) {
                let thumbTexture = new Clutter.Texture({filter_quality: 2, filename: thumbnail_path});
                let [thumbWidth, thumbHeight] = thumbTexture.get_base_size();
                this.actor.width = thumbWidth;
                this.actor.height = thumbHeight;
                this.actor.set_child(thumbTexture);
            }
        }
        else {
            this.actor.style = null;
            this.actor.width = this._iconSize;
            this.actor.height = this._iconSize;
            this.actor.child = new St.Icon({
                icon_name: 'image-missing',
                icon_size: this._iconSize
            });
        }
    },

    /**
       Return the thumbnail path for an image file.

       Creates a thumbnail if it doesn't exist already and return the
       absolute path for the thumbnail. If a thumbnail can't be generated,
       return null.

       This method is based on code shared by `James Henstridge
       <http://askubuntu.com/users/12469/james-henstridge>` on
       `AskUbuntu.com <http://askubuntu.com/a/201997/303>`, licensed under the
       Creative Commons Attribution-ShareAlike 3.0 Unported license.

       @param GnomeDesktop.DesktopThumbnailFactory factory Instance of a GNOME
            thumbnail factory.
       @param string image_path Absolute path of an image file.
       @return Returns path to the thumbnail, null on error.
    */
    getThumbnailPath: function(factory, image_path) {
        // Use Gio to determine the URI and mime type
        let file = Gio.file_new_for_path(image_path);
        let uri = file.get_uri();
        let info = file.query_info('standard::content-type,time::modified', Gio.FileQueryInfoFlags.NONE, null);
        let mtimeval = info.get_modification_time();
        let mtime = mtimeval.tv_sec;
        let mime_type = info.get_content_type();

        // Try to locate an existing thumbnail for the file specified. Returns
        // the absolute path of the thumbnail, or None if none exist.
        let path = factory.lookup(uri, mtime);
        if (path !== null) {
            return path;
        }

        // Returns TRUE if GnomeIconFactory can (at least try) to thumbnail this
        // file.
        if (!factory.can_thumbnail(uri, mime_type, mtime)) {
            return null;
        }

        // Try to generate a thumbnail for the specified file. If it succeeds it
        // returns a pixbuf that can be used as a thumbnail.
        let thumbnail = factory.generate_thumbnail(uri, mime_type);
        if (!thumbnail) {
            return null;
        }

        // Save thumbnail at the right place. If the save fails a failed
        // thumbnail is written.
        factory.save_thumbnail(thumbnail, uri, mtime);

        // Return the absolute path for the thumbnail.
        return factory.lookup(uri, mtime);
    }
});

/* Box for displaying current wallpaper info */
const WallpaperInfoBox = new Lang.Class({
    Name: 'WallpaperInfoBox',
    Extends: PopupMenu.PopupBaseMenuItem,

    _init: function(itemParams) {
        this.parent({ reactive: false, can_focus: false });
        this.loadSettings();
        this.createBox();
        this.update();
    },

    loadSettings: function() {
        this._settingsBackground = Convenience.getSettings(BACKGROUND_SCHEMA);
        this._settingsBackground.connect('changed',
            Lang.bind(this, this.update));

        this._settingsNextwall = Convenience.getSettings(NEXTWALL_SCHEMA);
        this._settingsNextwall.connect('changed',
            Lang.bind(this, this.onNextwallSettingsChanged));
    },

    createBox: function() {
        this._thumbnail = new ThumbnailWidget(null, { reactive: true });
        this.thumbnailBin = new St.Button({
            child: this._thumbnail.actor,
            style_class: 'thumbnail-bin'
        });

        let box = new St.BoxLayout({style_class: 'nextwall-box'});
        box.add(this.thumbnailBin);
        this.actor.add(box);
    },

    update: function() {
        let path = this._currentWallpaperPath;
        this.wallpaperFile = Gio.file_new_for_path(path);
        this._thumbnail.update(path);
    },

    onNextwallSettingsChanged: function() {
        nextwallMenu.panelIcon.icon_name = 'nextwall' + this.icon_type;
    },

    get icon_type() {
        if (this._settingsNextwall.get_boolean('symbolic-icons'))
            return '-symbolic';
        else
            return '';
    },

    get _currentWallpaperPath() {
        return this._settingsBackground.get_string('picture-uri').substring(7);
    }
});

/* Panel button */
const NextwallMenuButton = new Lang.Class({
    Name: 'NextwallMenuButton',
    Extends: PanelMenu.Button,

    _init: function() {
        this.parent(0.0);

        // Get nextwall settings.
        this._settings = Convenience.getSettings(NEXTWALL_SCHEMA);

        // Create a panel container.
        this.panelContainer = new St.BoxLayout();
        this.actor.add_actor(this.panelContainer);

        // Add an icon to the panel container.
        this.panelIcon = new St.Icon({
            icon_name: 'nextwall' + this.icon_type,
            icon_size: 16,
            style_class: 'panel-icon'
        });
        this.panelContainer.add(this.panelIcon);

        // Create a wallpaper info box.
        this.infoBox = new WallpaperInfoBox();

        // Open the wallpaper when the thumbnail is clicked.
        this.infoBox.thumbnailBin.connect('clicked', Lang.bind(this,
            this._onOpenWallpaper));

        // Create the menu.
        this._createMenu();
    },

    _createMenu: function() {
        let item;

        this.menu.addMenuItem(this.infoBox);

        item = new PopupMenu.PopupSeparatorMenuItem();
        this.menu.addMenuItem(item);

        item = new PopupMenu.PopupMenuItem("Next Wallpaper");
        item.connect('activate', Lang.bind(this, this.onNextWallpaper));
        this.menu.addMenuItem(item);

        item = new PopupMenu.PopupMenuItem("Show in File Manager");
        item.connect('activate', Lang.bind(this, this._onShowInFileManager));
        this.menu.addMenuItem(item);

        item = new PopupMenu.PopupMenuItem("Delete Wallpaper");
        item.connect('activate', Lang.bind(this, this._onDeleteWallpaper));
        this.menu.addMenuItem(item);

        item = new PopupMenu.PopupSeparatorMenuItem();
        this.menu.addMenuItem(item);

        item = new PopupMenu.PopupSwitchMenuItem("Fit Time of Day",
            this._settings.get_boolean('fit-time'));
        item.connect('activate', Lang.bind(this, this._onFitTimeChange));
        this.menu.addMenuItem(item);

        item = new PopupMenu.PopupMenuItem("Settings");
        item.connect('activate', Lang.bind(this, this._onSettings));
        this.menu.addMenuItem(item);
    },

    get icon_type() {
        if (this._settings.get_boolean('symbolic-icons'))
            return '-symbolic';
        else
            return '';
    },

    _onFitTimeChange: function(widget, data) {
        this._settings.set_boolean('fit-time', widget.state);
    },

    onNextWallpaper: function() {
        let wallpaper_path = this._settings.get_string('wallpaper-path');
        let current_location = this._settings.get_string('location');
        let command = 'nextwall';
        if (this._settings.get_boolean('fit-time')) {
            command += ' -tl ' + current_location;
        }
        let path = Gio.file_new_for_path(wallpaper_path);

        let app = Gio.app_info_create_from_commandline(command, null,
            Gio.AppInfoCreateFlags.NONE, null);

        try {
            app.launch([path], null, null);
        }
        catch(err) {
            log(err.message);
            let message = "Failed to launch Nextwall. Please make sure that Nextwall is installed correctly.\n\nThe following error message was returned:\n%s".format(err.message);
            let dialog = new ErrorDialog(message);
            dialog.open(global.get_current_time());
        }
    },

    /* Open wallpaper with default file handler */
    _onOpenWallpaper: function() {
        if (this.infoBox.wallpaperFile) {
            this.menu.close(BoxPointer.PopupAnimation.FULL);
            Main.overview.hide();
            let app = this.infoBox.wallpaperFile.query_default_handler(null, null);

            try {
                app.launch([this.infoBox.wallpaperFile], null, null);
            }
            catch(err) {
                log(err.message);
                let dialog = new ErrorDialog(err.message);
                dialog.open(global.get_current_time());
            }
        }
    },

    /* Show wallpaper in file manager */
    _onShowInFileManager: function() {
        if (this.infoBox.wallpaperFile) {
            // Get the default file manager
            let dir = Gio.file_new_for_path('/');
            let app = dir.query_default_handler(null, null);

            try {
                app.launch([this.infoBox.wallpaperFile], null, null);
            }
            catch(err) {
                log(err.message);
                let dialog = new ErrorDialog(err.message);
                dialog.open(global.get_current_time());
            }
        }
    },

   /* Send file to trashcan */
    _onDeleteWallpaper: function() {
        if (this.infoBox.wallpaperFile) {
            let dialog = new DeleteWallpaperDialog(this.infoBox.wallpaperFile);
            dialog.open(global.get_current_time());
        }
    },

    /* Open extension preferences */
    _onSettings: function() {
        Util.spawn(['gnome-shell-extension-prefs', 'nextwall@serrano.byobu.info']);
        return 0;
    }
});

function init(extensionMeta) {
    let theme = imports.gi.Gtk.IconTheme.get_default();
    theme.append_search_path(extensionMeta.path + "/icons");
}

function enable() {
    nextwallMenu = new NextwallMenuButton();
    Main.panel.addToStatusArea('nextwall', nextwallMenu, 0, 'right');
}

function disable() {
    Main.panel.menuManager.removeMenu(nextwallMenu.menu);
    nextwallMenu.destroy();
    nextwallMenu = null;
}
