# nextwall

This is the README file for nextwall - A wallpaper rotator for the GNOME
desktop.

*Nextwall* is a small script that changes the background of the GNOME desktop
to a random image. It was originally written in Python and based on the
[change-background.py](http://oracle.bridgewayconsulting.com.au/~danni/misc/change-background-py.html)
script by Davyd Madeley. At version 0.3 it was completely rewritten in C.
NextWall has the following features:

* Operates as a command-line tool. Run `nextwall --help` for
  usage information.
* The fit time of day feature automatically sets backgrounds that fit the time
  of the day (dark backgrounds at night, bright backgrounds at day,
  intermediate at twilight). It uses an Artificial Neural Network to determine
  the brightness of a wallpaper.


## Setup

To compile `nextwall` you need to have the following packages installed:

* libglib2.0-dev
* libmagickwand-dev
* libmagic-dev
* libsqlite3-dev
* libfann-dev

On Debian (based) systems, run this command to install the dependencies:

    sudo apt-get install libglib2.0-dev libmagickwand-dev libmagic-dev libsqlite3-dev libfann-dev

Then you can configure, compile, and install `nextwall`. In the top level
directory of the `nextwall` package, run the following commands:

    ./configure
    make
    make install


## Uninstall

To uninstall `nextwall`, run the uninstall command (as root):

    make uninstall


## Usage

When running `nextwall` for the first time, you need to scan your wallpaper
directory for wallpapers:

	nextwall -sr /path/to/wallpapers/

Then `nextwall` can be used as follows:

	nextwall -tl LAT:LON /path/to/wallpapers/

To change your desktop background to a wallpaper that fits the current time
of the day. Replace `LAT:LON` by the latitude:longitude of your location.


## Usage Tips

* You can use `watch` to temporarily change the background at a set interval.
  For 60 seconds, run `watch -n 60 nextwall [options] [path]`
* You can also use the cron deamon to schedule execution of `nextwall`. For
  once an hour, run `crontab -e` and add this line:
  ``0 * * * * DISPLAY=:0.0 /usr/local/bin/nextwall [options] [path]``


## ANN trainer

Nextwall comes with a trainer for the Artificial Neural Network (ANN). You can
build this trainer by running `make` in the directory `src/util/`:

    cd src/util/
	make

Then run `./train_ann --help` to see usage information for the trainer. To use
this trainer, first create a directory with image files that you want to train
the ANN on. Then you can run the trainer as follows:

    ./train_ann -p30 ~/Pictures/train/

This assumes that the directory `~/Pictures/train/` contains at least 30 image
files. Then follow the instructions on the screen to start training. Once
completed, two files will be created:

* nextwall.dat - The training data
* nextwall.net - The artificial neural network

The ANN is created from the training data. Now copy the ANN file (nextwall.net)
to `~/.local/share/nextwall/` to make `nextwall` use this ANN.

You can also create the ANN by using an existing training data file as
follows:

    ./train_ann -r ~/Pictures/train/

This will create nextwall.net from an existing nextwall.dat file.

## License

The source code for nextwall is licensed under the GNU General Public License
Version 3, which you can find in the COPYING file.

All graphical assets are licensed under the
[Creative Commons Attribution 3.0 Unported License](http://creativecommons.org/licenses/by/3.0/).

