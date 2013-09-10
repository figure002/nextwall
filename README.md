# nextwall

This is the README file for nextwall - A wallpaper rotator for the GNOME
desktop.

*Nextwall* is a small application that changes the background of the GNOME
desktop to a random image. Nextwall has the following features:

* Operates as a command-line tool. Run `nextwall --help` for usage information.
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

You can then build and install `nextwall`. See the INSTALL file for building
and installation instructions. Generally it boils down to:

    ./configure
    make
    make install

If you're building `nextwall` from the Git repository, you need to use GNU
Autotools to bring the package to autoconfiscated state before the above
commands can be excuted:

	aclocal
	autoconf
	automake -a


## Usage

Execute `nextwall --help` to see usage information for `nextwall`.

When running `nextwall` for the first time, you need to scan your wallpaper
directory for wallpapers:

	nextwall -sr /path/to/wallpapers/

Then `nextwall` can be used as follows:

	nextwall [OPTION...] PATH


### Usage Tips

* You can use `watch` to temporarily change the background at a set interval.
  For 60 seconds, run `watch -n 60 nextwall [OPTION...] PATH`
* You can also use the cron deamon to schedule execution of `nextwall`. For
  once an hour, run `crontab -e` and add this line:
  ``0 * * * * DISPLAY=:0.0 /usr/local/bin/nextwall [OPTION...] PATH``


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
to `~/.local/share/nextwall/` to have `nextwall` use this ANN the next time
it is executed with the `--scan` option.

You can also create the ANN by using an existing training data file as
follows:

    ./train_ann -r ~/Pictures/train/

This will create nextwall.net from an existing nextwall.dat file.

## License

Nextwall is free software. See the file COPYING for copying conditions.

