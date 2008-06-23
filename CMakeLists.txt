CMAKE_MINIMUM_REQUIRED(VERSION 2.4.7)
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS  TRUE)

SET(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
SET(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

IF(NOT EXISTS ${SOURCE_DIR}/awesomeConfig.cmake)
    MESSAGE(FATAL_ERROR "Please provide awesomeConfig.cmake")
ENDIF()

INCLUDE(awesomeConfig.cmake)

INCLUDE_DIRECTORIES(${SOURCE_DIR}
                    ${BUILD_DIR}
                    ${AWESOME_REQUIRED_INCLUDE_DIRS}
                    ${AWESOME_OPTIONAL_INCLUDE_DIRS})

SET(AWE_LUA_FILES
    ${SOURCE_DIR}/lib/tabulous.lua
    ${SOURCE_DIR}/lib/awful.lua)

SET(AWE_CONF_FILES ${BUILD_DIR}/awesomerc.lua)

SET(AWE_ICON_DIR ${SOURCE_DIR}/icons)

SET(AWE_DOC_FILES
    ${SOURCE_DIR}/AUTHORS
    ${SOURCE_DIR}/BUGS
    ${SOURCE_DIR}/README
    ${SOURCE_DIR}/LICENSE)

SET(AWE_SRCS
    ${SOURCE_DIR}/awesome.c
    ${SOURCE_DIR}/client.c
    ${SOURCE_DIR}/cnode.c
    ${SOURCE_DIR}/dbus.c
    ${SOURCE_DIR}/event.c
    ${SOURCE_DIR}/ewmh.c
    ${SOURCE_DIR}/focus.c
    ${SOURCE_DIR}/keybinding.c
    ${SOURCE_DIR}/keygrabber.c
    ${SOURCE_DIR}/layout.c
    ${SOURCE_DIR}/lua.c
    ${SOURCE_DIR}/mouse.c
    ${SOURCE_DIR}/placement.c
    ${SOURCE_DIR}/screen.c
    ${SOURCE_DIR}/stack.c
    ${SOURCE_DIR}/statusbar.c
    ${SOURCE_DIR}/systray.c
    ${SOURCE_DIR}/tag.c
    ${SOURCE_DIR}/titlebar.c
    ${SOURCE_DIR}/widget.c
    ${SOURCE_DIR}/window.c
    ${SOURCE_DIR}/common/buffer.c
    ${SOURCE_DIR}/common/configopts.c
    ${SOURCE_DIR}/common/draw.c
    ${SOURCE_DIR}/common/markup.c
    ${SOURCE_DIR}/common/socket.c
    ${SOURCE_DIR}/common/swindow.c
    ${SOURCE_DIR}/common/util.c
    ${SOURCE_DIR}/common/version.c
    ${SOURCE_DIR}/common/xembed.c
    ${SOURCE_DIR}/common/xscreen.c
    ${SOURCE_DIR}/common/xutil.c
    ${SOURCE_DIR}/layouts/fibonacci.c
    ${SOURCE_DIR}/layouts/floating.c
    ${SOURCE_DIR}/layouts/magnifier.c
    ${SOURCE_DIR}/layouts/max.c
    ${SOURCE_DIR}/layouts/tile.c
    ${SOURCE_DIR}/widgets/graph.c
    ${SOURCE_DIR}/widgets/iconbox.c
    ${SOURCE_DIR}/widgets/progressbar.c
    ${SOURCE_DIR}/widgets/taglist.c
    ${SOURCE_DIR}/widgets/tasklist.c
    ${SOURCE_DIR}/widgets/textbox.c
    ${SOURCE_DIR}/widgets/systray.c)

SET(AWE_CLIENT_SRCS
    ${SOURCE_DIR}/awesome-client.c 
    ${SOURCE_DIR}/common/socket.c
    ${SOURCE_DIR}/common/util.c
    ${SOURCE_DIR}/common/version.c)

SET(AWE_MAN_SRCS
    ${SOURCE_DIR}/awesome.1.txt
    ${SOURCE_DIR}/awesome-client.1.txt
    ${SOURCE_DIR}/awesomerc.5.txt)

ADD_EXECUTABLE(${PROJECT_AWE_NAME} ${AWE_SRCS}
    ${BUILD_DIR}/common/tokenize.c)

ADD_EXECUTABLE(${PROJECT_AWECLIENT_NAME} ${AWE_CLIENT_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/common/tokenize.c)

TARGET_LINK_LIBRARIES(${PROJECT_AWE_NAME}
    ${AWESOME_REQUIRED_LIBRARIES}
    ${AWESOME_OPTIONAL_LIBRARIES})

TARGET_LINK_LIBRARIES(${PROJECT_AWECLIENT_NAME}
    ${LIB_READLINE}
    ${LIB_NCURSES})

# {{{ Generated sources
FILE(GLOB LAYOUTGEN_DEPS ${SOURCE_DIR}/layouts/*.h)
ADD_CUSTOM_COMMAND(
    COMMAND ${SOURCE_DIR}/build-utils/layoutgen.sh
    ARGS    > ${BUILD_DIR}/layoutgen.h
    OUTPUT  ${BUILD_DIR}/layoutgen.h
    WORKING_DIRECTORY ${SOURCE_DIR}
    DEPENDS ${LAYOUTGEN_DEPS}
    COMMENT "Generating layoutgen.h."
    VERBATIM)

ADD_CUSTOM_COMMAND(
    COMMAND ${SOURCE_DIR}/build-utils/widgetgen.sh
    ARGS    > ${BUILD_DIR}/widgetgen.h
    OUTPUT  ${BUILD_DIR}/widgetgen.h
    WORKING_DIRECTORY ${SOURCE_DIR}
    DEPENDS ${SOURCE_DIR}/widget.h
    COMMENT "Generating widgetgen.h."
    VERBATIM)

# gperf 
FILE(MAKE_DIRECTORY ${BUILD_DIR}/common)

ADD_CUSTOM_COMMAND(
    COMMAND ${SOURCE_DIR}/build-utils/gperf.sh
    ARGS    ${SOURCE_DIR}/common/tokenize.gperf
            ${BUILD_DIR}/common/tokenize.h
    OUTPUT  ${BUILD_DIR}/common/tokenize.h
    DEPENDS ${SOURCE_DIR}/common/tokenize.gperf
            ${SOURCE_DIR}/build-utils/gperf.sh
    COMMENT "Generating common/tokenize.h")

ADD_CUSTOM_COMMAND(
    COMMAND ${SOURCE_DIR}/build-utils/gperf.sh
    ARGS    ${SOURCE_DIR}/common/tokenize.gperf
            ${BUILD_DIR}/common/tokenize.c
    OUTPUT  ${BUILD_DIR}/common/tokenize.c
    DEPENDS ${SOURCE_DIR}/common/tokenize.gperf
            ${SOURCE_DIR}/build-utils/gperf.sh
    COMMENT "Generating common/tokenize.c")

ADD_CUSTOM_TARGET(generated_sources
    DEPENDS ${BUILD_DIR}/widgetgen.h
            ${BUILD_DIR}/layoutgen.h
            ${BUILD_DIR}/common/tokenize.c
            ${BUILD_DIR}/common/tokenize.h)

ADD_DEPENDENCIES(${PROJECT_AWE_NAME} generated_sources)
# }}}

# {{{ version stamp
IF(BUILD_FROM_GIT)
    ADD_CUSTOM_TARGET(version_stamp ALL
                      COMMAND ${SOURCE_DIR}/build-utils/git-version-stamp.sh
                              ${VERSION_STAMP_FILE}
                              ${BUILD_DIR}/awesome-version-internal.h
                      WORKING_DIRECTORY ${SOURCE_DIR})

    ADD_DEPENDENCIES(${PROJECT_AWE_NAME} version_stamp)
    ADD_DEPENDENCIES(${PROJECT_AWECLIENT_NAME} version_stamp)
ENDIF()
# }}}

# {{{ Manpages.
IF(GENERATE_MANPAGES)

    # {{{ Lua API documentation.
    SET(LUADOC_FILE ${BUILD_DIR}/apidocgen.txt)

    MACRO(a_file_match infile regex result_var)
        IF(CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION LESS 6)
            # cmake < 2.6 doesn't know FILE(STRINGS ...)
            EXECUTE_PROCESS(COMMAND ${GREP_EXECUTABLE} -l ${regex} ${infile}
                            RESULT_VARIABLE exit_code
                            OUTPUT_QUIET)
            IF(exit_code EQUAL 0)
                SET(${result_var} TRUE)
            ELSE()
                SET(${result_var} FALSE)
            ENDIF()
        ELSE()
            FILE(STRINGS ${infile} match REGEX ${regex} LIMIT_COUNT 1)
            IF(match)
                SET(${result_var} TRUE)
            ELSE()
                SET(${result_var} FALSE)
            ENDIF()
        ENDIF()
    ENDMACRO()

    # find .c files exporting lua functions
    FOREACH(cfile ${AWE_SRCS})
        a_file_match(${cfile} "const struct luaL_reg" result)
        IF(result)
            SET(LUA_APIDOC_SRCS ${LUA_APIDOC_SRCS} ${cfile})
        ENDIF()
    ENDFOREACH()

    # luadoc target
    ADD_CUSTOM_COMMAND(COMMAND ${CAT_EXECUTABLE} ${LUA_APIDOC_SRCS}
                       | ${LUA_EXECUTABLE} ${SOURCE_DIR}/build-utils/gendoc.lua
                       > ${LUADOC_FILE}
                       OUTPUT ${LUADOC_FILE}
                       WORKING_DIRECTORY ${SOURCE_DIR}
                       DEPENDS ${LUA_APIDOC_SRCS}
                       COMMENT "Generating Lua API documentation."
                       VERBATIM)

    ADD_CUSTOM_TARGET(luadoc DEPENDS ${LUADOC_FILE})
    # }}}

    FOREACH(txtfile ${AWE_MAN_SRCS})
        STRING(REGEX REPLACE ".txt\$" ".xml" xmlfile ${txtfile})
        STRING(REPLACE ${SOURCE_DIR}
                       ${BUILD_DIR} xmlfile ${xmlfile})
        STRING(REGEX REPLACE ".xml\$" ".gz" gzfile ${xmlfile})
        STRING(REGEX REPLACE ".gz\$" "" manfile ${gzfile})

        # TODO: This is still not 100% right. Actually only
        #       awesomerc.5.xml should depend on ${LUADOC_FILE}.
        ADD_CUSTOM_COMMAND(COMMAND ${CAT_EXECUTABLE} ${txtfile}
                           | ${ASCIIDOC_EXECUTABLE} -d manpage -b docbook -o ${xmlfile} -
                           WORKING_DIRECTORY ${BUILD_DIR}
                           OUTPUT ${xmlfile}
                           DEPENDS ${txtfile} ${LUADOC_FILE}
                           VERBATIM)
        ADD_CUSTOM_COMMAND(COMMAND ${XMLTO_EXECUTABLE} man ${xmlfile}
                           OUTPUT ${manfile}
                           DEPENDS ${xmlfile})
        ADD_CUSTOM_COMMAND(COMMAND ${CAT_EXECUTABLE} ${manfile}
                           | ${GZIP_EXECUTABLE}
                           > ${gzfile}
                           OUTPUT ${gzfile}
                           DEPENDS ${manfile}
                           VERBATIM)

        SET(MAN_FILES ${MAN_FILES} ${gzfile})

        # figure out in what section to install to from filename
        STRING(REGEX REPLACE "^.*\\.([0-9])\\.gz\$" "\\1" section ${gzfile})
        SET(AWE_MAN${section}_FILES ${AWE_MAN${section}_FILES} ${gzfile})
    ENDFOREACH()

    ADD_CUSTOM_TARGET(man ALL DEPENDS ${MAN_FILES})
ENDIF()
# }}}

# {{{ Doxygen
IF(DOXYGEN_EXECUTABLE)
    ADD_CUSTOM_TARGET(doc
                      COMMAND ${DOXYGEN_EXECUTABLE} awesome.doxygen
                      WORKING_DIRECTORY ${BUILD_DIR})
ENDIF()
# }}}

# {{{ Installation.
INSTALL(TARGETS ${PROJECT_AWE_NAME} ${PROJECT_AWECLIENT_NAME} RUNTIME DESTINATION bin)
INSTALL(FILES ${AWE_LUA_FILES} DESTINATION ${AWESOME_REL_LUA_LIB_PATH})
INSTALL(FILES ${AWE_CONF_FILES} DESTINATION ${AWESOME_REL_CONF_PATH})
IF(GENERATE_MANPAGES)
    INSTALL(FILES ${AWE_MAN1_FILES} DESTINATION ${AWESOME_MAN1_PATH})
    INSTALL(FILES ${AWE_MAN5_FILES} DESTINATION ${AWESOME_MAN5_PATH})
ENDIF()
INSTALL(DIRECTORY ${AWE_ICON_DIR} DESTINATION ${AWESOME_REL_ICON_PATH})
INSTALL(FILES ${AWE_DOC_FILES} DESTINATION ${AWESOME_REL_DOC_PATH})
# }}}

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
