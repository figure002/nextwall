# CPack nextwall config

set (CPACK_GENERATORS "TGZ")
set (CPACK_SOURCE_GENERATORS "TGZ")
set (CPACK_STRIP_FILES ON)
set (CPACK_SOURCE_IGNORE_FILES ".bzr;build;.swp;push.sh;icons/src")

set (CPACK_DEBIAN_PACKAGE_NAME "nextwall")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Wallpaper changer for GNOME\n NextWall is a small script that changes the wallpaper of the GNOME desktop to a random image. NextWall can operate as a command-line tool, but can also place itself in the GNOME panel. The fit time of day feature automatically sets wallpapers that fit the time of day (dark backgrounds at night, bright backgrounds at day, intermediate at dusk/dawn).")
set (CPACK_DEBIAN_PACKAGE_MAINTAINER "Serrano Pereira <serrano.pereira@gmail.com>")
set (CPACK_PACKAGE_CONTACT "serrano.pereira@gmail.com")
set (CPACK_DEBIAN_PACKAGE_DEPENDS "python (>=2.6), python (<2.7), python-gtk2, imagemagick, libappindicator-dev, python-appindicator")
set (CPACK_DEBIAN_PACKAGE_SECTION "GNOME")
set (CPACK_DEBIAN_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})

set (CPACK_BINARY_RPM OFF)
set (CPACK_BINARY_DEB ON)
set (CPACK_BINARY_Z OFF)
set (CPACK_BINARY_TGZ OFF)

set (CPACK_SOURCE_TGZ ON)
set (CPACK_SOURCE_Z OFF)
set (CPACK_SOURCE_TZ OFF)
set (CPACK_SOURCE_TBZ2 OFF)

set (CPACK_PACKAGE_VERSION_MAJOR ${nextwall_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${nextwall_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${nextwall_VERSION_PATCH})

include (CPack)

