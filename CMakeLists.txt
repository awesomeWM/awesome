CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS  TRUE)

IF(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/awesomeConfig.cmake)
    MESSAGE(FATAL_ERROR "Please provide awesomeConfig.cmake")
ENDIF()

INCLUDE(awesomeConfig.cmake)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}
                    ${AWESOME_REQUIRED_INCLUDE_DIRS}
                    ${AWESOME_OPTIONAL_INCLUDE_DIRS})

SET(AWE_LUA_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/tabulous.lua
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/awful.lua)

SET(AWE_CONF_FILES ${CMAKE_CURRENT_SOURCE_DIR}/awesomerc.lua)

SET(AWE_ICON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/icons)

SET(AWE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/awesome.c
    ${CMAKE_CURRENT_SOURCE_DIR}/client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cnode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dbus.c
    ${CMAKE_CURRENT_SOURCE_DIR}/event.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ewmh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/focus.c
    ${CMAKE_CURRENT_SOURCE_DIR}/keybinding.c
    ${CMAKE_CURRENT_SOURCE_DIR}/keygrabber.c
    ${CMAKE_CURRENT_SOURCE_DIR}/layout.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lua.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mouse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/placement.c
    ${CMAKE_CURRENT_SOURCE_DIR}/screen.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stack.c
    ${CMAKE_CURRENT_SOURCE_DIR}/statusbar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/systray.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tag.c
    ${CMAKE_CURRENT_SOURCE_DIR}/titlebar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/widget.c
    ${CMAKE_CURRENT_SOURCE_DIR}/window.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/configopts.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/draw.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/markup.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/socket.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/swindow.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/version.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/xembed.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/xscreen.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/xutil.c
    ${CMAKE_CURRENT_SOURCE_DIR}/layouts/fibonacci.c
    ${CMAKE_CURRENT_SOURCE_DIR}/layouts/floating.c
    ${CMAKE_CURRENT_SOURCE_DIR}/layouts/magnifier.c
    ${CMAKE_CURRENT_SOURCE_DIR}/layouts/max.c
    ${CMAKE_CURRENT_SOURCE_DIR}/layouts/tile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/graph.c
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/iconbox.c
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/progressbar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/taglist.c
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/tasklist.c
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/textbox.c
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets/systray.c)

SET(AWE_CLIENT_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/awesome-client.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/common/socket.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/version.c)

SET(AWE_MAN_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/awesome.1.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/awesome-client.1.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/awesomerc.5.txt)

SET(AWE_MAN1_NAME
    ${CMAKE_CURRENT_BINARY_DIR}/awesome.1.gz
    ${CMAKE_CURRENT_BINARY_DIR}/awesome-client.1.gz)

SET(AWE_MAN5_NAME ${CMAKE_CURRENT_BINARY_DIR}/awesomerc.5.gz)

SET(AWE_LUADOC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/apidocgen.txt)


ADD_EXECUTABLE(${PROJECT_AWE_NAME} ${AWE_SRCS})
ADD_EXECUTABLE(${PROJECT_AWECLIENT_NAME} ${AWE_CLIENT_SRCS})

TARGET_LINK_LIBRARIES(${PROJECT_AWE_NAME}
    ${AWESOME_REQUIRED_LIBRARIES}
    ${AWESOME_OPTIONAL_LIBRARIES})

TARGET_LINK_LIBRARIES(${PROJECT_AWECLIENT_NAME}
    ${LIB_READLINE}
    ${LIB_NCURSES})

# {{{ Generated headers
FILE(GLOB LAYOUTGEN_DEPS ${CMAKE_CURRENT_SOURCE_DIR}/layouts/*.h)
ADD_CUSTOM_COMMAND(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-utils/layoutgen.sh
                   ARGS > layoutgen.h
                   OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/layoutgen.h
                   WORKING_DIRECTORY ${CURRENT_SOURCE_DIR}
                   DEPENDS ${LAYOUTGEN_DEPS}
                   COMMENT "Generating layoutgen.h."
                   VERBATIM)

ADD_CUSTOM_COMMAND(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-utils/widgetgen.sh
                   ARGS > widgetgen.h
                   OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/widgetgen.h
                   WORKING_DIRECTORY ${CURRENT_SOURCE_DIR}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/widget.h
                   COMMENT "Generating widgetgen.h."
                   VERBATIM)

ADD_CUSTOM_TARGET(generated_headers
                  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/widgetgen.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/layoutgen.h)

ADD_DEPENDENCIES(${PROJECT_AWE_NAME} generated_headers)
# }}}

IF(GENERATE_MANPAGES)
    # Create luadoc file
    EXECUTE_PROCESS(COMMAND ${CAT_EXECUTABLE} ${AWE_SRCS}
                    COMMAND ${LUA_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/build-utils/gendoc.lua
                    OUTPUT_FILE ${AWE_LUADOC_FILE}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    FOREACH(txtfile ${AWE_MAN_SRCS})
        STRING(REGEX REPLACE ".txt\$" ".xml" xmlfile ${txtfile})
        STRING(REGEX REPLACE ".xml\$" ".gz" gzipman ${xmlfile})
        STRING(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}
                       ${CMAKE_CURRENT_BINARY_DIR} gzipman ${gzipman})
        STRING(REGEX REPLACE ".gz\$" "" manfile ${gzipman})

        ADD_CUSTOM_COMMAND(TARGET ${PROJECT_AWECLIENT_NAME} POST_BUILD
                           COMMAND ${ASCIIDOC_EXECUTABLE} -d manpage -b docbook -o ${xmlfile} ${txtfile}
                           COMMAND ${XMLTO_EXECUTABLE} man ${xmlfile}
                           COMMAND ${GZIP_EXECUTABLE} -f ${manfile}
                           COMMENT "Generating manpage for ${manfile}")
    ENDFOREACH()
ENDIF()

IF(DOXYGEN_EXECUTABLE)
    ADD_CUSTOM_TARGET(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/awesome.doxygen)
ENDIF()

INSTALL(TARGETS ${PROJECT_AWE_NAME} ${PROJECT_AWECLIENT_NAME} RUNTIME DESTINATION bin)
INSTALL(FILES ${AWE_LUA_FILES} DESTINATION ${AWESOME_REL_LUA_LIB_PATH})
INSTALL(FILES ${AWE_CONF_FILES} DESTINATION ${AWESOME_REL_CONF_PATH})
IF(GENERATE_MANPAGES)
    INSTALL(FILES ${AWE_MAN1_NAME} DESTINATION ${AWESOME_MAN1_PATH})
    INSTALL(FILES ${AWE_MAN5_NAME} DESTINATION ${AWESOME_MAN5_PATH})
ENDIF()
INSTALL(DIRECTORY ${AWE_ICON_DIR} DESTINATION ${AWESOME_REL_ICON_PATH})

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
