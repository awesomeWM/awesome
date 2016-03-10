
# Get and update the LUA_PATH so the scripts can be executed without Awesome
execute_process(COMMAND lua -e print\(package.path\) OUTPUT_VARIABLE LUA_PATH_)
set(ENV{LUA_PATH} "${SOURCE_DIR}/lib/?;${SOURCE_DIR}/lib/?.lua;${LUA_PATH_}")

file(MAKE_DIRECTORY "${BUILD_DIR}/doc/images")

#
# Shape API
#
foreach (SHAPE_NAME "circle" "arrow" "rounded_rect" "hexagon" "infobubble"
    "powerline" "isosceles_triangle" "cross" "octogon" "parallelogram"
    "losange" "partially_rounded_rect" "radial_progress" "rounded_bar"
    "rectangle" "rectangular_tag")
    set(SHAPE_FILE "${SOURCE_DIR}/tests/shape/${SHAPE_NAME}.lua")
    set(SHAPE_SVG  "${BUILD_DIR}/doc/images/shape_${SHAPE_NAME}.svg")

    # Generate some SVG for the documentation and load the examples for the doc
    execute_process(
        COMMAND lua ${SOURCE_DIR}/tests/shape/test-shape.lua ${SHAPE_FILE} ${SHAPE_SVG}
        OUTPUT_VARIABLE SHAPE_OUTPUT
        ERROR_VARIABLE  SHAPE_ERROR
    )

    if (NOT SHAPE_ERROR STREQUAL "")
        message(${SHAPE_ERROR})
        message(FATAL_ERROR ${SHAPE_NAME} " SVG generation failed, bye")
    endif()

    # Set the SVG paths for the doc
    set(SHAPE_${SHAPE_NAME}_SVG "../images/shape_${SHAPE_NAME}.svg")

    # Use the .lua as code example
    file(READ ${SHAPE_FILE} SHAPE_EXAMPLE)
    STRING(REGEX REPLACE "\n" ";" SHAPE_EXAMPLE "${SHAPE_EXAMPLE}")
    SET(SHAPE_COMMENTED
        "![Shape example](../images/shape_${SHAPE_NAME}.svg)\n--\n-- @usage"
    )
    foreach (EXAMPLE_FILE ${SHAPE_EXAMPLE})
        if(NOT EXAMPLE_FILE MATCHES "^.+--DOC_HIDE$")
            SET(SHAPE_COMMENTED ${SHAPE_COMMENTED}\n--${EXAMPLE_FILE})
        endif()
    endforeach()

    set(SHAPE_${SHAPE_NAME}_EXAMPLE ${SHAPE_COMMENTED})

endforeach()
