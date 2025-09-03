/*
  This file is part of nextwall - a wallpaper rotator with some sense of time.

   Copyright 2004, Davyd Madeley <davyd@madeley.id.au>
   Copyright 2010-2013, Serrano Pereira <serrano@bitosis.nl>

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

#include <gio/gio.h>
#include <stdio.h>
#include <bsd/string.h> /* strlcpy */

#include "gnome.h"

/**
  Set the desktop background.

  @param[in] settings GSettings object with desktop background schema.
  @param[in] path The wallpaper path to set.
  @return Returns 0 on success, -1 on failure.
 */
int set_background_uri(GSettings *settings, const char *path) {
    char normalized_path[PATH_MAX];
    static char picture_param[] = "picture-uri-dark";

    if (strstr(path, "file://") == NULL) {
        if (snprintf(normalized_path,
                     PATH_MAX,
                     "file://%s", path) >= PATH_MAX) {
            return -1;
        }
    }
    else {
        if (strlcpy(normalized_path,
                    path,
                    PATH_MAX) >= PATH_MAX) {
            return -1;
        }
    }

    g_assert(g_settings_set(settings, picture_param, "s", normalized_path));
    g_settings_sync(); // Make sure the changes are written to disk

    if (strcmp(g_settings_get_string(settings, picture_param), normalized_path) == 0) {
        return 0;
    }

    return -1;
}

/**
  Get the current desktop background.

  @param[in] settings GSettings object with desktop background schema.
  @param[out] dest Is set to the URI of the current desktop background.
 */
int get_background_uri(GSettings *settings, char *dest) {
    const char *uri;
    static char picture_param[] = "picture-uri-dark";

    uri = g_variant_get_string(g_settings_get_value(settings, picture_param), NULL);

    // Strip off the "file://" part of the URI.
    if (strlcpy(dest, uri + 7, PATH_MAX) >= PATH_MAX) {
        return -1;
    }

    return 0;
}

/**
  Launch the default application for an image path.

  @param[in] path Path or uri for the image.
  @return Returns 0 on success, -1 on error.
 */
int open_image(char *path) {
    gboolean success;
    GError *error = NULL;
    char uri[PATH_MAX];

    if (strstr(path, "file://") == NULL) {
        if (snprintf(uri,
                     PATH_MAX,
                     "file://%s", path) >= PATH_MAX) {
            return -1;
        }
    }
    else {
        if (strlcpy(uri, path, PATH_MAX) >= PATH_MAX) {
            return FALSE;
        }
    }

    success = g_app_info_launch_default_for_uri(uri, NULL, &error);

    if (success == FALSE) {
        g_message("%s", error->message);
        return -1;
    }

    return 0;
}

/**
  Move a file to trash.

  @param[in] path Path for the file.
  @return Returns 0 on success, -1 on error.
 */
int file_trash(char *path) {
    gboolean success;
    GFile *file;
    GError *error = NULL;

    file = g_file_new_for_path(path);
    success = g_file_trash(file, NULL, &error);

    if (success) {
        printf("Successfully moved '%s' to the trash.\n", path);
    }
    else {
        fprintf(stderr, "%s\n", error->message);
        g_error_free(error);
    }

    g_object_unref(file);

    return success ? 0 : -1;
}
