SET( PROJECT_AWE_NAME awesome )
SET( PROJECT_AWECLIENT_NAME awesome-client )
SET( VERSION 3 )

PROJECT( ${PROJECT_AWE_NAME} )

SET( CMAKE_BUILD_TYPE RELEASE )

# If this is a git repository...
IF( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git/HEAD )
	# ...update version
	FIND_PROGRAM(GIT_EXECUTABLE git)
	IF( GIT_EXECUTABLE )
		EXECUTE_PROCESS( COMMAND ${GIT_EXECUTABLE} describe
						 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
						 OUTPUT_VARIABLE VERSION
						 OUTPUT_STRIP_TRAILING_WHITESPACE )
	ENDIF( GIT_EXECUTABLE )
ENDIF( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git/HEAD )


# Check for doxygen
INCLUDE( FindDoxygen )
INCLUDE( FindPkgConfig )
INCLUDE( UsePkgConfig )

SET( AWE_COMMON_DIR common )
SET( AWE_LAYOUT_DIR layouts )
SET( AWE_WIDGET_DIR widgets )

MESSAGE( "Checking for REQUIRED modules: " )

# Use pkgconfig to get most of the libraries
pkg_check_modules( AWE_MOD REQUIRED     glib-2.0
                                        cairo
                                        pango
                                        gtk+-2.0>=2.2
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
                                        cairo-xcb
                                        dbus-1
                                        imlib2 
                                        )

# Check for readline and ncurse                                    
FIND_LIBRARY( LIB_READLINE readline )
FIND_LIBRARY( LIB_NCURSES ncurses )

# Check for lobev
FIND_LIBRARY( LIB_EV ev )

# Check for lua5.1
FIND_PATH(LUA_INC_DIR lua.h
    /usr/include
    /usr/include/lua5.1
    /usr/local/include/lua5.1
    ../libs/lua-5.1.3/src)

FIND_LIBRARY(LUA_LIB NAMES lua5.1 lua
    /usr/lib
    /usr/lib/lua
    /usr/local/lib
    ../libs/lua-5.1.3/lib)

FIND_PROGRAM( LUA_EXECUTABLE lua )

# Error check
IF( NOT LIB_READLINE )
    MESSAGE( FATAL_ERROR "readline library not found" )
ENDIF( NOT LIB_READLINE )

IF( NOT LIB_NCURSES )
    MESSAGE( FATAL_ERROR "ncurse library not found" )
ENDIF( NOT LIB_NCURSES )        

IF( NOT LIB_EV )
    MESSAGE( FATAL_ERROR "libev not found" )
ENDIF( NOT LIB_EV )

IF( NOT LUA_LIB )
    MESSAGE( FATAL_ERROR "lua library not found" )
ELSE( LUA_LIB AND LUA_INC_DIR )
    MESSAGE( "lua 5.1 found: " ${LUA_LIB} )    
ENDIF( NOT LUA_LIB )    

# Add awesome defines
ADD_DEFINITIONS( -DWITH_DBUS
                 -DWITH_IMLIB
                 )

IF ( DOXYGEN_EXECUTABLE )
	ADD_CUSTOM_TARGET( doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/awesome.doxygen )
ENDIF ( DOXYGEN_EXECUTABLE )

# Check for programs needed for man pages
FIND_PROGRAM( ASCIIDOC_EXECUTABLE asciidoc )
FIND_PROGRAM( XMLTO_EXECUTABLE xmlto )
FIND_PROGRAM( GZIP_EXECUTABLE gzip )

IF( ASCIIDOC_EXECUTABLE AND XMLTO_EXECUTABLE AND GZIP_EXECUTABLE )
		SET( AWESOME_GENERATE_MAN TRUE )
ENDIF( ASCIIDOC_EXECUTABLE AND XMLTO_EXECUTABLE AND GZIP_EXECUTABLE )

# Set awesome informations and path
SET( AWESOME_VERSION_INTERNAL   devel )
SET( AWESOME_COMPILE_MACHINE    ${CMAKE_SYSTEM_PROCESSOR} )
SET( AWESOME_COMPILE_HOSTNAME   $ENV{HOSTNAME} )
SET( AWESOME_COMPILE_BY         $ENV{USER} )
SET( AWESOME_RELEASE            ${VERSION} )
SET( AWESOME_ETC                ${CMAKE_INSTALL_PREFIX}/etc )
SET( AWESOME_SHARE              ${CMAKE_INSTALL_PREFIX}/share )
SET( AWESOME_DATA_PATH          ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_AWE_NAME} )
SET( AWESOME_LUA_LIB_PATH       ${AWESOME_DATA_PATH}/lib )
SET( AWESOME_ICON_PATH          ${AWESOME_DATA_PATH}/icons )
SET( AWESOME_CONF_PATH          ${AWESOME_ETC}/${PROJECT_AWE_NAME} )
SET( AWESOME_MAN1_PATH          ${AWESOME_SHARE}/man/man1 )
SET( AWESOME_MAN5_PATH          ${AWESOME_SHARE}/man/man5 )

# Configure awesome config.h from template
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in 
                ${CMAKE_CURRENT_SOURCE_DIR}/config.h 
                ESCAPE_QUOTE 
                @ONLY 
                )

# Confiure awesomerc.lua.in
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/awesomerc.lua.in
				${CMAKE_CURRENT_SOURCE_DIR}/awesomerc.lua
				ESCAPE_QUOTE
				@ONLY
				)

# Configure awesome awesome-version-internal.h from template            
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/awesome-version-internal.h.in 
                ${CMAKE_CURRENT_SOURCE_DIR}/awesome-version-internal.h 
                ESCAPE_QUOTE 
                @ONLY 
                )

# Configure awesome.doxygen
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/awesome.doxygen.in
                ${CMAKE_CURRENT_SOURCE_DIR}/awesome.doxygen
                ESCAPE_QUOTE
                @ONLY
                )

# Execute some header generator
EXECUTE_PROCESS(    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-utils/layoutgen.sh 
                    OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/layoutgen.h 
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
                    )

EXECUTE_PROCESS(    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-utils/widgetgen.sh 
                    OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/widgetgen.h 
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
                    )

# Set the awesome include dir
SET( AWE_INC_DIR    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${AWE_MOD_INCLUDE_DIRS}
                    ${LUA_INC_DIR}
                    )

