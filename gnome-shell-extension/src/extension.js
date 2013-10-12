const St = imports.gi.St;
const Main = imports.ui.main;
const Lang = imports.lang;
const PanelMenu = imports.ui.panelMenu;
const PopupMenu = imports.ui.popupMenu;
const Tweener = imports.ui.tweener;

let nextwallMenu;

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
        this._currentWallpaper = new St.Bin({ style_class: 'current-wallpaper-bin' });
        box.add(this._currentWallpaper);

        // Build the UI.
        this.buildUI();
        this.refreshInfo();
    },

	refreshInfo: function() {
        //TODO
    },

    buildUI: function() {
        // This will hold the thumbnail for the current wallpaper.
        this._currentWallpaperThumbnail = new St.Icon({
            icon_size: 128,
            icon_name: 'nextwall',
            style_class: 'current-wallpaper-thumbnail'
        });

        // Add the current wallpaper thumb to a box.
        let box = new St.BoxLayout({
            style_class: 'current-wallpaper-box'
        });
        box.add_actor(this._currentWallpaperThumbnail);
        this._currentWallpaper.set_child(box);

        // Wallpaper labels
        this._currentWallpaperPath = new St.Label({ text: '...' });
        this._currentWallpaperBrightness = new St.Label({ text: '...' });

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

        databox_captions.add_actor(new St.Label({text: _('Path:')}));
        databox_values.add_actor(this._currentWallpaperPath);
        databox_captions.add_actor(new St.Label({text: _('Brightness:')}));
        databox_values.add_actor(this._currentWallpaperBrightness);

        let xb = new St.BoxLayout();
        xb.add_actor(databox);

        box.add_actor(xb);
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

        // Create a panel container.
        this.panelContainer = new St.BoxLayout();
        this.actor.add_actor(this.panelContainer);

        // Add an icon to the panel container.
        this.icon = new St.Icon({
            icon_name: 'nextwall',
            icon_size: 16,
            style_class: 'panel-icon'
        });
        this.panelContainer.add(this.icon);

        // Set the menu itens:

        // Create the wallpaper info box.
        let info = new WallpaperInfoBox();
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

        // Delete Wallpaper
        item = new PopupMenu.PopupMenuItem("Delete Wallpaper");
        item.connect('activate', Lang.bind(this, this._onDeleteWallpaper));
        this.menu.addMenuItem(item);

        // -----
        let item = new PopupMenu.PopupSeparatorMenuItem();
        this.menu.addMenuItem(item);

        // Fit Time of Day
        this.todSwitch = new PopupMenu.PopupSwitchMenuItem("Fit Time of Day", false);
        this.menu.addMenuItem(this.todSwitch);

        // Settings
        item = new PopupMenu.PopupMenuItem("Settings");
        item.connect('activate', Lang.bind(this, this._onSettings));
        this.menu.addMenuItem(item);
	},

    _onNextWallpaper: function() {
        //TODO
    },

    _onOpenWallpaper: function() {
        //TODO
    },

    _onDeleteWallpaper: function() {
        //TODO
    },

    _onScanWallpapers: function() {
        //TODO
    },

    _onSettings: function() {
        //TODO
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
