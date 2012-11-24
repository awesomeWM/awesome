set(PROJECT_AWE_NAME awesome)

# If ${SOURCE_DIR} is a git repository VERSION is set to
# `git describe` later.
set(VERSION devel)

set(CODENAME "Dirty Magic")

project(${PROJECT_AWE_NAME} C)

set(CMAKE_BUILD_TYPE RELEASE)

option(WITH_DBUS "build with D-BUS" ON)
option(GENERATE_MANPAGES "generate manpages" ON)
option(COMPRESS_MANPAGES "compress manpages" ON)
option(GENERATE_DOC "generate API documentation" ON)

# {{{ CFLAGS
add_definitions(-std=gnu99 -ggdb3 -rdynamic -fno-strict-aliasing -Wall -Wextra
    -Wchar-subscripts -Wundef -Wshadow -Wcast-align -Wwrite-strings
    -Wsign-compare -Wunused -Wno-unused-parameter -Wuninitialized -Winit-self
    -Wpointer-arith -Wredundant-decls -Wformat-nonliteral
    -Wno-format-zero-length -Wmissing-format-attribute -Wmissing-prototypes
    -Wstrict-prototypes)
# }}}

# {{{ Endianness
include(TestBigEndian)
TEST_BIG_ENDIAN(AWESOME_IS_BIG_ENDIAN)
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
# programs needed for man pages
a_find_program(ASCIIDOC_EXECUTABLE asciidoc FALSE)
a_find_program(XMLTO_EXECUTABLE xmlto FALSE)
a_find_program(GZIP_EXECUTABLE gzip FALSE)
# lua documentation
a_find_program(LUA_EXECUTABLE lua FALSE)
a_find_program(LDOC_EXECUTABLE ldoc.lua FALSE)
# theme graphics
a_find_program(CONVERT_EXECUTABLE convert TRUE)
# doxygen
include(FindDoxygen)
# pkg-config
include(FindPkgConfig)
# lua 5.1
include(FindLua51)
# }}}

# {{{ Check if documentation can be build
if(GENERATE_MANPAGES)
    if(NOT ASCIIDOC_EXECUTABLE OR NOT XMLTO_EXECUTABLE OR (COMPRESS_MANPAGES AND NOT GZIP_EXECUTABLE))
        if(NOT ASCIIDOC_EXECUTABLE)
            SET(missing "asciidoc")
        endif()
        if(NOT XMLTO_EXECUTABLE)
            SET(missing ${missing} " xmlto")
        endif()
        if(COMPRESS_MANPAGES AND NOT GZIP_EXECUTABLE)
            SET(missing ${missing} " gzip")
        endif()

        message(STATUS "Not generating manpages. Missing: " ${missing})
        set(GENERATE_MANPAGES OFF)
    endif()
endif()

if(GENERATE_DOC)
    if(NOT LDOC_EXECUTABLE)
        message(STATUS "Not generating API documentation. Missing: ldoc")
        set(GENERATE_DOC OFF)
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
# AWESOME_REQUIRED_LDFLAGS
# AWESOME_REQUIRED_INCLUDE_DIRS

# Use pkgconfig to get most of the libraries
pkg_check_modules(AWESOME_COMMON_REQUIRED REQUIRED
    xcb>=1.6)

pkg_check_modules(AWESOME_REQUIRED REQUIRED
    glib-2.0
    gdk-pixbuf-2.0
    cairo
    x11
    x11-xcb
    xcursor
    xcb-randr
    xcb-xtest
    xcb-xinerama
    xcb-shape
    xcb-util>=0.3.8
    xcb-keysyms>=0.3.4
    xcb-icccm>=0.3.8
    xcb-image>=0.3.0
    cairo-xcb
    libstartup-notification-1.0>=0.10
    xproto>=7.0.15
    libxdg-basedir>=1.0.0)

if(NOT AWESOME_REQUIRED_FOUND OR NOT AWESOME_COMMON_REQUIRED_FOUND)
    message(FATAL_ERROR)
endif()

macro(a_find_library variable library)
    find_library(${variable} ${library})
    if(NOT ${variable})
        message(FATAL_ERROR ${library} " library not found.")
    endif()
endmacro()

# Check for backtrace_symbols()
include(CheckFunctionExists)
check_function_exists(backtrace_symbols HAS_EXECINFO)
if(NOT HAS_EXECINFO)
    find_library(LIB_EXECINFO execinfo)
    if(LIB_EXECINFO)
        set(HAS_EXECINFO 1)
        set(AWESOME_REQUIRED_LDFLAGS
            ${AWESOME_REQUIRED_LDFLAGS}
            ${LIB_EXECINFO})
    endif()
endif()
if(HAS_EXECINFO)
    message(STATUS "checking for execinfo -- found")
else()
    message(STATUS "checking for execinfo -- not found")
endif()

# __builtin_clz is available since gcc 3.4
try_compile(HAS___BUILTIN_CLZ
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/build-tests/__builtin_clz.c)
if(HAS___BUILTIN_CLZ)
    message(STATUS "checking for __builtin_clz -- yes")
else()
    message(STATUS "checking for __builtin_clz -- no")
endif()

# Error check
if(NOT LUA51_FOUND AND NOT LUA50_FOUND) # This is a workaround to a cmake bug
    message(FATAL_ERROR "lua library not found")
endif()

set(AWESOME_REQUIRED_LDFLAGS
    ${AWESOME_COMMON_REQUIRED_LDFLAGS}
    ${AWESOME_REQUIRED_LDFLAGS}
    ${LUA_LIBRARIES})

set(AWESOME_REQUIRED_INCLUDE_DIRS
    ${AWESOME_COMMON_REQUIRED_INCLUDE_DIRS}
    ${AWESOME_REQUIRED_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR})
# }}}

# {{{ Optional libraries
#
# this sets up:
# AWESOME_OPTIONAL_LDFLAGS
# AWESOME_OPTIONAL_INCLUDE_DIRS

if(WITH_DBUS)
    pkg_check_modules(DBUS dbus-1)
    if(DBUS_FOUND)
        set(AWESOME_OPTIONAL_LDFLAGS ${AWESOME_OPTIONAL_LDFLAGS} ${DBUS_LDFLAGS})
        set(AWESOME_OPTIONAL_INCLUDE_DIRS ${AWESOME_OPTIONAL_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS})
    else()
        set(WITH_DBUS OFF)
        message(STATUS "DBUS not found. Disabled.")
    endif()
endif()
# }}}

# {{{ Install path and configuration variables
#If a sysconfdir is specified, use it instead
#of the default configuration dir.
if(DEFINED SYSCONFDIR)
    set(SYSCONFDIR ${SYSCONFDIR} CACHE PATH "config directory")
else()
    set(SYSCONFDIR ${CMAKE_INSTALL_PREFIX}/etc CACHE PATH "config directory")
endif()

#If an XDG Config Dir is specificed, use it instead
#of the default XDG configuration dir.
if(DEFINED XDG_CONFIG_DIR)
    set(XDG_CONFIG_DIR ${XDG_CONFIG_DIR} CACHE PATH "xdg config directory")
else()
    set(XDG_CONFIG_DIR ${SYSCONFDIR}/xdg CACHE PATH "xdg config directory")
endif()

# setting AWESOME_DOC_PATH
if(DEFINED AWESOME_DOC_PATH)
    set(AWESOME_DOC_PATH ${AWESOME_DOC_PATH} CACHE PATH "awesome docs directory")
else()
    set(AWESOME_DOC_PATH ${CMAKE_INSTALL_PREFIX}/share/doc/${PROJECT_AWE_NAME} CACHE PATH "awesome docs directory")
endif()

# setting AWESOME_XSESSION_PATH
if(DEFINED AWESOME_XSESSION_PATH)
    set(AWESOME_XSESSION_PATH ${AWESOME_XSESSION_PATH} CACHE PATH "awesome xsessions directory")
else()
    set(AWESOME_XSESSION_PATH ${CMAKE_INSTALL_PREFIX}/share/xsessions CACHE PATH "awesome xsessions directory")
endif()

# set man path
if(DEFINED AWESOME_MAN_PATH)
   set(AWESOME_MAN_PATH ${AWESOME_MAN_PATH} CACHE PATH "awesome manpage directory")
else()
   set(AWESOME_MAN_PATH ${CMAKE_INSTALL_PREFIX}/share/man CACHE PATH "awesome manpage directory")
endif()

# Hide to avoid confusion
mark_as_advanced(CMAKE_INSTALL_CMAKE_INSTALL_PREFIX)

set(AWESOME_VERSION          ${VERSION})
set(AWESOME_COMPILE_MACHINE  ${CMAKE_SYSTEM_PROCESSOR})
set(AWESOME_COMPILE_HOSTNAME ${BUILDHOSTNAME})
set(AWESOME_COMPILE_BY       $ENV{USER})
set(AWESOME_RELEASE          ${CODENAME})
set(AWESOME_SYSCONFDIR       ${XDG_CONFIG_DIR}/${PROJECT_AWE_NAME})
set(AWESOME_DATA_PATH        ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_AWE_NAME})
set(AWESOME_LUA_LIB_PATH     ${AWESOME_DATA_PATH}/lib)
set(AWESOME_ICON_PATH        ${AWESOME_DATA_PATH}/icons)
set(AWESOME_THEMES_PATH      ${AWESOME_DATA_PATH}/themes)
# }}}

# {{{ Configure files
file(GLOB_RECURSE awesome_lua_configure_files RELATIVE ${SOURCE_DIR} ${SOURCE_DIR}/lib/*.lua.in ${SOURCE_DIR}/themes/*/*.lua.in)
set(AWESOME_CONFIGURE_FILES
    ${awesome_lua_configure_files}
    config.h.in
    config.ld.in
    awesomerc.lua.in
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

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
