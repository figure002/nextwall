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
import datetime

import xdg.BaseDirectory as basedir
from gi.repository import GObject, GConf

import std

GObject.threads_init()

__author__ = "Serrano Pereira"
__copyright__ = ("Copyright 2004, Davyd Madeley\n"
    "Copyright 2010, 2011, Serrano Pereira")
__credits__ = ["Serrano Pereira <serrano.pereira@gmail.com>"]
__license__ = "GPL3"
__version__ = "0.3"
__maintainer__ = "Serrano Pereira"
__email__ = "serrano.pereira@gmail.com"
__status__ = "Production"
__date__ = "2011/08/27"


# Set the user's data folder.
DATA_HOME = os.path.join(basedir.xdg_data_home, 'nextwall')
# Set the path to database file.
DBFILE = os.path.join(DATA_HOME, 'nextwall.db')
# <=1st item = Day; >=2nd item = Night; In between = Twilight
KURTOSIS_THRESHOLD = (0.0, 2.0)


def main():
    # Make sure the 'identify' command is available.
    if not std.is_command('identify'):
        logging.error("No command 'identify' found. Please install package 'imagemagick'.")
        sys.exit(1)

    # Registers adapt_str to convert the custom Python type into one of
    # SQLite's supported types. This adds support for Unicode filenames.
    sqlite.register_adapter(str, adapt_str)

    # Check if the data folder exists. If not, create it.
    if not os.path.exists(DATA_HOME):
        logging.info("Creating data folder %s" % (DATA_HOME))
        os.mkdir(DATA_HOME)

    # Check if the database file exists. If not, create it.
    if not os.path.isfile(DBFILE):
        logging.info("Creating database file %s" % (DBFILE))
        connection = sqlite.connect(DBFILE)
        make_db(connection)
        connection.close()

    # Create a database connection.
    connection = sqlite.connect(DBFILE)

    # Run the main class.
    NextWall(connection)

    # Close the database connection.
    connection.close()

def adapt_str(string):
    """Convert the custom Python type into one of SQLite's supported types."""
    return string.decode("utf-8")

def make_db(connection):
    """Create an empty database with the necessary tables."""
    db_version = "0.2"
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


class NextWall(object):
    """The main class."""

    def __init__(self, connection):
        self.connection = connection # Database connection
        self.recursive = False # Enable recursion
        self.applet = False # Display Application Indicator
        self.fit_time = False # Fit time of day
        self.path = "/usr/share/backgrounds/" # Default backgrounds folder
        self.gconf_client = GConf.Client.get_default() # GConf client
        self.longitude = 0.0
        self.latitude = 0.0

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
        parser.add_argument('-l',
            action='store',
            type=str,
            required=False,
            help="Specify latitude and longitude of your current location.",
            metavar="LAT:LON",
            dest='lat_lon')
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
        if args.recursive: self.recursive = True
        if args.fit_time: self.fit_time = True
        if args.lat_lon:
            lat_lon = args.lat_lon.split(':')
            if len(lat_lon) == 2:
                error = self.set_lat_lon(lat_lon[0], lat_lon[1])
                if error:
                    parser.print_help()
                    sys.exit()
            else:
                parser.print_help()
                sys.exit()

        if args.verbose:
            logging.basicConfig(level=logging.INFO, format='%(levelname)s %(message)s')
        if args.scan:
            # Turn on verbose mode for scanning, so the user knows what's happening.
            logging.basicConfig(level=logging.INFO, format='%(levelname)s %(message)s')

        # Set the backgrounds folder.
        self.set_backgrounds_folder(args.path)

        # Check if we need to populate the database.
        if args.scan:
            logging.info("Scanning image files in the specified folder...")
            self.on_scan_for_images()
            sys.exit()

        # Decide what to do next.
        if args.applet:
            # Show the applet.
            import indicator
            indicator.Indicator(self)
        else:
            # Change the background.
            self.change_background()

    def set_lat_lon(self, lat, lon):
        """Set latitude and longitude."""
        try:
            self.latitude = float(lat)
            self.longitude = float(lon)

            # Set sunrise and sunset times.
            self.sunrise = std.get_sunrise(datetime.datetime.today(), self.longitude, self.latitude)
            self.sunset = std.get_sunset(datetime.datetime.today(), self.longitude, self.latitude)
            self.twilight = std.get_twilight(datetime.datetime.today(), self.longitude, self.latitude)

            return 0
        except:
            return 1

    def set_gnome_version(self):
        """Set the version of Gnome."""
        try:
            # Here subprocess.Popen() is used instead of commands.getoutput()
            # because stdout and stderr can be separated. If a GTK theme
            # results in errors, this won't affect the version detecion.
            cmd = ['gnome-session','--version']
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = p.communicate()
            # Get the version number.
            output = stdout.split()
            version = int(output[1][0])
        except:
            raise OSError("You don't appear to be running GNOME. Either GNOME 2 or 3 is required.")
        # Set the GNOME version.
        self.gnome_version = version

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

    def get_image_kurtosis(self, file):
        """Return kurtosis, a measure of the peakedness of the
        probability distribution of a real-valued random variable.

        First check if the imaga kurtosis can be found in the database.
        If it's not in the database, calculate it using ImageMagick's
        'identify' and save the value to the database.
        """
        # Try to get the image kurtosis from the database. We create a new
        # connection, because this can be called from a new thread.
        connection = sqlite.connect(DBFILE)
        cursor = connection.cursor()
        cursor.execute("SELECT kurtosis FROM wallpapers WHERE path=?", [file])
        kurtosis = cursor.fetchone()
        cursor.close()
        connection.close()

        if not kurtosis:
            # If kurtosis not found in the database, calculate it.

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

            # Convert to float.
            try:
                kurtosis = float(kurtosis)
            except:
                return None

            # Save the calculated kurtosis to the database.
            self.save_to_db(file, kurtosis)
        else:
            kurtosis = kurtosis[0]

        return float(kurtosis)

    def save_to_db(self, file, kurtosis):
        """Save the kurtosis value for `file` to the database."""
        if not isinstance(kurtosis, float): return
        # Here we create a new database connection, as this may be called from
        # a separate thread.
        connection = sqlite.connect(DBFILE)
        cursor = connection.cursor()
        cursor.execute("INSERT INTO wallpapers VALUES (null, ?, ?, null, null);", [file, kurtosis])
        connection.commit()
        cursor.close()
        connection.close()

    def set_fit_time(self, boolean):
        """Setter for fit time of day feature."""
        self.fit_time = boolean

    def set_defined_brightness(self, file, brightness):
        if brightness not in (0,1,2):
            raise ValueError("The brightness value must be one of 0, 1, or 2.")
        cursor = self.connection.cursor()
        cursor.execute("UPDATE wallpapers SET defined_brightness=? WHERE path=?;", [brightness, file])
        self.connection.commit()
        cursor.close()

    def get_defined_brightness(self, file):
        cursor = self.connection.cursor()
        cursor.execute("SELECT defined_brightness FROM wallpapers WHERE path=?;", [file])
        brightness = cursor.fetchone()
        cursor.close()
        if not brightness:
            return None
        return brightness[0]

    def on_scan_for_images(self):
        """Calculate the image kurtosis for each image in the selected
        folder and save it to the database.
        """
        images = self.get_image_files()
        if len(images) == 0:
            logging.info("No images found.")
            return

        # Create a progress bar.
        pbar = std.ProgressBar(0, len(images), 30)

        # Get the image kurtosis of each file and save it to the database.
        for i, image in enumerate(images, start=1):
            pbar.updateAmount(i)
            sys.stdout.write("\r%s" % str(pbar))
            sys.stdout.flush()
            self.get_image_kurtosis(image)

        print
        logging.info("Database successfully populated.")

    def get_image_brightness(self, file, get=False):
        """Return the image brightness value for `file`.

        First checks if the user defined a brightness for this image. If not,
        it used the kurtosis value to define the brightness value.

        Dark: return 0
        Medium: return 1
        Bright: return 2
        """
        kurtosis = self.get_image_kurtosis(file)
        if kurtosis == None:
            return None
        brightness = self.get_defined_brightness(file)

        if brightness == None:
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
        current_brightness = self.get_local_brightness()

        # Compare image value with target value.
        if brightness == current_brightness:
            return True
        else:
            return False

    def get_local_brightness(self):
        """Return the brightness value for the current time of day.

        Twilight is the time between dawn and sunrise or between sunset and
        dusk."""
        time = std.localtime()
        dawn = self.twilight[0]
        dusk = self.twilight[1]

        # Create info message.
        logging.info("Dawn: %s; Sunrise: %s; Sunset: %s; Dusk: %s" % (dawn, self.sunrise, self.sunset, dusk))

        # Decide based on current time which brightness value (0,1,2)
        # is expected.
        if self.sunrise < time < self.sunset:
            return 2 # Day
        elif time > dusk or time < dawn:
            return 0 # Night
        else:
            return 1 # Twilight

    def get_image_files(self):
        """Return a list of image files from the specified folder."""
        if self.recursive:
            # Get the files from the backgrounds folder recursively.
            dir_items = std.get_files_recursively(self.path)
        else:
            # Get the files from the backgrounds folder non-recursively.
            dir_items = std.get_files(self.path)

        # Check if the background items are actually images. Approved
        # files are put in 'images'.
        images = []
        for item in dir_items:
            mimetype = mimetypes.guess_type(item)[0]
            if mimetype and mimetype.split('/')[0] == "image":
                images.append(item)

        return images

    def get_random_wallpaper(self):
        cursor = self.connection.cursor()

        # Prevent that /path/a matches /path/abc by appending the path
        # with a trailing slash.
        if not self.path.endswith('/'): self.path += '/'

        if not self.fit_time:
            cursor.execute("SELECT id FROM wallpapers WHERE path LIKE \"%s%%\"" %
                self.path)

            matching_ids = cursor.fetchall()
            if matching_ids:
                matching_ids = list(matching_ids)
            else:
                # Return None if no wallpapers were found.
                return None
        else:
            brightness = self.get_local_brightness()

            # First get the wallpapers for which the user hasn't set a custom
            # brightness value.

            # Day
            if brightness == 2:
                cursor.execute("SELECT id \
                    FROM wallpapers \
                    WHERE path LIKE \"%s%%\" \
                    AND kurtosis < ? \
                    AND defined_brightness IS NULL;" % self.path,
                    [KURTOSIS_THRESHOLD[0]]
                    )
            # Night
            elif brightness == 0:
                cursor.execute("SELECT id \
                    FROM wallpapers \
                    WHERE path LIKE \"%s%%\" \
                    AND kurtosis > ? \
                    AND defined_brightness IS NULL;" % self.path,
                    [KURTOSIS_THRESHOLD[1]]
                    )
            # Twilight
            elif brightness == 1:
                cursor.execute("SELECT id \
                    FROM wallpapers \
                    WHERE path LIKE \"%s%%\" \
                    AND kurtosis > ? \
                    AND kurtosis < ? \
                    AND defined_brightness IS NULL;" % self.path,
                    [KURTOSIS_THRESHOLD[0], KURTOSIS_THRESHOLD[1]]
                    )

            matching_ids = cursor.fetchall()
            if matching_ids:
                matching_ids = list(matching_ids)
            else:
                matching_ids = []
            #print matching_ids

            # Then get the wallpapers with a custom brightness value and where
            # the custom value matched the current time of day.

            cursor.execute("SELECT id \
                FROM wallpapers \
                WHERE path LIKE \"%s%%\" \
                AND defined_brightness = ?;" % self.path,
                [brightness]
                )

            # Finally, merge the two lists together.

            matching_ids_extended = cursor.fetchall()
            #print matching_ids_extended
            if matching_ids_extended:
                matching_ids.extend(matching_ids_extended)

            # Return None if no wallpapers were found.
            if len(matching_ids) == 0:
                return None

        # Get a list of integers instead of a list of tuples.
        matching_ids = [x[0] for x in matching_ids]
        #print matching_ids

        # Get a new random wallpaper from the database.
        random_id = random.choice(matching_ids)
        cursor.execute("SELECT path FROM wallpapers WHERE id = ?;", [random_id])
        path = cursor.fetchone()[0]

        while not os.path.isfile(path):
            # Remove this wallpaper from the database and the list.
            logging.info("The file %s does not exist. Removing from the database." % path)
            cursor.execute("DELETE FROM wallpapers WHERE id = ?;", [random_id])
            self.connection.commit()
            matching_ids.remove(random_id)

            # Get a new random wallpaper from the selection.
            if len(matching_ids) == 0:
                path = None
                break
            random_id = random.choice(matching_ids)
            cursor.execute("SELECT path FROM wallpapers WHERE id = ?;", [random_id])
            path = cursor.fetchone()[0]

        cursor.close()

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
        while path.decode('utf-8') == current_bg:
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
