cmake_minimum_required (VERSION 2.6)
project (nextwall)

# The version number.
set (nextwall_VERSION_MAJOR 0)
set (nextwall_VERSION_MINOR 9)

set (CMAKE_INSTALL_PREFIX /usr/share)

# add the install targets
install (FILES "${PROJECT_BINARY_DIR}/*.py COPYING" DESTINATION nextwall)
install (FILES "${PROJECT_BINARY_DIR}/nextwall" DESTINATION nextwall)
install (FILES "${PROJECT_BINARY_DIR}/docs" DESTINATION nextwall)
install (FILES "${PROJECT_BINARY_DIR}/glade" DESTINATION nextwall)
