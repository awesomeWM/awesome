set(PROJECT_AWE_NAME awesome)
set(PROJECT_AWECLIENT_NAME awesome-client)

# If ${SOURCE_DIR} is a git repository VERSION is set to
# `git describe` later.
set(VERSION devel)

set(CODENAME "The Golden Floor")

project(${PROJECT_AWE_NAME} C)

set(CMAKE_BUILD_TYPE RELEASE)

set(CURSES_NEED_NCURSES true)

option(WITH_DBUS "build with D-BUS" ON)
option(GENERATE_MANPAGES "generate manpages" ON)
option(GENERATE_LUADOC "generate luadoc" ON)

link_directories(/usr/local/lib)

# {{{ CFLAGS
add_definitions(-std=gnu99 -ggdb3 -fno-strict-aliasing -Wall -Wextra
    -Wchar-subscripts -Wundef -Wshadow -Wcast-align -Wwrite-strings
    -Wsign-compare -Wunused -Wno-unused-parameter -Wuninitialized -Winit-self
    -Wpointer-arith -Wredundant-decls -Wformat-nonliteral
    -Wno-format-zero-length -Wmissing-format-attribute -Wmissing-prototypes
    -Wstrict-prototypes)
# }}}

# {{{ Find external utilities
macro(a_find_program var prg req)
    set(required ${req})
    find_program(${var} ${prg})
    if(NOT ${var})
        message(STATUS "${prg} not found.")
        if(required)
            message(FATAL_ERROR "${prg} is required to build awesome")
        endif()
    else()
        message(STATUS "${prg} -> ${${var}}")
    endif()
endmacro()

a_find_program(CAT_EXECUTABLE cat TRUE)
a_find_program(LN_EXECUTABLE ln TRUE)
a_find_program(GREP_EXECUTABLE grep TRUE)
a_find_program(GIT_EXECUTABLE git FALSE)
a_find_program(HOSTNAME_EXECUTABLE hostname FALSE)
a_find_program(GPERF_EXECUTABLE gperf TRUE)
# programs needed for man pages
a_find_program(ASCIIDOC_EXECUTABLE asciidoc FALSE)
a_find_program(XMLTO_EXECUTABLE xmlto FALSE)
a_find_program(GZIP_EXECUTABLE gzip FALSE)
# lua documentation
a_find_program(LUA_EXECUTABLE lua FALSE)
a_find_program(LUADOC_EXECUTABLE luadoc FALSE)
# doxygen
include(FindDoxygen)
# pkg-config
include(FindPkgConfig)
# ncurses
include(FindCurses)
# lua 5.1
include(FindLua51) #Due to a cmake bug, you will see Lua50 on screen
# }}}

# {{{ Check if documentation can be build
if(GENERATE_MANPAGES)
    if(NOT ASCIIDOC_EXECUTABLE OR NOT XMLTO_EXECUTABLE OR NOT GZIP_EXECUTABLE)
        if(NOT ASCIIDOC_EXECUTABLE)
            SET(missing "asciidoc")
        endif()
        if(NOT XMLTO_EXECUTABLE)
            SET(missing ${missing} " xmlto")
        endif()
        if(NOT GZIP_EXECUTABLE)
            SET(missing ${missing} " gzip")
        endif()

        message(STATUS "Not generating manpages. Missing: " ${missing})
        set(GENERATE_MANPAGES OFF)
    endif()
endif()

if(GENERATE_LUADOC)
    if(NOT LUADOC_EXECUTABLE)
        message(STATUS "Not generating luadoc. Missing: luadoc")
        set(GENERATE_LUADOC OFF)
    endif()
endif()
# }}}

# {{{ Version stamp
if(EXISTS ${SOURCE_DIR}/.git/HEAD AND GIT_EXECUTABLE)
    # get current version
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe
        WORKING_DIRECTORY ${SOURCE_DIR}
        OUTPUT_VARIABLE VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    # file the git-version-stamp.sh script will look into
    set(VERSION_STAMP_FILE ${BUILD_DIR}/.version_stamp)
    file(WRITE ${VERSION_STAMP_FILE} ${VERSION})
    # create a version_stamp target later
    set(BUILD_FROM_GIT TRUE)
elseif( EXISTS ${SOURCE_DIR}/.version_stamp )
    # get version from version stamp
    file(READ ${SOURCE_DIR}/.version_stamp VERSION)
endif()
# }}}

# {{{ Get hostname
execute_process(
    COMMAND ${HOSTNAME_EXECUTABLE}
    WORKING_DIRECTORY ${SOURCE_DIR}
    OUTPUT_VARIABLE BUILDHOSTNAME
    OUTPUT_STRIP_TRAILING_WHITESPACE)
# }}}

# {{{ Required libraries
#
# this sets up:
# AWESOME_REQUIRED_LIBRARIES
# AWESOME_REQUIRED_INCLUDE_DIRS
# AWESOMECLIENT_LIBRARIES

# Use pkgconfig to get most of the libraries
pkg_check_modules(AWESOME_COMMON_REQUIRED REQUIRED
    xcb)

pkg_check_modules(AWESOME_REQUIRED REQUIRED
    glib-2.0
    cairo
    pango
    pangocairo
    x11-xcb
    xcb-randr
    xcb-xinerama
    xcb-event>=0.3.0
    xcb-aux>=0.3.0
    xcb-atom>=0.3.0
    xcb-keysyms>=0.3.0
    xcb-icccm>=0.3.0
    cairo-xcb
    xproto>=7.0.11
    imlib2)

if(NOT AWESOME_REQUIRED_FOUND OR NOT AWESOME_COMMON_REQUIRED_FOUND)
    message(FATAL_ERROR)
endif()

macro(a_find_library variable library)
    find_library(${variable} ${library})
    if(NOT ${variable})
        message(FATAL_ERROR ${library} " library not found.")
    endif()
endmacro()

# Check for readline, ncurse and libev
a_find_library(LIB_READLINE readline)
a_find_library(LIB_EV ev)

# Error check
set( LUA_FOUND LUA51_FOUND OR LUA50_FOUND )# This is a workaround to a cmake bug
if(NOT LUA_FOUND)
    message(FATAL_ERROR "lua library not found")
endif()

set(AWESOME_REQUIRED_LIBRARIES
    ${AWESOME_COMMON_REQUIRED_LIBRARIES}
    ${AWESOME_REQUIRED_LIBRARIES}
    ${LIB_EV}
    ${LUA_LIBRARIES})

set(AWESOME_REQUIRED_INCLUDE_DIRS
    ${AWESOME_COMMON_REQUIRED_INCLUDE_DIRS}
    ${AWESOME_REQUIRED_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR})

set(AWESOMECLIENT_LIBRARIES
    ${AWESOME_COMMON_REQUIRED_LIBRARIES}
    ${LIB_READLINE}
    ${LIB_XCB}
    ${CURSES_LIBRARIES})

set(AWESOMECLIENT_REQUIRED_INCLUDE_DIRS
    ${AWESOME_COMMON_REQUIRED_INCLUDE_DIRS})
# }}}

# {{{ Optional libraries
#
# this sets up:
# AWESOME_OPTIONAL_LIBRARIES
# AWESOME_OPTIONAL_INCLUDE_DIRS

if(WITH_DBUS)
    pkg_check_modules(DBUS dbus-1)
    if(DBUS_FOUND)
        set(AWESOME_OPTIONAL_LIBRARIES ${AWESOME_OPTIONAL_LIBRARIES} ${DBUS_LIBRARIES})
        set(AWESOME_OPTIONAL_INCLUDE_DIRS ${AWESOME_OPTIONAL_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS})
    else()
        set(WITH_DBUS OFF)
        message(STATUS "DBUS not found. Disabled.")
    endif()
endif()
# }}}

# {{{ Install path and configuration variables
if(DEFINED PREFIX)
    set(PREFIX ${PREFIX} CACHE PATH "install prefix")
    set(CMAKE_INSTALL_PREFIX ${PREFIX})
else()
    set(PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "install prefix")
endif()

#If a sysconfdir is specified, use it instead
#of the default configuration dir.
if(DEFINED SYSCONFDIR)
    set(SYSCONFDIR ${SYSCONFDIR} CACHE PATH "config directory")
else()
    set(SYSCONFDIR /etc CACHE PATH "config directory")
endif()

#If an XDG Config Dir is specificed, use it instead
#of the default XDG configuration dir.
if(DEFINED XDG_CONFIG_DIR)
    set(XDG_CONFIG_DIR ${XDG_CONFIG_SYS} CACHE PATH "xdg config directory")
else()
    set(XDG_CONFIG_DIR ${SYSCONFDIR}/xdg CACHE PATH "xdg config directory")
endif()

# setting AWESOME_DOC_PATH
if(DEFINED AWESOME_DOC_PATH)
    set(AWESOME_DOC_PATH ${AWESOME_DOC_PATH} CACHE PATH "awesome docs directory")
else()
    set(AWESOME_DOC_PATH ${PREFIX}/share/doc/${PROJECT_AWE_NAME} CACHE PATH "awesome docs directory")
endif()

# setting AWESOME_XSESSION_PATH
if(DEFINED AWESOME_XSESSION_PATH)
    set(AWESOME_XSESSION_PATH ${AWESOME_XSESSION_PATH} CACHE PATH "awesome xsessions directory")
else()
    set(AWESOME_XSESSION_PATH ${PREFIX}/share/xsessions CACHE PATH "awesome xsessions directory")
endif()

# set man path
if(DEFINED AWESOME_MAN_PATH)
   set(AWESOME_MAN_PATH ${AWESOME_MAN_PATH} CACHE PATH "awesome manpage directory")
else()
   set(AWESOME_MAN_PATH ${PREFIX}/share/man CACHE PATH "awesome manpage directory")
endif()

# Hide to avoid confusion
mark_as_advanced(CMAKE_INSTALL_PREFIX)

set(AWESOME_VERSION          ${VERSION})
set(AWESOME_COMPILE_MACHINE  ${CMAKE_SYSTEM_PROCESSOR})
set(AWESOME_COMPILE_HOSTNAME ${BUILDHOSTNAME})
set(AWESOME_COMPILE_BY       $ENV{USER})
set(AWESOME_RELEASE          ${CODENAME})
set(AWESOME_SYSCONFDIR       ${XDG_CONFIG_DIR}/${PROJECT_AWE_NAME})
set(AWESOME_DATA_PATH        ${PREFIX}/share/${PROJECT_AWE_NAME})
set(AWESOME_LUA_LIB_PATH     ${AWESOME_DATA_PATH}/lib)
set(AWESOME_ICON_PATH        ${AWESOME_DATA_PATH}/icons)
set(AWESOME_THEMES_PATH      ${AWESOME_DATA_PATH}/themes)
# }}}

# {{{ Configure files
file(GLOB_RECURSE awesome_lua_configure_files RELATIVE ${SOURCE_DIR} ${SOURCE_DIR}/lib/*.lua.in)
set(AWESOME_CONFIGURE_FILES 
    ${awesome_lua_configure_files}
    config.h.in
    awesomerc.lua.in
    themes/default/theme.in
    themes/sky/theme.in
    awesome-version-internal.h.in
    awesome.doxygen.in)

macro(a_configure_file file)
    string(REGEX REPLACE ".in\$" "" outfile ${file})
    message(STATUS "Configuring ${outfile}")
    configure_file(${SOURCE_DIR}/${file}
                   ${BUILD_DIR}/${outfile}
                   ESCAPE_QUOTE
                   @ONLY)
endmacro()

foreach(file ${AWESOME_CONFIGURE_FILES})
    a_configure_file(${file})
endforeach()
#}}}

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
