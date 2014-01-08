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

#include <wand/MagickWand.h>

#include "image.h"

/**
  Returns the kurtosis and lightness value for an image file.

  @param[in] path Absolute path of the image file.
  @param[out] kurtosis The kurtosis value.
  @param[out] lightness The lightness value.
  @return Retuns 0 on success, -1 on failure.
 */
int get_image_info(const char *path, double *kurtosis, double *lightness) {
    MagickBooleanType status;
    MagickWand *magick_wand;
    PixelWand *pixel_wand;
    double hue, saturation, skewness;

    MagickWandGenesis();
    magick_wand = NewMagickWand();
    pixel_wand = NewPixelWand();

    // Read the image
    status = MagickReadImage(magick_wand, path);
    if (status == MagickFalse)
        return -1;

    // Get the kurtosis value
    MagickGetImageChannelKurtosis(magick_wand, DefaultChannels, kurtosis,
            &skewness);

    // Resize the image to 1x1 pixel (results in average color)
    MagickResizeImage(magick_wand, 1, 1, LanczosFilter, 1);

    // Get pixel color
    status = MagickGetImagePixelColor(magick_wand, 0, 0, pixel_wand);
    if (status == MagickFalse) {
        return -1;
    }

    // Get the lightness value
    PixelGetHSL(pixel_wand, &hue, &saturation, lightness);

    pixel_wand = DestroyPixelWand(pixel_wand);
    magick_wand = DestroyMagickWand(magick_wand);
    MagickWandTerminus();

    return 0;
}

