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
install (FILES nextwall.py indicator.py COPYING DESTINATION share/nextwall)
install (DIRECTORY glade DESTINATION share/nextwall)
install (PROGRAMS ${PROJECT_BINARY_DIR}/scripts/nextwall DESTINATION bin)

