# This file gather the different default example screenshots and create an HTML
# table. Those tables are re-used in the official documentation.

# Ldoc wont parse the HTML content and discount tables are disabled, so here is
# some raw HTML
function(add_to_table name namespace group current_table new_table)
    set(URI_PATH "${group}")

    if(${URI_PATH} STREQUAL "awidget")
        set(URI_PATH "widget")
    elseif(NOT ${URI_PATH} STREQUAL "widget")
        set(URI_PATH "widget_${group}")
    endif()

    set(${new_table} "${current_table}\n\
<tr>\n\
 <td>
  <a href='../${URI_PATH}s/${namespace}${name}.html'>${namespace}${name}</a>
 </td>\n\
 <td><img src='../images/AUTOGEN_wibox_${group}_defaults_${name}.svg' /></td>\n\
</tr>\n\
" PARENT_SCOPE)
endfunction()

# Use the generated "defaults" images to build a list
function(generate_widget_list name)
    file(GLOB ex_files RELATIVE "${SOURCE_DIR}/tests/examples/wibox/${name}/defaults"
    "${SOURCE_DIR}/tests/examples/wibox/${name}/defaults/*")
    list(SORT ex_files)

    # Generate namespace prefix
    set(NAMESPACE_PREFIX "wibox.${name}.")
    if(${name} STREQUAL "awidget")
        set(NAMESPACE_PREFIX "awful.widget.")
    endif()

    # Add the table header
    set(MY_LIST "<table class='widget_list' border=1>\n\
 <tr style='font-weight: bold;'>\n\
  <th align='center'>Name</th>\n\
  <th align='center'>Example</th>\n\
 </tr>"
    )

    # Loop all examples (and assume an image exist for them)
    foreach(ex_file_name ${ex_files})
        string(REGEX REPLACE "\\.lua" "" ex_file_name ${ex_file_name})

        if(NOT ${ex_file_name} STREQUAL "template")
            add_to_table(${ex_file_name} "${NAMESPACE_PREFIX}" "${name}" "${MY_LIST}" MY_LIST)
        endif()
    endforeach()

    # Add the table footer
    set(MY_LIST "${MY_LIST}</table>\n\n")

    set(DOC_${name}_WIDGET_LIST ${MY_LIST} PARENT_SCOPE)
endfunction()

generate_widget_list( "container" )
generate_widget_list( "layout"    )
generate_widget_list( "widget"    )
generate_widget_list( "awidget"   )

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80:foldmethod=marker
