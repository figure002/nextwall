const St = imports.gi.St;
const Clutter = imports.gi.Clutter;
const Main = imports.ui.main;
const Lang = imports.lang;
const PanelMenu = imports.ui.panelMenu;
const PopupMenu = imports.ui.popupMenu;
const Gio = imports.gi.Gio;
const GLib = imports.gi.GLib;
const GnomeDesktop = imports.gi.GnomeDesktop;
const Util = imports.misc.util;

const ExtensionUtils = imports.misc.extensionUtils;
const Me = ExtensionUtils.getCurrentExtension();
const Convenience = Me.imports.convenience;

let nextwallMenu;
let panelIcon;
let wallpaperFile;
let wallpaperInfo;

/* Box for displaying current wallpaper info */
function WallpaperInfoBox() {
    this._init.apply(this, arguments);
}

WallpaperInfoBox.prototype = {
    __proto__: PopupMenu.PopupBaseMenuItem.prototype,

    _init: function(itemParams) {
        PopupMenu.PopupBaseMenuItem.prototype._init.call(this, {reactive: false});

        // Menu box for wallpaper info.
        let box = new St.BoxLayout({style_class: 'info-box'});
        box.set_vertical(true);
        this.addActor(box);

        // Add a bin for the current wallpaper info.
        this._currentWallpaperBin = new St.Bin({ style_class: 'current-wallpaper-bin' });
        box.add(this._currentWallpaperBin);

        // Thumbnail factory
        this.thumbnail_factory = new GnomeDesktop.DesktopThumbnailFactory();

        // Thumbnail container
        this._thumbnailBin = new St.Bin({
            style_class: 'thumbnail-bin'
        });

        // Default thumbnail
        this._default_thumb = new St.Icon({
            icon_size: 64,
            icon_name: 'image-missing',
        });

        // Build the UI.
        this.buildUI();
        this.refreshInfo();
    },

    buildUI: function() {
        // Create a box for the wallpaper thumbnail + labels
        let box = new St.BoxLayout({
            vertical: false,
            style_class: 'current-wallpaper-box'
        });
        let tbl = new St.BoxLayout({style_class: 'thumbnail-box'});
        tbl.add_actor(this._thumbnailBin);
        box.add_actor(tbl);

        // Wallpaper labels
        this._currentWallpaperFolder = new St.Label({ text: '...' });
        this._currentWallpaperName = new St.Label({ text: '...' });

        let databox = new St.BoxLayout({
            style_class: 'current-wallpaper-databox'
        });
        let databox_captions = new St.BoxLayout({
            vertical: true,
            style_class: 'current-wallpaper-databox-captions'
        });
        let databox_values = new St.BoxLayout({
            vertical: true,
            style_class: 'current-wallpaper-databox-values'
        });
        databox.add_actor(databox_captions);
        databox.add_actor(databox_values);

        databox_captions.add_actor(new St.Label({text: 'Folder:'}));
        databox_values.add_actor(this._currentWallpaperFolder);
        databox_captions.add_actor(new St.Label({text: 'Name:'}));
        databox_values.add_actor(this._currentWallpaperName);

        box.add_actor(databox);

        this._currentWallpaperBin.set_child(box);
    },

    icon_type: function() {
        if (!this._settingsNextwall) {
            this.loadSettings();
        }

        if (this._settingsNextwall.get_boolean("symbolic-icons"))
            return "-symbolic";
        else
            return "";
    },

    loadSettings: function() {
        let that = this;
        this._settingsBackground = Convenience.getSettings("org.gnome.desktop.background");
        this._settingsBackgroundC = this._settingsBackground.connect("changed", function() {that.refreshInfo();});

        this._settingsNextwall = Convenience.getSettings("org.gnome.shell.extensions.nextwall");
        this._settingsNextwallC = this._settingsNextwall.connect("changed", function() {that.onNextwallSettingsChanged();});
    },

    refreshInfo: function() {
        let path = this._currentWallpaperPath;

        // Set global vars
        wallpaperFile = Gio.file_new_for_path(path);
        wallpaperInfo = wallpaperFile.query_info('standard::content-type', Gio.FileQueryInfoFlags.NONE, null);

        // Update the labels
        this._currentWallpaperFolder.text = path.substring(0, path.lastIndexOf('/') + 1);
        this._currentWallpaperName.text = path.substring(path.lastIndexOf('/') + 1);

        // Change thumbnail
        this._thumbnailPath = this.getThumbnailPath(this.thumbnail_factory, path);
        if (this._thumbnailPath && GLib.file_test(this._thumbnailPath, GLib.FileTest.EXISTS)) {
            let thumbTexture = new Clutter.Texture({filter_quality: 2, filename: this._thumbnailPath});
            let [thumbWidth, thumbHeight] = thumbTexture.get_base_size();
            this._thumbnailBin.width = thumbWidth;
            this._thumbnailBin.height = thumbHeight;
            this._thumbnailBin.set_child(thumbTexture);
        }
        else {
            this._thumbnailBin.width = 64;
            this._thumbnailBin.height = 64;
            this._thumbnailBin.set_child(this._default_thumb);
        }
    },

    onNextwallSettingsChanged: function() {
        panelIcon.icon_name = 'nextwall' + this.icon_type();
    },

    get _currentWallpaperPath() {
        if (!this._settingsBackground) {
            this.loadSettings();
        }
        return this._settingsBackground.get_string("picture-uri").substring(7);
    },

    /* Return the thumbnail path for an image file.

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
        let info = file.query_info('standard::content-type', Gio.FileQueryInfoFlags.NONE, null);
        let mtime = info.get_modification_time();
        let mime_type = info.get_content_type();

        // Try to locate an existing thumbnail for the file specified. Returns
        // the absolute path of the thumbnail, or None if none exist.
        let path = factory.lookup(uri, mtime);
        if (path != null) {
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
        factory.save_thumbnail(thumbnail, uri, mtime)

        // Return the absolute path for the thumbnail.
        return factory.lookup(uri, mtime)
    },
};

/* Panel button */
function NextwallExtension() {
    this._init();
}

NextwallExtension.prototype = {
    __proto__: PanelMenu.Button.prototype,

    _init: function() {
        PanelMenu.Button.prototype._init.call(this, 0.0);

        this._settings = Convenience.getSettings();

        // Create a panel container.
        this.panelContainer = new St.BoxLayout();
        this.actor.add_actor(this.panelContainer);

        // Create the wallpaper info box.
        let info = new WallpaperInfoBox();

        // Add an icon to the panel container.
        panelIcon = new St.Icon({
            icon_name: 'nextwall' + info.icon_type(),
            icon_size: 16,
            style_class: 'panel-icon'
        });
        this.panelContainer.add(panelIcon);

        // Set the menu itens:

        // Add the info box.
        this.menu.addMenuItem(info);

        // -----
        let item = new PopupMenu.PopupSeparatorMenuItem();
        this.menu.addMenuItem(item);

        // Next Wallpaper
        item = new PopupMenu.PopupMenuItem("Next Wallpaper");
        item.connect('activate', Lang.bind(this, this._onNextWallpaper));
        this.menu.addMenuItem(item);

        // Open Wallpaper
        item = new PopupMenu.PopupMenuItem("Open Wallpaper");
        item.connect('activate', Lang.bind(this, this._onOpenWallpaper));
        this.menu.addMenuItem(item);

        // Show in File Manager
        item = new PopupMenu.PopupMenuItem("Show in File Manager");
        item.connect('activate', Lang.bind(this, this._onShowInFileManager));
        this.menu.addMenuItem(item);

        // Delete Wallpaper
        item = new PopupMenu.PopupMenuItem("Delete Wallpaper");
        item.connect('activate', Lang.bind(this, this._onDeleteWallpaper));
        this.menu.addMenuItem(item);

        // -----
        let item = new PopupMenu.PopupSeparatorMenuItem();
        this.menu.addMenuItem(item);

        // Fit Time of Day
        this.todSwitch = new PopupMenu.PopupSwitchMenuItem("Fit Time of Day",
            this._settings.get_boolean("fit-time"));
        this.todSwitch.connect('activate', Lang.bind(this, this._onFitTimeChange));
        this.menu.addMenuItem(this.todSwitch);

        // Settings
        item = new PopupMenu.PopupMenuItem("Settings");
        item.connect('activate', Lang.bind(this, this._onSettings));
        this.menu.addMenuItem(item);
    },

    _onFitTimeChange: function(widget, data) {
        this._settings.set_boolean("fit-time", widget.state);
    },

    _onNextWallpaper: function() {
        let wallpaper_path = this._settings.get_string("wallpaper-path");
        let current_location = this._settings.get_string("location");
        let command = 'nextwall';
        if (this.todSwitch.state) {
            command += ' -tl ' + current_location;
        }
        let path = Gio.file_new_for_path(wallpaper_path);
        let app = Gio.app_info_create_from_commandline(
            command,
            null,
            Gio.AppInfoCreateFlags.NONE,
            null);
        app.launch([path], null, null);
    },

    /* Open wallpaper with default file handler */
    _onOpenWallpaper: function() {
        if (wallpaperFile) {
            let app = wallpaperFile.query_default_handler(null, null);
            app.launch([wallpaperFile], null, null);
        }
    },

    /* Show wallpaper in file manager */
    _onShowInFileManager: function() {
        if (wallpaperFile) {
            let app = Gio.app_info_create_from_commandline('nautilus --no-desktop',
                    null, Gio.AppInfoCreateFlags.NONE, null);
            app.launch([wallpaperFile], null, null);
        }
    },

   /* Send file to trashcan */
    _onDeleteWallpaper: function() {
        if (wallpaperFile) {
            wallpaperFile.trash(null, null);
        }
    },

    _onScanWallpapers: function() {
        //TODO
    },

    /* Open extension preferences */
    _onSettings: function() {
        Util.spawn(["gnome-shell-extension-prefs", "nextwall@serrano.byobu.info"]);
        return 0;
    },

}

function init() {
}

function enable() {
    nextwallMenu = new NextwallExtension();
    Main.panel.addToStatusArea('nextwall', nextwallMenu, 0, 'right');
}

function disable() {
    nextwallMenu.destroy();
}
