cmake_minimum_required (VERSION 2.6)
project (nextwall)

# The version number.
set (nextwall_VERSION_MAJOR 0)
set (nextwall_VERSION_MINOR 9)

# Set the install prefix.
#set (CMAKE_INSTALL_PREFIX /usr)

# Pass some of the CMake settings to the nextwall script.
configure_file (
  "${PROJECT_SOURCE_DIR}/scripts/nextwall.in"
  "${PROJECT_BINARY_DIR}/scripts/nextwall"
  )

# Add the install targets.
install (FILES nextwall.py indicator.py DESTINATION share/nextwall)
install (DIRECTORY glade DESTINATION share/nextwall)
install (PROGRAMS ${PROJECT_BINARY_DIR}/scripts/nextwall DESTINATION bin)

# Add the install targets for the application icons.
install (FILES ../icons/nextwall-48.png DESTINATION share/icons/hicolor/48x48/apps RENAME nextwall.png)
install (FILES ../icons/nextwall-22.svg DESTINATION share/icons/hicolor/22x22/status RENAME nextwall.svg)
install (FILES ../icons/nextwall-24.svg DESTINATION share/icons/hicolor/24x24/status RENAME nextwall.svg)
install (FILES ../icons/nextwall-48.svg DESTINATION share/icons/hicolor/scalable/apps RENAME nextwall.svg)
