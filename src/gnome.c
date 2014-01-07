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
#include <gio/gio.h>
#include <string.h>

/**
  Set the desktop background.

  @param[in] settings GSettings object with desktop background schema.
  @param[in] path The wallpaper path to set.
  @return Returns 0 on success, -1 on failure.
 */
int set_background_uri(GSettings *settings, const char *path) {
    char pathc[PATH_MAX];

    if (!strstr(path, "file://"))
        sprintf(pathc, "file://%s", path);
    else
        strcpy(pathc, path);

    g_assert(g_settings_set(settings, "picture-uri", "s", pathc));
    g_settings_sync(); // Make sure the changes are written to disk
    if ( strcmp(g_settings_get_string(settings, "picture-uri"), pathc) == 0 )
        return 0;
    else
        return -1;
}

/**
  Get the current desktop background.

  @param[in] settings GSettings object with desktop background schema.
  @param[out] dest Is set to the URI of the current desktop background.
 */
void get_background_uri(GSettings *settings, char *dest) {
    const char *uri;

    uri = g_variant_get_string(g_settings_get_value(settings, "picture-uri"), NULL);
    strcpy(dest, uri+7);
}

/**
  Launch the default application for an image path.

  @param[in] path Path or uri for the image.
  @return Returns TRUE on success, FALSE on error.
 */
int open_image(char *path) {
    gboolean ret;
    GError *error = NULL;
    char uri[PATH_MAX];

    if (!strstr(path, "file://"))
        sprintf(uri, "file://%s", path);
    else
        strcpy(uri, path);

    ret = g_app_info_launch_default_for_uri(uri, NULL, &error);
    if (!ret)
        g_message("%s", error->message);

    return ret;
}

/**
  Move a file to trash.

  @param[in] path Path for the file.
  @return Returns TRUE on success, FALSE on error.
 */
int file_trash(char *path) {
    gboolean ret;
    GFile *file;

    file = g_file_new_for_path(path);
    ret = g_file_trash(file, NULL, NULL);
    g_object_unref(file);

    return ret;
}

