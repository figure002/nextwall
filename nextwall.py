#!/usr/bin/env python
# -*- coding: latin-1 -*-
#
# nextwall.py
#
# A script to change to a random background image.
#
#  Copyright 2004, Davyd Madeley <davyd@madeley.id.au>
#  Copyright 2010, Serrano Pereira <serrano.pereira@gmail.com>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys
import gconf
import os
import random
import mimetypes
import getopt
import appindicator
import gtk

__version__ = "0.7"

class NextWall(object):
    def __init__(self):
        self.argv = sys.argv[1:]
        self.recursive = False
        self.applet = False
        self.path = "/usr/share/backgrounds/"
        self.client = gconf.client_get_default()

        try:
            opts, args = getopt.getopt(self.argv, "hra",
                ["help", "recursive", "applet"])
        except getopt.GetoptError:
            self.usage()

        # Check if the user wants help.
        for opt, arg in opts:
            if opt in ("-h", "--help"):
                self.usage()
            if opt in ("-r", "--recursive"):
                self.recursive = True
            if opt in ("-a", "--applet"):
                self.applet = True

        # Set backgrounds folder.
        if len(args) == 0:
            pass
        elif len(args) == 1:
            self.path = os.path.abspath(args[0])
        else:
            self.usage()

        # Check if the backgrounds folder is valid.
        if not os.path.exists(self.path):
            print ("The folder %s doesn't seem to exist.\n"
                "Run '%s --help' for usage information." %
                (self.path, os.path.split(sys.argv[0])[1]))
            sys.exit(1)

        # Decide what to do next.
        if self.applet:
            # Show the applet.
            self.show_applet()
        else:
            # Change the background.
            self.on_change_background()

    def get_files_recursively(self, rootdir):
        """Recursively get a list of files from a folder."""
        file_list = []

        for root, sub_folders, files in os.walk(rootdir):
            for file in files:
                file_list.append(os.path.join(root,file))

            # Don't visit thumbnail directories.
            if '.thumbs' in sub_folders:
                sub_folders.remove('.thumbs')

        return file_list

    def get_files(self, rootdir):
        """Non-recursively get a list of files from a folder."""
        file_list = []

        files = os.listdir(rootdir)
        for file in files:
            file_list.append(os.path.join(rootdir,file))

        return file_list

    def usage(self):
        """Print usage information."""
        print "nextwall %s" % (__version__)
        print "\nUsage: %s [options] [path]" % ( os.path.split(sys.argv[0])[1] )
        print "  [path]\tPath to folder containing background images."
        print "\t\tIf no path is specified, the default path"
        print "\t\t/usr/share/backgrounds/ will be used."
        print "\nOptions:"
        print "  -h, --help\t\tShow usage information."
        print "  -r, --recursive\tLook in subfolders."
        print "  -a, --applet\t\tRun as applet in the GNOME panel."
        sys.exit()

    def on_quit(self, widget, data=None):
        """Exit the application."""
        gtk.main_quit()

    def on_preferences(self, widget, data=None):
        """Display preferences window."""
        Preferences(self)

    def on_delete_current(self, widget, data=None):
        """Delete the current background from the harddisk."""

        current_bg = self.client.get_string("/desktop/gnome/background/picture_filename")
        message = ("This will <b>permanently</b> remove the current background "
            "image (%s) from your harddisk.") % (current_bg)

        dialog = gtk.MessageDialog(parent=None, flags=0,
            type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_YES_NO,
            message_format=None)
        dialog.set_markup(message)
        dialog.format_secondary_text("Continue?")
        response = dialog.run()

        if response == gtk.RESPONSE_YES:
            os.remove(current_bg)
            self.on_change_background()

        dialog.destroy()

    def on_change_background(self, widget=None, data=None):
        """Change background according to given arguments."""

        if self.recursive:
            # Get the files from the backgrounds folder recursively.
            dir_items = self.get_files_recursively(self.path)
        else:
            # Get the files from the backgrounds folder non-recursively.
            dir_items = self.get_files(self.path)

        # Check if the background items are actually images. Approved
        # files are put in 'items'.
        items = []
        for item in dir_items:
            mimetype = mimetypes.guess_type(item)[0]
            if mimetype and mimetype.split('/')[0] == "image":
                items.append(item)

        # Check if any image files were found.
        if len(items) == 0:
            message = ("No image files were found in '%s'\nSet a "
                "different backgrounds folder or enable recursion to "
                "look in subfolders.") % (self.path)
            if self.applet:
                dialog = gtk.MessageDialog(parent=None, flags=0,
                    type=gtk.MESSAGE_INFO, buttons=gtk.BUTTONS_OK,
                    message_format=message)
                response = dialog.run()

                if response == gtk.RESPONSE_OK:
                    dialog.destroy()
                    return
            else:
                print message
                sys.exit(1)

        # Get a random background item from the file list.
        item = random.randint(0, len(items) - 1)

        # Create a gconf object.
        #client = gconf.client_get_default()

        # Get the current background used by GNOME.
        current_bg = self.client.get_string("/desktop/gnome/background/picture_filename")

        # Make sure the random background item isn't the same as the
        # background currently being used.
        while(items[item] == current_bg):
            item = random.randint(0, len(items) - 1)

        # Finally, set the new background.
        self.client.set_string("/desktop/gnome/background/picture_filename",
            items[item])

    def show_applet(self):
        """Display an Application Indicator in the GNOME panel."""

        # Create an Application Indicator icon
        ind = appindicator.Indicator ("change-background-menu",
            "gsd-xrandr",
            appindicator.CATEGORY_APPLICATION_STATUS)
        ind.set_status (appindicator.STATUS_ACTIVE)

        # Create GTK menu
        menu = gtk.Menu()

        # Create the menu items
        change_item = gtk.MenuItem("Next Wallpaper")
        delete_item = gtk.MenuItem("Delete Current")
        pref_item = gtk.MenuItem("Preferences")
        quit_item = gtk.MenuItem("Quit")
        separator = gtk.SeparatorMenuItem()

        # Add them to the menu
        menu.append(change_item)
        menu.append(delete_item)
        menu.append(separator)
        menu.append(pref_item)
        menu.append(quit_item)

        # Attach the callback functions to the activate signal
        change_item.connect("activate", self.on_change_background)
        delete_item.connect("activate", self.on_delete_current)
        pref_item.connect("activate", self.on_preferences)
        quit_item.connect("activate", self.on_quit)

        # Show menu items
        change_item.show()
        delete_item.show()
        pref_item.show()
        separator.show()
        quit_item.show()

        # Add the menu to the Application Indicator
        ind.set_menu(menu)

        # Run the main loop
        gtk.main()

class Preferences(gtk.Window):
    def __init__(self, wallobj):
        self.wallobj = wallobj
        super(Preferences, self).__init__()

        self.set_title("Preferences")
        self.set_size_request(400, -1)
        self.set_border_width(10)
        self.set_position(gtk.WIN_POS_CENTER)

        # Create table container
        table = gtk.Table(rows=3, columns=2, homogeneous=False)
        table.set_col_spacings(10)
        table.set_row_spacings(10)

        # Create a label
        title = gtk.Label("Select backgrounds folder:")
        table.attach(child=title, left_attach=0, right_attach=1,
            top_attach=0, bottom_attach=1, xoptions=gtk.FILL,
            yoptions=gtk.SHRINK, xpadding=0, ypadding=0)

        # Create a fole chooser button
        self.folderselect = gtk.FileChooserButton('Select a folder')
        self.folderselect.set_action(gtk.FILE_CHOOSER_ACTION_SELECT_FOLDER)
        self.folderselect.set_current_folder(self.wallobj.path)
        table.attach(child=self.folderselect, left_attach=1, right_attach=2,
            top_attach=0, bottom_attach=1, xoptions=gtk.FILL|gtk.EXPAND,
            yoptions=gtk.SHRINK, xpadding=0, ypadding=0)

        # Create a checkbutton for recusrion
        self.check_recursion = gtk.CheckButton("Enable recursion (look in subfolders)")
        if self.wallobj.recursive:
            self.check_recursion.set_active(True)
        #check_recursion.unset_flags(gtk.CAN_FOCUS)
        table.attach(child=self.check_recursion, left_attach=0, right_attach=2,
            top_attach=1, bottom_attach=2, xoptions=gtk.FILL|gtk.EXPAND,
            yoptions=gtk.SHRINK, xpadding=0, ypadding=0)

        # About button
        button_about = gtk.Button("About")
        button_about.set_size_request(70, -1)
        button_about.connect("clicked", self.on_about)
        about_align = gtk.Alignment(xalign=0, yalign=0, xscale=0, yscale=0)
        about_align.add(button_about)
        table.attach(child=about_align, left_attach=0, right_attach=1,
            top_attach=2, bottom_attach=3, xoptions=gtk.FILL,
            yoptions=gtk.SHRINK, xpadding=0, ypadding=0)

        # OK button
        button_ok = gtk.Button("OK")
        button_ok.set_size_request(70, -1)
        button_ok.connect("clicked", self.on_ok)

        # Cancel button
        button_cancel = gtk.Button("Cancel")
        button_cancel.set_size_request(70, -1)
        button_cancel.connect("clicked", self.on_cancel)

        # Put the buttons in a box
        button_box = gtk.HBox(homogeneous=True, spacing=5)
        button_box.add(button_cancel)
        button_box.add(button_ok)

        # Align the button box
        buttons_align = gtk.Alignment(xalign=1.0, yalign=0, xscale=0, yscale=0)
        buttons_align.add(button_box)

        # Add the aligned box to the table
        table.attach(child=buttons_align, left_attach=1, right_attach=2,
            top_attach=2, bottom_attach=3, xoptions=gtk.FILL,
            yoptions=gtk.SHRINK, xpadding=0, ypadding=0)

        # Add the table to the main window.
        self.add(table)

        # Make it visible.
        self.show_all()

    def on_ok(self, widget, data=None):
        """Save new settings and close the preferences dialog."""
        # Set the new backgrounds folder.
        self.wallobj.path = self.folderselect.get_filename()

        # Set recursion.
        if self.check_recursion.get_active():
            self.wallobj.recursive = True
        else:
            self.wallobj.recursive = False

        # Destroy the dialog.
        self.destroy()

    def on_cancel(self, widget, data=None):
        """Close the preferences dialog."""
        self.destroy()

    def on_about(self, widget, data=None):
        license = ("This program is free software: you can redistribute it and/or modify\n"
            "it under the terms of the GNU General Public License as published by\n"
            "the Free Software Foundation, either version 3 of the License, or\n"
            "(at your option) any later version.\n\n"

            "This program is distributed in the hope that it will be useful,\n"
            "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
            "GNU General Public License for more details.\n\n"

            "You should have received a copy of the GNU General Public License\n"
            "along with this program.  If not, see http://www.gnu.org/licenses/.")

        about = gtk.AboutDialog()
        about.set_program_name("NextWall")
        about.set_version(__version__)
        about.set_copyright("Copyright © Davyd Madeley, Serrano Pereira")
        about.set_comments("A script to change to a random background image.")
        about.set_license(license)
        about.run()
        about.destroy()

if __name__ == "__main__":
    wallobj = NextWall()
    sys.exit()
