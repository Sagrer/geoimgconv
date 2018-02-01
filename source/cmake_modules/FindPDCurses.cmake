# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindPDCurses
# ----------
#
# Find the PDcurses include file and library.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ``PDCURSES_FOUND``
#   True if Curses is found.
# ``PDCURSES_INCLUDE_DIRS``
#   The include directories needed to use Curses.
# ``PDCURSES_LIBRARIES``
#   The libraries needed to use Curses.
# ``PDCURSES_HAVE_CURSES_H``
#   True if curses.h is available.

include(${CMAKE_CURRENT_LIST_DIR}/CheckLibraryExists.cmake)

find_library(PDCURSES_CURSES_LIBRARY NAMES pdcurses )

get_filename_component(_pdcursesLibDir "${PDCURSES_CURSES_LIBRARY}" PATH)
get_filename_component(_pdcursesParentDir "${_pdcursesLibDir}" PATH)

find_path(PDCURSES_INCLUDE_PATH
      NAMES curses.h
      HINTS "${_pdcursesParentDir}/include/pdcurses"
      )
message("PDCURSES: PDCURSES_INCLUDE_PATH=" ${PDCURSES_INCLUDE_PATH})
	
if(EXISTS "${PDCURSES_INCLUDE_PATH}/pdcurses/curses.h")
    set(PDCURSES_HAVE_CURSES_H "${PDCURSES_INCLUDE_PATH}/pdcurses/curses.h")
else()
    set(PDCURSES_HAVE_CURSES_H "PDCURSES_HAVE_CURSES_H-NOTFOUND")
endif()

if(NOT DEFINED PDCURSES_LIBRARIES)
	set(PDCURSES_LIBRARIES "${PDCURSES_CURSES_LIBRARY}")
endif()
  
set(PDCURSES_INCLUDE_DIRS ${PDCURSES_INCLUDE_PATH})

include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PDCurses DEFAULT_MSG
  PDCURSES_LIBRARIES PDCURSES_INCLUDE_PATH)

mark_as_advanced(
  PDCURSES_INCLUDE_PATH
  PDCURSES_CURSES_LIBRARY
  )
