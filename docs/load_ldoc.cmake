
foreach(path ${SOURCE_DIR}/docs/common/)
    # Get the documentation file list
    file(GLOB doc_files RELATIVE "${path}" "${path}/*.ldoc")

    foreach(doc_file_name ${doc_files})
        # Apply all @DOC_foo@ in the file. Note that including a .ldoc from a
        # .ldoc isn't currently supported (but could be).
        configure_file("${path}/${doc_file_name}"
            ${BUILD_DIR}/docs/common/${file}
            @ONLY
        )

        # Read the file
        file(READ "${BUILD_DIR}/docs/common/${doc_file_name}" doc_file_content)

        # Remove the file extension
        string(REGEX REPLACE "\\.ldoc" "" DOC_FILE_NAME ${doc_file_name})

        # There is a trailing \n, remove it or it cannot be included in existing blocks
        string(REGEX REPLACE "\n$" "" doc_file_content "${doc_file_content}")

        # Create a new variable usable from lua files
        set(DOC_${DOC_FILE_NAME}_COMMON "${doc_file_content}")
    endforeach()
endforeach()

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80:foldmethod=marker
