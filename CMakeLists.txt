IF( NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/awesomeConfig.cmake )
        MESSAGE( FATAL "Please provide awesomeConfig.cmake" )
ENDIF( NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/awesomeConfig.cmake )

CMAKE_MINIMUM_REQUIRED( VERSION 2.4 )

INCLUDE( awesomeConfig.cmake )

INCLUDE_DIRECTORIES( ${AWE_INC_DIR} )

SET( AWE_LUA_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/lib/tabulous.lua
                    ${CMAKE_CURRENT_SOURCE_DIR}/lib/awful.lua
                    )

SET( AWE_CONF_FILES ${CMAKE_CURRENT_SOURCE_DIR}/awesomerc.lua
					)

SET( AWE_ICON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/icons )

SET( AWE_SRCS       ${CMAKE_CURRENT_SOURCE_DIR}/awesome.c
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
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/configopts.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/draw.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/markup.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/socket.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/swindow.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/util.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/version.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/xembed.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/xscreen.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/xutil.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_LAYOUT_DIR}/fibonacci.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_LAYOUT_DIR}/floating.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_LAYOUT_DIR}/magnifier.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_LAYOUT_DIR}/max.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_LAYOUT_DIR}/tile.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_WIDGET_DIR}/graph.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_WIDGET_DIR}/iconbox.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_WIDGET_DIR}/progressbar.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_WIDGET_DIR}/taglist.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_WIDGET_DIR}/tasklist.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_WIDGET_DIR}/textbox.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_WIDGET_DIR}/systray.c
                    )

SET( AWE_CLIENT_SRCS    ${CMAKE_CURRENT_SOURCE_DIR}/awesome-client.c 
                        ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/configopts.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/draw.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/markup.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/socket.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/swindow.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/util.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/version.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/xscreen.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/${AWE_COMMON_DIR}/xutil.c
                        )

SET( AWE_MAN_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/awesome.1.txt
				  ${CMAKE_CURRENT_SOURCE_DIR}/awesome-client.1.txt
				  ${CMAKE_CURRENT_SOURCE_DIR}/awesomerc.5.txt
				  )
SET( AWE_MAN1_NAME  ${CMAKE_CURRENT_BINARY_DIR}/awesome.1.gz
                    ${CMAKE_CURRENT_BINARY_DIR}/awesome-client.1.gz )
SET( AWE_MAN5_NAME  ${CMAKE_CURRENT_BINARY_DIR}/awesomerc.5.gz )                

SET( AWE_LUADOC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/apidocgen.txt )

ADD_EXECUTABLE( ${PROJECT_AWE_NAME}         ${AWE_SRCS} )
ADD_EXECUTABLE( ${PROJECT_AWECLIENT_NAME}   ${AWE_CLIENT_SRCS} )

TARGET_LINK_LIBRARIES( ${PROJECT_AWE_NAME}  ${AWE_MOD_LIBRARIES}
                                            ${LUA_LIB}
                                            ${LIB_READLINE}
                                            ${LIB_NCURSES}
                                            ${LIB_EV}
                                            )

TARGET_LINK_LIBRARIES( ${PROJECT_AWECLIENT_NAME}    ${AWE_MOD_LIBRARIES}
                                                    ${LIB_READLINE}
                                                    ${LIB_NCURSES}
                                                    )         

IF( AWESOME_GENERATE_MAN )
    # Create luadoc file
    FIND_PROGRAM( CAT_EXECUTABLE cat )
    EXECUTE_PROCESS( COMMAND ${CAT_EXECUTABLE} ${AWE_SRCS}
                     COMMAND ${LUA_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/build-utils/gendoc.lua
                     OUTPUT_FILE ${AWE_LUADOC_FILE}
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
    
    FOREACH( txtfile ${AWE_MAN_SRCS})
	STRING( REGEX REPLACE ".txt\$" ".xml" xmlfile ${txtfile} )
        STRING( REGEX REPLACE ".xml\$" ".gz" gzipman ${xmlfile} )
        STRING( REPLACE ${CMAKE_CURRENT_SOURCE_DIR} 
                        ${CMAKE_CURRENT_BINARY_DIR} gzipman ${gzipman} )
	STRING( REGEX REPLACE ".gz\$" "" manfile ${gzipman} )

        ADD_CUSTOM_COMMAND( TARGET ${PROJECT_AWECLIENT_NAME} POST_BUILD
                            COMMAND ${ASCIIDOC_EXECUTABLE} -d manpage -b docbook -o ${xmlfile} ${txtfile}
			    COMMAND ${XMLTO_EXECUTABLE} man ${xmlfile}
                            COMMAND ${GZIP_EXECUTABLE} -f ${manfile} 
                            COMMENT "Generating man pages for ${manfile}"
                            VERBATIM
                            )
    ENDFOREACH( txtfile )
ENDIF( AWESOME_GENERATE_MAN )
                                            
INSTALL(    TARGETS ${PROJECT_AWE_NAME} ${PROJECT_AWECLIENT_NAME} 
            RUNTIME DESTINATION bin
            )

INSTALL(    FILES ${AWE_LUA_FILES}
            DESTINATION ${AWESOME_LUA_LIB_PATH}
            )

INSTALL(	FILES ${AWE_CONF_FILES}
			DESTINATION ${AWESOME_CONF_PATH}
			)

INSTALL(    FILES ${AWE_MAN1_NAME}
            DESTINATION ${AWESOME_MAN1_PATH}
            ) 

INSTALL(    FILES ${AWE_MAN5_NAME}
            DESTINATION ${AWESOME_MAN5_PATH}
            )    

INSTALL(    DIRECTORY ${AWE_ICON_DIR}
            DESTINATION ${AWESOME_DATA_PATH}
			)

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
