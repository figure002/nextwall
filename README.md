# nextwall

This is the README file for nextwall - A wallpaper rotator for the GNOME
desktop.

Nextwall is a small application that changes the background of the GNOME
desktop to a random image. Nextwall has the following features:

* Operates as a command-line tool. Run `nextwall --help` for usage information.
* The fit time of day feature automatically sets backgrounds that fit the time
  of the day (dark backgrounds at night, bright backgrounds at day,
  intermediate at twilight). It uses an Artificial Neural Network to determine
  the brightness of a wallpaper.

[![Build Status](https://travis-ci.org/figure002/nextwall.svg?branch=master)](https://travis-ci.org/figure002/nextwall)

- - -

Nextwall also comes with a GNOME Shell extension.

![Screenshot](https://raw.github.com/figure002/nextwall/master/data/screenshot-extension.png)


## Setup

To compile `nextwall` you need to have `gcc` and `automake` as well as the
following development libraries:

* check
* help2man
* libfann
* libglib2.0
* libmagic
* libmagickwand
* libreadline
* libsqlite3

On Debian (based) systems, run this command to install the dependencies:

    apt-get install build-essential automake check help2man libfann-dev \
    libglib2.0-dev libmagic-dev libmagickwand-dev libreadline-dev libsqlite3-dev

If you're building `nextwall` from the Git repository, you first need to use
GNU Autotools to make the GNU Build System files before the below commands work.
This can be done with the shell command `autoreconf --install`.

You can then build and install `nextwall`. See the INSTALL file for building
and installation instructions. Briefly, the shell commands
`./configure; make; make install` should configure, build, and install this
package.


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

Nextwall comes with a trainer for the Artificial Neural Network (ANN).
Run `nextwall-trainer --help` to see usage information for the trainer. To use
this trainer, first create a directory with image files that you want to train
the ANN on. Then you can run the trainer as follows:

    nextwall-trainer -p30 ~/Pictures/train/

This will train the ANN on 30 image files from the directory
`~/Pictures/train/`. Then follow the instructions on the screen to start
training. Once completed, two files will be created:

* nextwall.dat - The training data
* nextwall.net - The artificial neural network

The ANN is created from the training data. Now copy the ANN (nextwall.net)
to `~/.local/share/nextwall/` to have `nextwall` use this ANN the next time
it is executed with the `--scan` option.

You can update the ANN by using an existing training data file as follows:

    nextwall-trainer -r

This will create/update nextwall.net from an existing nextwall.dat file. This
is useful if you want to create a new ANN with different parameters, without
having to rebuild the training data.


## License

Nextwall is free software. See the file COPYING for copying conditions.

