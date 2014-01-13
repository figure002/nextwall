/*
  This file is part of nextwall - a wallpaper rotator with some sense of time.

   Copyright 2004, Davyd Madeley <davyd@madeley.id.au>
   Copyright 2010-2013, Serrano Pereira <serrano.pereira@gmail.com>

   Nextwall is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Nextwall is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "std.h"

/**
  Converts hours to the format hh:mm.

  @param[in] hours Hours as a floating point number.
  @param[out] dest Time in the format hh:mm.
  @return Pointer to the time in format hh:mm.
 */
char *hours_to_hm(double hours, char *dest) {
    char hm[6];
    double h = floor(hours);
    double m = (hours - h) * 60.0;
    snprintf(hm, sizeof hm, "%02.0f:%02.0f", h, m);
    strcpy(dest, hm);
    return dest;
}

/**
  Compare two floats `pa` and `pb`.

  @param[in] pa First float value.
  @param[in] pa Second float value.
  @return Returns -1, 0, or 1 if `pa` is less, equal, or greater than pb
          respectively.
 */
int floatcmp(const void *pa, const void *pb) {
    float a = *(const float*)pa;
    float b = *(const float*)pb;
    if (a < b)
        return -1;
    else if (a > b)
        return 1;
    else
        return 0;
}

