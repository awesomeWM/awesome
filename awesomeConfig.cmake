SET(PROJECT_AWE_NAME awesome)
SET(PROJECT_AWECLIENT_NAME awesome-client)

SET(VERSION 3)
SET(VERSION_MAJOR ${VERSION})
SET(VERSION_MINOR 0)
SET(VERSION_PATCH 0)

SET(CODENAME "Productivity Breaker")

PROJECT(${PROJECT_AWE_NAME})

SET(CMAKE_BUILD_TYPE RELEASE)

OPTION(WITH_DBUS "build with D-BUS" ON)
OPTION(WITH_IMLIB2 "build with Imlib2" ON)
OPTION(GENERATE_MANPAGES "generate manpages" ON)

# {{{ CFLAGS
ADD_DEFINITIONS(-std=gnu99 -ggdb3 -fno-strict-aliasing -Wall -Wextra
    -Wchar-subscripts -Wundef -Wshadow -Wcast-align -Wwrite-strings
    -Wsign-compare -Wunused -Wno-unused-parameter -Wuninitialized -Winit-self
    -Wpointer-arith -Wredundant-decls -Wformat-nonliteral
    -Wno-format-zero-length -Wmissing-format-attribute)
# }}}

# {{{ Find external utilities
FIND_PROGRAM(CAT_EXECUTABLE cat)
FIND_PROGRAM(GREP_EXECUTABLE grep)
FIND_PROGRAM(GIT_EXECUTABLE git)
FIND_PROGRAM(LUA_EXECUTABLE lua)
# programs needed for man pages
FIND_PROGRAM(ASCIIDOC_EXECUTABLE asciidoc)
FIND_PROGRAM(XMLTO_EXECUTABLE xmlto)
FIND_PROGRAM(GZIP_EXECUTABLE gzip)
# doxygen
INCLUDE(FindDoxygen)
# pkg-config
INCLUDE(FindPkgConfig)
# }}}

# {{{ Check if manpages can be build
IF(GENERATE_MANPAGES)
    IF(NOT ASCIIDOC_EXECUTABLE OR NOT XMLTO_EXECUTABLE OR NOT GZIP_EXECUTABLE)
        IF(NOT ASCIIDOC_EXECUTABLE)
            SET(missing "asciidoc")
        ENDIF()
        IF(NOT XMLTO_EXECUTABLE)
            SET(missing ${missing} " xmlto")
        ENDIF()
        IF(NOT GZIP_EXECUTABLE)
            SET(missing ${missing} " gzip")
        ENDIF()

        MESSAGE(STATUS "Not generating manpages. Missing: " ${missing})
        SET(GENERATE_MANPAGES OFF)
    ENDIF()
ENDIF()
# }}}

# {{{ git version stamp
# If this is a git repository...
IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git/HEAD)
    IF(GIT_EXECUTABLE)
        # get current version
        EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} describe
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        OUTPUT_VARIABLE VERSION
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
        # file the git-version-stamp.sh script will look into
        SET(VERSION_STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/.version_stamp)
        FILE(WRITE ${VERSION_STAMP_FILE} ${VERSION})
        # create a version_stamp target later
        SET(BUILD_FROM_GIT TRUE)
    ENDIF()
ENDIF()
# }}}

# {{{ Required libraries
#
# this sets up:
# AWESOME_REQUIRED_LIBRARIES
# AWESOME_REQUIRED_INCLUDE_DIRS

# Use pkgconfig to get most of the libraries
pkg_check_modules(AWESOME_REQUIRED REQUIRED
    glib-2.0
    cairo
    pango
    gdk-2.0>=2.2
    gdk-pixbuf-2.0>=2.2
    xcb
    xcb-event
    xcb-randr
    xcb-xinerama
    xcb-shape
    xcb-aux
    xcb-atom
    xcb-keysyms
    xcb-render
    xcb-icccm
    cairo-xcb)

MACRO(a_find_library variable library)
    FIND_LIBRARY(${variable} ${library})
    IF(NOT ${variable})
        MESSAGE(FATAL_ERROR ${library} " library not found.")
    ENDIF()
ENDMACRO()

# Check for readline, ncurse and libev
a_find_library(LIB_READLINE readline)
a_find_library(LIB_NCURSES ncurses)
a_find_library(LIB_EV ev)

# Check for lua5.1
FIND_PATH(LUA_INC_DIR lua.h
    /usr/include
    /usr/include/lua5.1
    /usr/local/include/lua5.1)

FIND_LIBRARY(LUA_LIB NAMES lua5.1 lua
    /usr/lib
    /usr/lib/lua
    /usr/local/lib)

# Error check
IF(NOT LUA_LIB)
    MESSAGE(FATAL_ERROR "lua library not found")
ENDIF()

SET(AWESOME_REQUIRED_LIBRARIES ${AWESOME_REQUIRED_LIBRARIES}
        ${LIB_READLINE}
        ${LIB_NCURSES}
        ${LIB_EV}
        ${LUA_LIB})

SET(AWESOME_REQUIRED_INCLUDE_DIRS ${AWESOME_REQUIRED_INCLUDE_DIRS} ${LUA_INC_DIR})
# }}}

# {{{ Optional libraries.
#
# this sets up:
# AWESOME_OPTIONAL_LIBRARIES
# AWESOME_OPTIONAL_INCLUDE_DIRS

IF(WITH_DBUS)
    pkg_check_modules(DBUS dbus-1)
    IF(DBUS_FOUND)
        SET(AWESOME_OPTIONAL_LIBRARIES ${AWESOME_OPTIONAL_LIBRARIES} ${DBUS_LIBRARIES})
        SET(AWESOME_OPTIONAL_INCLUDE_DIRS ${AWESOME_OPTIONAL_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS})
    ELSE()
        SET(WITH_DBUS OFF)
        MESSAGE(STATUS "DBUS not found. Disabled.")
    ENDIF()
ENDIF()

IF(WITH_IMLIB2)
    pkg_check_modules(IMLIB2 imlib2)
    IF(IMLIB2_FOUND)
        SET(AWESOME_OPTIONAL_LIBRARIES ${AWESOME_OPTIONAL_LIBRARIES} ${IMLIB2_LIBRARIES})
        SET(AWESOME_OPTIONAL_INCLUDE_DIRS ${AWESOME_OPTIONAL_INCLUDE_DIRS} ${IMLIB2_INCLUDE_DIRS})
    ELSE()
        SET(WITH_IMLIB2 OFF)
        MESSAGE(STATUS "Imlib2 not found. Disabled.")
    ENDIF()
ENDIF()
# }}}

# {{{ Install path and configuration variables.
IF(DEFINED PREFIX)
    SET(CMAKE_INSTALL_PREFIX ${PREFIX})
ENDIF()
SET(AWESOME_VERSION          ${VERSION} )
SET(AWESOME_COMPILE_MACHINE  ${CMAKE_SYSTEM_PROCESSOR} )
SET(AWESOME_COMPILE_HOSTNAME $ENV{HOSTNAME} )
SET(AWESOME_COMPILE_BY       $ENV{USER} )
SET(AWESOME_RELEASE          ${CODENAME} )
SET(AWESOME_ETC              etc )
SET(AWESOME_SHARE            share )
SET(AWESOME_LUA_LIB_PATH     ${CMAKE_INSTALL_PREFIX}/${AWESOME_SHARE}/${PROJECT_AWE_NAME}/lib )
SET(AWESOME_ICON_PATH        ${CMAKE_INSTALL_PREFIX}/${AWESOME_SHARE}/${PROJECT_AWE_NAME}/icons )
SET(AWESOME_CONF_PATH        ${CMAKE_INSTALL_PREFIX}/${AWESOME_ETC}/${PROJECT_AWE_NAME} )
SET(AWESOME_MAN1_PATH        ${AWESOME_SHARE}/man/man1 )
SET(AWESOME_MAN5_PATH        ${AWESOME_SHARE}/man/man5 )
SET(AWESOME_REL_LUA_LIB_PATH ${AWESOME_SHARE}/${PROJECT_AWE_NAME}/lib )
SET(AWESOME_REL_CONF_PATH    ${AWESOME_ETC}/${PROJECT_AWE_NAME} )
SET(AWESOME_REL_ICON_PATH    ${AWESOME_SHARE}/${PROJECT_AWE_NAME} )
SET(AWESOME_REL_DOC_PATH     ${AWESOME_SHARE}/doc/${PROJECT_AWE_NAME})
# }}}

# {{{ Configure files.
SET (AWESOME_CONFIGURE_FILES config.h.in
                             awesomerc.lua.in
                             awesome-version-internal.h.in
                             awesome.doxygen.in)

MACRO(a_configure_file file)
    STRING(REGEX REPLACE ".in\$" "" outfile ${file})
    MESSAGE(STATUS "Configuring ${outfile}")
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${file}
                   ${CMAKE_CURRENT_BINARY_DIR}/${outfile}
                   ESCAPE_QUOTE
                   @ONLY)
ENDMACRO()

FOREACH(file ${AWESOME_CONFIGURE_FILES})
    a_configure_file(${file})
ENDFOREACH()
#}}}

# {{{ CPack configuration
SET(CPACK_PACKAGE_NAME                 "${PROJECT_AWE_NAME}")
SET(CPACK_GENERATOR                    "TBZ2")
SET(CPACK_SOURCE_GENERATOR             "TBZ2")
SET(CPACK_SOURCE_IGNORE_FILES
    ".git;.*.swp$;.*~;.*patch;.gitignore;${CMAKE_CURRENT_BINARY_DIR}")

FOREACH(file ${AWESOME_CONFIGURE_FILES}) 
    STRING(REPLACE ".in" "" confheader ${file})
    SET( CPACK_SOURCE_IGNORE_FILES
        ";${CPACK_SOURCE_IGNORE_FILES};${confheader}$;" )
ENDFOREACH()

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY  "A dynamic floating and tiling window manager")
SET(CPACK_PACKAGE_VENDOR               "awesome development team")
SET(CPACK_PACKAGE_DESCRIPTION_FILE     "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR        "${VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR        "${VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH        "${VERSION_PATCH}")

INCLUDE(CPack)
#}}}

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
