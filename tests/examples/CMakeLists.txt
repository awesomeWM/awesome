# This module is designed to allow the Awesome documentation examples to be
# tested.
#
# It shims enough of the Awesome C API to allow code to be executed without an
# actual X server or running Awesome process. These tests are not genuine
# integration tests, but they are the next best thing.
#
# As secondary goals, this module also generates images of the test result
# where relevant. Those images are used by the documentation and help the
# developers track user interface regressions and glitches. Finally, it also
# helps to find broken code.

if(NOT DEFINED PROJECT_NAME)
    project(awesome-tests-examples NONE)
endif()

cmake_minimum_required(VERSION 3.0.0)

# Get and update the LUA_PATH so the scripts can be executed without awesome.
execute_process(COMMAND ${LUA_EXECUTABLE} -e "p = package.path:gsub(';', '\\\\;'); io.stdout:write(p)"
    OUTPUT_VARIABLE "LUA_PATH_")

# Allow to use the example tests by themselves.
# This was used during CI for separate coverage reporting, but is not really
# required anymore.
if(NOT SOURCE_DIR AND ${CMAKE_CURRENT_SOURCE_DIR} MATCHES "/tests/examples")
    get_filename_component(TOP_SOURCE_DIR
                           "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
else()
    set(TOP_SOURCE_DIR ${CMAKE_SOURCE_DIR})
endif()

if (DO_COVERAGE)
  execute_process(
    COMMAND env "SOURCE_DIRECTORY=${TOP_SOURCE_DIR}" ${LUA_EXECUTABLE} -e "require('luacov.runner')('${TOP_SOURCE_DIR}/.luacov')"
    RESULT_VARIABLE TEST_RESULT
    ERROR_VARIABLE  TEST_ERROR
    ERROR_STRIP_TRAILING_WHITESPACE)
  if (TEST_RESULT OR NOT TEST_ERROR STREQUAL "")
    message(${TEST_ERROR})
    message(FATAL_ERROR "Failed to run luacov.runner.")
  endif()
  set(LUA_COV_RUNNER ${LUA_EXECUTABLE} "-erequire('luacov.runner')('${TOP_SOURCE_DIR}/.luacov')")
else()
  set(LUA_COV_RUNNER ${LUA_EXECUTABLE})
endif()

if (STRICT_TESTS)
    set(IGNORE_ERRORS)
else()
    set(IGNORE_ERRORS "||" "true")
endif()

# Add the main awesome lua libraries.
set(LUA_PATH_ "\
${TOP_SOURCE_DIR}/lib/?.lua\\;\
${TOP_SOURCE_DIR}/lib/?/init.lua\\;\
${TOP_SOURCE_DIR}/lib/?\\;\
${TOP_SOURCE_DIR}/themes/?.lua\\;\
${TOP_SOURCE_DIR}/themes/?\\;\
${LUA_PATH_}")

# Add the C API shims.
set(LUA_PATH_ "\
${TOP_SOURCE_DIR}/tests/examples/shims/?.lua\\;\
${TOP_SOURCE_DIR}/tests/examples/shims/?/init.lua\\;\
${TOP_SOURCE_DIR}/tests/examples/shims/?\\;\
${LUA_PATH_}")

# $SOURCE_DIRECTORY is used by .luacov.
set(LUA_COV_RUNNER env -u LUA_PATH_5_1 -u LUA_PATH_5_2 -u LUA_PATH_5_3 "LUA_PATH=${LUA_PATH_}" "AWESOME_THEMES_PATH=${TOP_SOURCE_DIR}/themes/" "SOURCE_DIRECTORY=${TOP_SOURCE_DIR}" ${LUA_COV_RUNNER})

# The documentation images directory.
set(RAW_IMAGE_DIR "${CMAKE_BINARY_DIR}/raw_images")
set(IMAGE_DIR "${CMAKE_BINARY_DIR}/doc/images")
file(MAKE_DIRECTORY "${RAW_IMAGE_DIR}")
file(MAKE_DIRECTORY "${IMAGE_DIR}")

# Escape potentially multiline strings to be part of the API doc.
#  * add "--" in front of each lines
#  * add a custom prefix in front of each lines
#  * drop empty lines
#  * convert --DOC_NEWLINE lines into empty lines
#  * drop lines ending with "--DOC_SOMETHING", they are handled elsewhere
function(escape_string variable content escaped_content line_prefix)
    # If DOC_HIDE_ALL is present, do nothing.
    if(variable MATCHES "--DOC_HIDE_ALL")
        return()
    endif()

    string(REGEX REPLACE ";" "&#59" var_lines "${variable}")
    string(REGEX REPLACE "\n" ";" var_lines "${var_lines}")

    set(tmp_output ${content})
    set(section OFF)
    foreach (LINE ${var_lines})

        # ;; will drop the line, but " ;" will create an empty line
        string(REGEX REPLACE "--DOC_NEWLINE" " " LINE "${LINE}")

        # Check for section start marker
        if(LINE MATCHES "^.*--DOC_HIDE_START")
            set(section ON)
        endif()

        # Pick lines that are not specified to be hidden
        if((NOT LINE MATCHES "^.*--DOC_[A-Z]+") AND (NOT section))
            set(tmp_output "${tmp_output}\n${DOC_LINE_PREFIX}${line_prefix}${LINE}")
        endif()

        # If we found a start marker previously, look for an end marker.
        # Do this after picking the lines to ensure that lines with
        # `--DOC_HIDE_END` are omitted
        if(section AND (LINE MATCHES "^.*--DOC_HIDE_END"))
            set(section OFF)
        endif()
    endforeach()

    set(${escaped_content} ${tmp_output} PARENT_SCOPE)
endfunction()

# Extract lines with the --DOC_HEADER marker.
function(extract_header variable pre_output post_output)
    string(REGEX REPLACE "\n" ";" var_lines "${variable}")

    set(IS_POST 0)

    foreach (LINE ${var_lines})
        # This function doesn't escape the lines, so make sure they are
        # already comments.
        if(LINE MATCHES "^--.*--DOC_HEADER")
            # Remove the header tag
            string(REGEX REPLACE "[ ]*--DOC_HEADER" "" LINE "${LINE}")

            # ldoc is picky about what happens after the first --@, so split
            # the output on that.
            if (NOT IS_POST AND LINE MATCHES "^--[ ]*@")
                set(IS_POST 1)
            endif()

            if (IS_POST)
                set(tmp_post_output ${tmp_post_output}\n${line_prefix}${LINE})
            else()
                set(tmp_pre_output ${tmp_pre_output}\n${line_prefix}${LINE})
            endif()
        endif()
    endforeach()

    set(${post_output} "${tmp_post_output}\n${DOC_LINE_PREFIX}" PARENT_SCOPE)
    set(${pre_output} "${tmp_pre_output}\n${DOC_LINE_PREFIX}" PARENT_SCOPE)
endfunction()

# Read a code file and convert it to ldoc usage example.
#  * add "--" in front of each lines
#  * drop empty lines
#  * convert " " lines into empty lines
#  * drop lines ending with "--DOC_HIDE"
function(escape_code path escaped_content pre_header post_header)
    file(READ ${path} path)

    escape_string("${path}\n" "" escaped_code " ")

    extract_header("${path}" example_pre_header example_post_header)

    set(${escaped_content} ${escaped_code} PARENT_SCOPE)
    set(${pre_header} ${example_pre_header} PARENT_SCOPE)
    set(${post_header} ${example_post_header} PARENT_SCOPE)
endfunction()

# Find the template.lua that is closest to the given file. For example, if a
# template.lua is present in the same directory, its path will be returned. If
# one is present in the parent directory, that path is returned etc.
function(find_template result_variable file)
    get_filename_component(path "${file}" DIRECTORY)

    while(NOT EXISTS "${path}/template.lua")
        set(last_path "${path}")
        get_filename_component(path "${path}" DIRECTORY)
        if(last_path STREQUAL path)
            message(FATAL_ERROR "Failed to find template.lua for ${file}")
        endif()
    endwhile()

    set(${result_variable} "${path}/template.lua" PARENT_SCOPE)
endfunction()

# Get the namespace of a file.
function(get_namespace result_variable file)
    get_filename_component(path "${file}" DIRECTORY)
    string(LENGTH "${TOP_SOURCE_DIR}/tests/examples" prefix_length)
    string(REPLACE "/" "_" namespace "${path}")
    string(SUBSTRING "${namespace}" "${prefix_length}" -1 namespace)

    set(${result_variable} "${namespace}" PARENT_SCOPE)
endfunction()

# Execute a Lua file.
function(run_test test_path namespace escaped_content)
    find_template(template "${test_path}")

    file(READ ${test_path} tmp_content)

    # Add "--" or " * " in front of each line. This is required for method doc,
    #but not for documentation pages
    if(tmp_content MATCHES "--DOC_ASTERISK")
        set(DOC_LINE_PREFIX " * ")
    elseif(NOT tmp_content MATCHES "--DOC_NO_DASH")
        set(DOC_LINE_PREFIX "--")
    endif()

    # Do not use the @usage tag, but 4 spaces.
    if(NOT tmp_content MATCHES "--DOC_NO_USAGE")
        set(DOC_BLOCK_PREFIX "@usage")
    endif()

    # Does the text generate text output?
    if(tmp_content MATCHES "--DOC_GEN_OUTPUT")
        # The expected output is next to the .lua file in a .output.txt file
        string(REPLACE ".lua" ".output.txt" expected_output_path ${test_path})
    else()
        set(expected_output_path "/dev/null")
    endif()

    # Get the file name without the extension.
    get_filename_component(${test_path} TEST_FILE_NAME NAME)
    set(RAW_IMAGE_PATH "${RAW_IMAGE_DIR}/AUTOGEN${namespace}_${TEST_FILE_NAME}")
    set(IMAGE_PATH "${IMAGE_DIR}/AUTOGEN${namespace}_${TEST_FILE_NAME}")

    # Read the code and turn it into an usage example.
    escape_code(${test_path} TEST_CODE TEST_PRE_HEADER TEST_POST_HEADER)

    # Build the documentation.
    set(TEST_DOC_CONTENT "${TEST_PRE_HEADER}")

    # Does the test generate an output image?
    if(tmp_content MATCHES "--DOC_GEN_IMAGE")
        set(OUTPUT_RAW_IMAGE_PATH "${RAW_IMAGE_PATH}.svg")
        set(OUTPUT_IMAGE_PATH "${IMAGE_PATH}.svg")
        escape_string(
            "![Usage example](../images/AUTOGEN${namespace}_${TEST_FILE_NAME}.svg)\n"
            "${TEST_DOC_CONTENT}" TEST_DOC_CONTENT ""
        )
    else()
        set(OUTPUT_RAW_IMAGE_PATH "")
        set(OUTPUT_IMAGE_PATH "")
    endif()

    # Does the text generate text output?
    # If there is an output, assume it is relevant and add it to the
    # documentation under the image.
    if(tmp_content MATCHES "--DOC_GEN_OUTPUT")
        if(EXISTS "${expected_output_path}")
            file(READ "${expected_output_path}" expected_output)
        endif()

        if (NOT tmp_content MATCHES "--DOC_RAW_OUTPUT")
            set(TEST_DOC_CONTENT
                "${TEST_DOC_CONTENT}\n${DOC_LINE_PREFIX}\n${DOC_LINE_PREFIX}**Usage example output**:\n${DOC_LINE_PREFIX}"
            )

            # Markdown requires an empty line before and after, and 4 spaces.
            escape_string(
                "\n${expected_output}"
                "${TEST_DOC_CONTENT}" TEST_DOC_CONTENT "    "
            )

            # Add a <br/> to prevent markdown from merging the usage and output
            # into a single block.
            if(tmp_content MATCHES "--DOC_NO_USAGE")
                set(TEST_DOC_CONTENT "${TEST_DOC_CONTENT}\n${DOC_LINE_PREFIX} **Usage example:**")
            endif()
        else()
            escape_string(
                "\n${expected_output}"
                "${TEST_DOC_CONTENT}" TEST_DOC_CONTENT ""
            )
        endif()

        set(TEST_DOC_CONTENT "${TEST_DOC_CONTENT}\n${DOC_LINE_PREFIX}")
    endif()

    # If there is some @* content, append it.
    set(TEST_DOC_CONTENT "${TEST_DOC_CONTENT}${TEST_POST_HEADER}")

    # Only add it if there is something to display.
    if(NOT ${TEST_CODE} STREQUAL "\n${DOC_LINE_PREFIX}")
        escape_string(
            " ${DOC_BLOCK_PREFIX}"
            "${TEST_DOC_CONTENT}" TEST_DOC_CONTENT ""
        )
        set(TEST_DOC_CONTENT "${TEST_DOC_CONTENT}${TEST_CODE}")
    endif()

    file(RELATIVE_PATH rel_template "${TOP_SOURCE_DIR}" "${template}")
    file(RELATIVE_PATH rel_test_path "${TOP_SOURCE_DIR}" "${test_path}")

    # Add target to run this as test (via check-examples, not ignoring errors).
    set(TARGET_NAME "run-${rel_test_path}")
    STRING(REPLACE "/" "-" TARGET_NAME ${TARGET_NAME})
    set(EXAMPLE_DOC_TEST_TARGETS ${EXAMPLE_DOC_TEST_TARGETS} ${TARGET_NAME}
        PARENT_SCOPE)
    add_custom_target(${TARGET_NAME}_RAW
        COMMAND "${TOP_SOURCE_DIR}/tests/examples/runner.sh" "${expected_output_path}" ${LUA_COV_RUNNER} ${template} ${test_path} ${RAW_IMAGE_PATH}
        VERBATIM)

    add_custom_target(${TARGET_NAME}
        COMMAND "${TOP_SOURCE_DIR}/tests/examples/_postprocess.lua" ${OUTPUT_RAW_IMAGE_PATH} "${OUTPUT_IMAGE_PATH}"
        VERBATIM
        DEPENDS ${TARGET_NAME}_RAW "${TOP_SOURCE_DIR}/tests/examples/_postprocess.lua"
    )


    if(NOT ${OUTPUT_RAW_IMAGE_PATH} STREQUAL "")
        file(RELATIVE_PATH rel_output "${TOP_SOURCE_DIR}" "${OUTPUT_RAW_IMAGE_PATH}")
        if(tmp_content MATCHES "--DOC_GEN_OUTPUT")
            add_custom_command(
                COMMAND "${TOP_SOURCE_DIR}/tests/examples/runner.sh" "${expected_output_path}" ${LUA_COV_RUNNER} ${template} ${test_path} ${RAW_IMAGE_PATH} ${IGNORE_ERRORS}
                # COMMENT "Running ${rel_test_path} (via ${rel_template}, generating ${rel_output})"
                DEPENDS ${template} ${test_path}
                OUTPUT  ${OUTPUT_RAW_IMAGE_PATH}
                OUTPUT  ${expected_output_path}
                VERBATIM)
        else()
            add_custom_command(
                COMMAND "${TOP_SOURCE_DIR}/tests/examples/runner.sh" "${expected_output_path}" ${LUA_COV_RUNNER} ${template} ${test_path} ${RAW_IMAGE_PATH} ${IGNORE_ERRORS}
                # COMMENT "Running ${rel_test_path} (via ${rel_template}, generating ${rel_output})"
                DEPENDS ${template} ${test_path}
                OUTPUT  ${OUTPUT_RAW_IMAGE_PATH}
                VERBATIM)
        endif()

        add_custom_command(
            COMMAND "${TOP_SOURCE_DIR}/tests/examples/_postprocess.lua" ${OUTPUT_RAW_IMAGE_PATH} "${OUTPUT_IMAGE_PATH}"
            DEPENDS ${template} ${test_path} ${OUTPUT_RAW_IMAGE_PATH} "${TOP_SOURCE_DIR}/tests/examples/_postprocess.lua"
            OUTPUT  "${OUTPUT_IMAGE_PATH}"
            VERBATIM)

        set(EXAMPLE_DOC_GENERATED_FILES "${OUTPUT_IMAGE_PATH}" ${EXAMPLE_DOC_GENERATED_FILES} PARENT_SCOPE)

    elseif(tmp_content MATCHES "--DOC_GEN_OUTPUT")
        add_custom_command(
            COMMAND "${TOP_SOURCE_DIR}/tests/examples/runner.sh" "${expected_output_path}" ${LUA_COV_RUNNER} ${template} ${test_path} "" ${IGNORE_ERRORS}
            # COMMENT "Running ${rel_test_path} (via ${rel_template}, generating ${rel_output})"
            DEPENDS ${template} ${test_path}
            OUTPUT ${expected_output_path}
            VERBATIM)
    endif()

    # Export the outout to the parent scope.
    set(${escaped_content} "${TEST_DOC_CONTENT}" PARENT_SCOPE)
endfunction()

# Find all test files, and run them (generating custom commands for updating them).
file(GLOB_RECURSE test_files FOLLOW_SYMLINKS LIST_DIRECTORIES false
    "${TOP_SOURCE_DIR}/tests/examples/*.lua")

# Find and run all test files.
foreach(file ${test_files})
    if ((NOT "${file}" MATCHES ".*/shims/.*")
        AND (NOT "${file}" MATCHES ".*/template.lua")
        AND (NOT "${file}" MATCHES ".*/_.*[.]lua"))

        # Get the file name without the extension.
        get_filename_component(TEST_FILE_NAME ${file} NAME_WE)

        get_namespace(namespace "${file}")

        run_test("${file}" "${namespace}" ESCAPED_CODE_EXAMPLE)

        # Set the test name.
        set(TEST_NAME DOC${namespace}_${TEST_FILE_NAME}_EXAMPLE)

        # Anything called @DOC_`namespace`_EXAMPLE@
        # in the Lua or C sources will be replaced by the content of that
        # variable during the pre-processing.
        # While at it, replace \" created by CMake by ',
        # &quot; wont work in <code>.
        string(REPLACE "\"" "&#34" ${TEST_NAME} ${ESCAPED_CODE_EXAMPLE})
    endif()
endforeach()

add_custom_target(generate-examples DEPENDS ${EXAMPLE_DOC_GENERATED_FILES})
add_custom_target(check-examples DEPENDS ${EXAMPLE_DOC_TEST_TARGETS})

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80:foldmethod=marker
