const Gdk = imports.gi.Gdk;
const Gio = imports.gi.Gio;
const Gtk = imports.gi.Gtk;
const GObject = imports.gi.GObject;
const Lang = imports.lang;

const ExtensionUtils = imports.misc.extensionUtils;
const Me = ExtensionUtils.getCurrentExtension();
const Convenience = Me.imports.convenience;

const NextwallSettingsWidget = new GObject.Class({
    Name: 'Nextwall.Prefs.NextwallSettingsWidget',
    GTypeName: 'NextwallSettingsWidget',
    Extends: Gtk.VBox,

    _init : function(params) {
        this.parent(params);
        this.margin = 10;

        this._settings = Convenience.getSettings();

        let vbox, label;

        // Wallpaper path
        label = new Gtk.Label();
        label.set_markup("<b>Wallpapers Folder</b>")
        label.set_alignment(0, 0.5)
        this.add(label);

        vbox = new Gtk.VBox({margin: 10});
        this.add(vbox);

        let file_chooser = new Gtk.FileChooserButton({title: 'Select Backgrounds Folder'});
        file_chooser.set_action(Gtk.FileChooserAction.SELECT_FOLDER);
        file_chooser.set_current_folder(this._settings.get_string("wallpaper-path"));
        vbox.add(file_chooser)
        file_chooser.connect('file-set', Lang.bind(this, this._onWallpaperPathChange));

        // Fit time of day
        /*
        label = new Gtk.Label();
        label.set_markup("<b>Fit Time of Day</b>")
        label.set_alignment(0, 0.5)
        this.add(label);

        vbox = new Gtk.VBox({margin: 10});
        this.add(vbox);
        let sw = new Gtk.Switch();
        sw.visible = 1;
        sw.can_focus = 0;
        sw.active = this._settings.get_boolean("fit-time");;
        vbox.add(sw)
        sw.connect('notify::active', Lang.bind(this, this._onFitTimeChange));
        sw.set_active(this._settings.get_boolean("fit-time"));
        */

        // Location
        label = new Gtk.Label();
        label.set_markup("<b>Your Current Location</b>")
        label.set_alignment(0, 0.5)
        this.add(label);

        label = new Gtk.Label();
        label.set_markup("Enter location in the format 'latitude:longitude'. This is required for the Fit Time of Day feature.")
        label.set_alignment(0, 0)
        this.add(label);

        vbox = new Gtk.VBox({margin: 10});
        this.add(vbox);
        let entry = new Gtk.Entry({margin_bottom: 10,
                                   margin_top: 5,
                                   text: this._settings.get_string("location")})
        vbox.add(entry)
        entry.connect('changed', Lang.bind(this, this._onLocationChange));
    },

    _onWallpaperPathChange: function(widget, data) {
        this._settings.set_string("wallpaper-path", widget.get_current_folder());
    },

    _onFitTimeChange: function(widget, data) {
        this._settings.set_boolean("fit-time", widget.get_active());
    },

    _onLocationChange: function(widget, data) {
        this._settings.set_string("location", widget.get_text());
    },
});

function init() {

}

function buildPrefsWidget() {
    let widget = new NextwallSettingsWidget();
    widget.show_all();

    return widget;
}
