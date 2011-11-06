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

import os
import commands
import datetime
import dateutil.tz

from sun import Sun

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


def is_command(command):
    """Check if a specific command is available."""
    cmd = 'which %s' % (command)
    output = commands.getoutput(cmd)
    if len(output.splitlines()) == 0:
        return False
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

def from_utc_to_local(value):
    """Convert UTC time to local time in format H:M.

    Function taken from My-Weather-Indicator.
    Copyright 2011, Lorenzo Carbonell <lorenzo.carbonell.cerezo@gmail.com>
    """
    if value<0:
        value = value+24
    if value >24:
        value = value-24
    hours = int(value)
    minutes = int((value-int(value))*60.0)
    now = datetime.datetime.now()
    utctime = datetime.datetime(now.year, now.month, now.day, hours, minutes, 0, 0,tzinfo=dateutil.tz.tzutc())
    localtime = utctime.astimezone(dateutil.tz.tzlocal())
    return localtime.strftime('%H:%M')

def utc_childs_to_local(list):
    return [from_utc_to_local(x) for x in list]

def get_sunrise(day, longitude, latitude):
    sun = Sun()
    ss = sun.sunRiseSet(day.year, day.month, day.day, longitude, latitude)
    ss = utc_childs_to_local(ss)
    t = [int(x) for x in ss[0].split(':')]
    return datetime.time(t[0], t[1])

def get_sunset(day, longitude, latitude):
    sun = Sun()
    ss = sun.sunRiseSet(day.year, day.month, day.day, longitude, latitude)
    ss = utc_childs_to_local(ss)
    t = [int(x) for x in ss[1].split(':')]
    return datetime.time(t[0], t[1])

def get_twilight(day, longitude, latitude):
    sun = Sun()
    ss = sun.civilTwilight(day.year, day.month, day.day, longitude, latitude)
    a,b = utc_childs_to_local(ss)
    a = [int(x) for x in a.split(':')]
    b = [int(x) for x in b.split(':')]
    return datetime.time(a[0],a[1]), datetime.time(b[0],b[1])

def localtime():
    t = datetime.datetime.today().time()
    return t
