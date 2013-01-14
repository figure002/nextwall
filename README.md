# NextWall

This is the README file for NextWall (nextwall) - A wallpaper changer for the
GNOME desktop.

*NextWall* is a small script that changes the background of the GNOME desktop
to a random image. It is based on the [change-background.py](http://oracle.bridgewayconsulting.com.au/~danni/misc/change-background-py.html)
script by Davyd Madeley. NextWall has the following features:

* Can operate as a command-line tool. Run <code>nextwall --help</code> for
  usage information.
* Can place itself in the GNOME-panel with support for Ubuntu's Application
  Indicator.
* The fit time of day feature automatically sets backgrounds that fit the time
  of day (dark backgrounds at night, bright backgrounds at day, intermediate at
  twilight).

## Setup

First make sure you have all dependencies installed:

* gnome
* python >= 2.6 && python < 3
* libappindicator-dev
* python-appindicator
* imagemagick

On Debian (based) systems, run this command to install all dependencies:

    sudo apt-get install python libappindicator-dev python-appindicator imagemagick

Then you can configure the nextwall build. You'll need CMake for that:

    sudo apt-get install cmake cmake-curses-gui

In the top level directory of the nextwall package, run the following commands:

    mkdir build
    cd build/
    ccmake ..

This will present you with a console based GUI with options for nextwall.
First press 'c' to run the configure script. Review the options and change what
you think is necessary. Pay special attention to the option `CMAKE_INSTALL_PREFIX`.
This determines where on the system nextwall will be installed.
`CMAKE_INSTALL_PREFIX = /usr` means that nextwall will be installed in
`/usr/share/nextwall/`. When done setting the options, press 'c' to run the
configure script again. Then press 'g' to generate the make files and exit.

Once the make files have been generated, run the following command to install
nextwall (you may need to run this as root):

    make install

This will also install icons for nextwall, so you need to update the system's
icon cache by running the post-installation script:

    cmake/debian/postinst


## Uninstall

To removing all installed nextwall files, change to the build folder which you
created in the top level directory:

    cd build/

And run the uninstall command (you may need to run this as root):

    make uninstall

Alternatively, you can remove the files listed in the install_manifest.txt file
which has the same result as the `make uninstall` command.

    xargs -d "\n" rm < install_manifest.txt


## Usage Tips
* You can use `watch` to temporarily change the background at a set interval.
  For 60 seconds, run `watch -n 60 nextwall [options] [path]`
* You can also use the cron deamon to schedule execution of NextWall. For once
  an hour, run `sudo crontab -u $USER -e` and add this line:

  ``0 * * * * DISPLAY=:0.0 nextwall [options] [path]``


## License

The source code for nextwall is licensed under the GNU General Public License
Version 3, which you can find in the COPYING file.

All graphical assets are licensed under the
[Creative Commons Attribution 3.0 Unported License](http://creativecommons.org/licenses/by/3.0/).
