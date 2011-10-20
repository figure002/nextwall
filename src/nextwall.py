#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# This file is part of NextWall - A wallpaper changer with some sense of time.
#
#  Copyright 2004, Davyd Madeley <davyd@madeley.id.au>
#  Copyright 2010, 2011, Serrano Pereira <serrano.pereira@gmail.com>
#
#  NextWall is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  NextWall is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys
import os
import random
import logging
import mimetypes
import argparse
import commands
import threading
import time
from sqlite3 import dbapi2 as sqlite
import subprocess

import xdg.BaseDirectory as basedir
import gconf
import gobject

import indicator

gobject.threads_init()

__author__ = "Serrano Pereira"
__copyright__ = ("Copyright 2004, Davyd Madeley\n"
    "Copyright 2010, 2011, Serrano Pereira")
__credits__ = ["Serrano Pereira <serrano.pereira@gmail.com>"]
__license__ = "GPL3"
__version__ = "0.2.2"
__maintainer__ = "Serrano Pereira"
__email__ = "serrano.pereira@gmail.com"
__status__ = "Production"
__date__ = "2011/08/27"


KURTOSIS_THRESHOLD = (0.0, 2.0) # <=1st item = Day; >=2nd item = Night; In between = Dusk/Dawn
DAWN_START = 6 # Hour at which dawn starts
DAY_START = 8 # Hour at which day starts
DUSK_START = 20 # Hour at which dusk starts
NIGHT_START = 22 # Hour at which night starts

def main():
    # Make sure the 'identify' command is available.
    if not is_command('identify'):
        logging.error("No command 'identify' found. Please install package 'imagemagick'.")
        sys.exit(1)

    # Registers adapt_str to convert the custom Python type into one of
    # SQLite's supported types. This adds support for Unicode filenames.
    sqlite.register_adapter(str, adapt_str)

    NextWall()

def adapt_str(string):
    """Convert the custom Python type into one of SQLite's supported types."""
    return string.decode("utf-8")

def is_command(command):
    """Check if a specific command is available."""
    cmd = 'which %s' % (command)
    output = commands.getoutput(cmd)
    if len(output.splitlines()) == 0:
        return False
    else:
        return True

def get_files_recursively(rootdir):
    """Recursively get a list of files from a folder."""
    file_list = []

    for root, sub_folders, files in os.walk(rootdir):
        for file in files:
            file_list.append(os.path.join(root,file))

        # Don't visit thumbnail directories.
        if '.thumbs' in sub_folders:
            sub_folders.remove('.thumbs')

    return file_list

def get_files(rootdir):
    """Non-recursively get a list of files from a folder."""
    file_list = []

    files = os.listdir(rootdir)
    for file in files:
        file_list.append(os.path.join(rootdir,file))

    return file_list

class NextWall(object):
    """The main class."""

    def __init__(self):
        self.recursive = False # Enable recursion
        self.applet = False # Display Application Indicator
        self.fit_time = False # Fit time of day
        self.scan_for_images = False # Analyze all image files
        self.path = "/usr/share/backgrounds/" # Default backgrounds folder
        self.gconf_client = gconf.client_get_default() # GConf client
        self.data_home = os.path.join(basedir.xdg_data_home, 'nextwall') # User's data folder
        self.dbfile = os.path.join(self.data_home, 'nextwall.db') # Path to database file

        # Check which version of Gnome we are running.
        self.set_gnome_version()

        # Create main argument parser.
        parser = argparse.ArgumentParser(prog='nextwall', description='A wallpaper changer with some sense of time.')
        parser.add_argument('--version',
            action='version',
            help="Print version information.",
            version="nextwall "+__version__)
        parser.add_argument('-r, --recursive',
            action='store_true',
            help="Look in subfolders",
            dest='recursive')
        parser.add_argument('-a, --applet',
            action='store_true',
            help="Run as applet in the GNOME panel.",
            dest='applet')
        parser.add_argument('-t, --fit-time',
            action='store_true',
            help="Select backgrounds that fit the time of day. It is required to scan (--scan) for wallpapers first.",
            dest='fit_time')
        parser.add_argument('-s, --scan',
            action='store_true',
            help="Scan for images files in PATH.",
            dest='scan')
        parser.add_argument('-v, --verbose',
            action='store_true',
            help="Turn on verbose output.",
            dest='verbose')
        parser.add_argument('path',
            action='store',
            default='/usr/share/backgrounds/',
            nargs='?',
            type=str,
            help="Path to folder containing background images. If no path is specified, the default path /usr/share/backgrounds/ will be used.",
            metavar="PATH")

        # Parse the arguments.
        args = parser.parse_args()

        # Handle the arguments.
        if args.recursive:
            self.recursive = True
        if args.applet:
            self.applet = True
        if args.fit_time:
            self.set_fit_time(True)
        if args.verbose:
            logging.basicConfig(level=logging.INFO, format='%(levelname)s %(message)s')
        if args.scan:
            self.scan_for_images = True
            # Turn on verbose mode, so the user knows what's happening.
            logging.basicConfig(level=logging.INFO, format='%(levelname)s %(message)s')

        # Check if the data folder exists. If not, create it.
        if not os.path.exists(self.data_home):
            logging.info("Creating data folder %s" % (self.data_home))
            os.mkdir(self.data_home)

        # Check if the database file exists. If not, create it.
        if not os.path.isfile(self.dbfile):
            logging.info("Creating database file %s" % (self.dbfile))
            self.make_db()

        # Set the backgrounds folder.
        self.set_backgrounds_folder(args.path)

        # Check if we need to populate the database.
        if self.scan_for_images:
            logging.info("Will start scanning for images. This could take a while depending on the \n"
                "number of images in the specified folder. Starting in 5 seconds...")
            time.sleep(5)
            self.on_scan_for_images()
            sys.exit()

        # Decide what to do next.
        if self.applet:
            # Show the applet.
            indicator.Indicator(self)
        else:
            # Change the background.
            self.change_background()

    def set_backgrounds_folder(self, path):
        """Set the backgrounds folder."""
        # If no argument was set, use the default path.
        if not path:
            return

        # Check if the backgrounds folder is valid.
        if not os.path.exists(path):
            print ("The folder %s doesn't seem to exist.\n"
                "Run '%s --help' for usage information." %
                (path, os.path.split(sys.argv[0])[1]))
            sys.exit(1)

        # Set the new folder.
        logging.info("Setting backgrounds folder to %s" % (path))
        self.path = path

    def set_fit_time(self, boolean):
        """Setter for fit time of day feature."""
        self.fit_time = boolean

    def get_image_kurtosis(self, file):
        """Return kurtosis, a measure of the peakedness of the
        probability distribution of a real-valued random variable.

        First check if the imaga kurtosis can be found in the database.
        If it's not in the database, calculate it using ImageMagick's
        'identify' and save the value to the database.
        """
        # Try to get the image kurtosis from the database.
        connection = sqlite.connect(self.dbfile)
        cursor = connection.cursor()
        cursor.execute("SELECT kurtosis FROM wallpapers WHERE path=?", [file])
        kurtosis = cursor.fetchone()
        cursor.close()
        connection.close()

        if not kurtosis:
            # If kurtosis not found in the database, calculate it.
            logging.info("\tNot in database. Calculating image kurtosis...")

            # Use 'identify' to calculate the image kurtosis.
            cmd = 'identify -verbose "%s" | grep kurtosis' % (file)
            output = commands.getoutput(cmd)
            output = output.splitlines()
            items = len(output)

            # Handle exceptions.
            if items == 0:
                return 0

            # Get the last kurtosis calculation, which is the overall.
            output = output[items-1].split()

            # The second part is the kurtosis value.
            kurtosis = output[1]

            # Save the calculated kurtosis to the database.
            self.save_to_db(file, kurtosis)

            logging.info("\tImage kurtosis: %s" % (kurtosis))
        else:
            kurtosis = kurtosis[0]

        return float(kurtosis)

    def set_gnome_version(self):
        cmd = 'gnome-session --version'
        output = commands.getoutput(cmd)
        output = output.split()
        version = output[1][0]
        try:
            version = int(version)
        except:
            raise OSError("You don't appear to be running GNOME. Either GNOME 2 or 3 is required.")
        self.gnome_version = version

    def save_to_db(self, path, kurtosis):
        connection = sqlite.connect(self.dbfile)
        cursor = connection.cursor()
        cursor.execute("INSERT INTO wallpapers VALUES (null, ?, ?, null, null)", [path, kurtosis])
        connection.commit()
        cursor.close()
        connection.close()

    def make_db(self):
        """Create an empty database with the necessary tables."""
        db_version = "0.2"
        connection = sqlite.connect(self.dbfile)
        cursor = connection.cursor()

        cursor.execute("CREATE TABLE wallpapers (\
            id INTEGER PRIMARY KEY, \
            path TEXT, \
            kurtosis FLOAT, \
            defined_brightness INTEGER, \
            rating INTEGER \
            )")

        cursor.execute("CREATE TABLE info (\
            id INTEGER PRIMARY KEY, \
            name VARCHAR, \
            value VARCHAR \
            )")

        cursor.execute("INSERT INTO info VALUES (null, 'version', ?)", [db_version])

        connection.commit()
        cursor.close()
        connection.close()

    def on_scan_for_images(self):
        """Calculate the image kurtosis for each image in the selected
        folder and save it to the database.
        """
        images = self.get_image_files()
        self.populate_total = len(images)

        # Get the image kurtosis of each file and save it to the database.
        for i, image in enumerate(images, start=1):
            self.populate_current = i
            logging.info("Found %s" % (image))
            self.get_image_kurtosis(image)

        logging.info("Database successfully populated.")

    def get_image_brightness(self, file, get=False):
        """Decide wether a kurtosis value is considered bright or dark

        Dark: return 0
        Medium: return 1
        Bright: return 2
        """
        kurtosis = self.get_image_kurtosis(file)

        if kurtosis < KURTOSIS_THRESHOLD[0]:
            # Bright
            brightness = 2
        elif kurtosis > KURTOSIS_THRESHOLD[1]:
            # Dark
            brightness = 0
        else:
            # Medium
            brightness = 1

        if get:
            return (kurtosis, brightness)

        return brightness

    def match_image_time(self, file):
        """Decide whether the image's brightness is suitable for the
        time of day.

        Suitable: return True
        Not Suitable: return False
        """
        brightness = self.get_image_brightness(file)
        current_brightness = self.get_current_brightness()

        # Compare image value with target value.
        if brightness == current_brightness:
            return True
        else:
            return False

    def get_current_brightness(self):
        """Return the brightness value for the current time of day."""
        hour = int(time.strftime("%H", time.localtime()))

        # Decide based on current time which brightness value (0,1,2)
        # is expected.
        if hour >= DAY_START and hour < DUSK_START:
            # Day
            return 2
        elif hour >= NIGHT_START or hour < DAWN_START:
            # Night
            return 0
        else:
            # Dawn/Dusk
            return 1

    def get_image_files(self):
        """Return a list of image files from the specified folder."""
        if self.recursive:
            # Get the files from the backgrounds folder recursively.
            dir_items = get_files_recursively(self.path)
        else:
            # Get the files from the backgrounds folder non-recursively.
            dir_items = get_files(self.path)

        # Check if the background items are actually images. Approved
        # files are put in 'images'.
        images = []
        for item in dir_items:
            mimetype = mimetypes.guess_type(item)[0]
            if mimetype and mimetype.split('/')[0] == "image":
                images.append(item)

        return images

    def get_random_wallpaper(self):
        connection = sqlite.connect(self.dbfile)
        cursor = connection.cursor()

        if not self.fit_time:
            cursor.execute("SELECT id FROM wallpapers WHERE path LIKE \"%s%%\"" %
                self.path)
        else:
            brightness = self.get_current_brightness()

            # Day
            if brightness == 2:
                cursor.execute("SELECT id \
                    FROM wallpapers \
                    WHERE path LIKE \"%s%%\" \
                    AND kurtosis < ?" % self.path,
                    [KURTOSIS_THRESHOLD[0]]
                    )
            # Night
            elif brightness == 0:
                cursor.execute("SELECT id \
                    FROM wallpapers \
                    WHERE path LIKE \"%s%%\" \
                    AND kurtosis > ?" % self.path,
                    [KURTOSIS_THRESHOLD[1]]
                    )
            # Dusk/Dawn
            elif brightness == 1:
                cursor.execute("SELECT id \
                    FROM wallpapers \
                    WHERE path LIKE \"%s%%\" \
                    AND kurtosis > ? \
                    AND kurtosis < ?" % self.path,
                    [KURTOSIS_THRESHOLD[0], KURTOSIS_THRESHOLD[1]]
                    )

        matching_ids = cursor.fetchall()
        if not matching_ids:
            return None

        # Get a new random wallpaper from the database.
        random_id = random.choice(matching_ids)[0]
        cursor.execute("SELECT path \
            FROM wallpapers \
            WHERE id = ?", [random_id])
        path = cursor.fetchone()[0]

        while not os.path.isfile(path):
            # Remove this path from the database.
            logging.info("The file %s does not exist. Removing from the database." % path)
            cursor.execute("DELETE FROM wallpapers \
                WHERE id = ?", [random_id])
            connection.commit()

            # Get a new random wallpaper from the database.
            random_id = random.choice(matching_ids)[0]
            cursor.execute("SELECT path \
                FROM wallpapers \
                WHERE id = ?", [random_id])
            path = cursor.fetchone()[0]

        cursor.close()
        connection.close()

        return path

    def change_background(self, widget=None, data=None):
        """Change the background."""
        if self.fit_time:
            logging.info("Looking for backgrounds that fit the time of day...")

        path = self.get_random_wallpaper()

        # Check if a path was returned.
        if not path:
            logging.error("No wallpaper was found in the database. You might need to scan for images first.")
            return 1

        # Get the current background used by GNOME.
        current_bg = self.get_background_uri()

        # Make sure the random background item isn't the same as the
        # current background.
        sames = 0
        while path == current_bg:
            # Stop the loop if we find the same image 3 times.
            if sames == 3:
                logging.info("Not enough image files. Select a different folder or scan for image files.")
                return 2
            path = self.get_random_wallpaper()
            sames += 1

        # Finally, set the new background.
        logging.info("Setting background to %s" % path)
        self.set_background_uri(path)

        return 0

    def set_background_uri(self, filename):
        """Set the background URI.

        If we are running GNOME 2, then the gconf client is used to set the
        new background. If running GNOME 3, then `gsettings' is used.
        """
        if self.gnome_version == 3:
            if not filename.startswith("file://"):
                filename = "file://"+filename
            cmd = ['gsettings','set','org.gnome.desktop.background','picture-uri', filename]
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = p.communicate()
        else:
            self.gconf_client.set_string("/desktop/gnome/background/picture_filename", filename)

    def get_background_uri(self):
        """Return the background URI.

        If we are running GNOME 2, then the gconf client is used to get the
        URI. If running GNOME 3, then `gsettings' is used.
        """
        if self.gnome_version == 3:
            cmd = ['gsettings','get','org.gnome.desktop.background','picture-uri']
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = p.communicate()
            uri = stdout
            uri = stdout.strip("'")
            uri = uri[7:]
            uri = uri[:-2] # This is a workaround.
        else:
            uri = self.gconf_client.get_string("/desktop/gnome/background/picture_filename")
        return uri

if __name__ == "__main__":
    main()

sys.exit()
