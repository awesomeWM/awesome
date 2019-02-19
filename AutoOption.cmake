# Add a new option with default value "auto":
#   autoOption(FOOBAR "Support foobar")
#
# To check if FOOBAR support should be enabled, use:
#   if(FOOBAR)
#
# If necessary dependencies for FOOBAR are missing, use:
#   autoDisable(FOOBAR "Did not find baz")
#
# Example:
#   autoOption(FOOBAR "Support foobar")
#   if(FOOBAR)
#     Check for FOOBAR here.
#     if(NOT BAZ_FOUND)
#       autoDisable(FOOBAR "Did not find baz")
#     endif()
#   endif()

function(autoOption name description)
    set(${name} AUTO CACHE STRING "${description}")
    set_property(CACHE ${name} PROPERTY STRINGS AUTO ON OFF)

    if((NOT ${name} STREQUAL ON) AND
        (NOT ${name} STREQUAL OFF) AND
        (NOT ${name} STREQUAL AUTO))
        message(FATAL_ERROR "Value of ${name} must be one of ON/OFF/AUTO, but is ${${name}}")
    endif()
endfunction()

function(autoDisable name reason)
    message(STATUS "${reason}")
    if(${name} STREQUAL AUTO)
        message(STATUS "Disabled.")
        SET(${name} OFF PARENT_SCOPE)
    elseif(${name} STREQUAL ON)
        message(SEND_ERROR "Aborting because ${name} was set to ON.")
    else()
        message(AUTHOR_WARNING "Unexpected value for ${name}: ${${name}}.")
    endif()
endfunction()

# vim: filetype=cmake:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80:foldmethod=marker
