# Enable some useful warnings
ADD_DEFINITIONS(
   -Wall
   -Wextra
   -Wmissing-declarations
   -Wmissing-noreturn
   -Wpointer-arith
   -Wcast-align
   -Wwrite-strings
   -Wformat-nonliteral
   -Wformat-security
   -Wswitch-enum
   -Winit-self
   -Wmissing-include-dirs
   -Wundef
   -Wmissing-format-attribute
   -Wunused
   -Wuninitialized
   -Wunused-value
   -pedantic
   -Wnonnull
   -Wsequence-point
   #-Wsystem-headers
   -Wsizeof-pointer-memaccess
   -Wvarargs
   -Wmaybe-uninitialized
   -Wunused-local-typedefs

    # because awesome has *a lot* of functions defined in macros, many are
    # not used
   -Wno-unused-function

   -Wno-attributes
   -Wno-missing-include-dirs

   # GCC will warn about everything from Clang like switch fallthrough
   -Wno-unknown-pragmas

   # Too noisy
   -Wno-pedantic
)

#Add more warnings for compilers that support it. I used this command:
#curl https://gcc.gnu.org/onlinedocs/gcc-4.9.2/gcc/Warning-Options.html | \
#grep -E "^[\t ]+<br><dt><code>-W[a-zA-Z=-]*" -o | grep -E "\-W[a-zA-Z=-]*" -o
#cat /tmp/48 /tmp/49 | sort | uniq -u

IF (CMAKE_COMPILER_IS_GNUCC)
   IF (CMAKE_C_COMPILER_VERSION VERSION_GREATER 4.9 OR CMAKE_C_COMPILER_VERSION VERSION_EQUAL 4.9)
      ADD_DEFINITIONS(
         -Wunused-but-set-parameter
         -Wno-cpp
         -Wdouble-promotion
         -Wdate-time
         -Wfloat-conversion
      )
   ENDIF()

   if (CMAKE_C_COMPILER_VERSION VERSION_GREATER 5.1 OR CMAKE_C_COMPILER_VERSION VERSION_EQUAL 5.1)
      ADD_DEFINITIONS(
         -Wbool-compare
         #-Wformat-signedness # useless
         -Wlogical-not-parentheses
         -Wnormalized
         -Wshift-count-negative
         -Wshift-count-overflow
         -Wsizeof-array-argument
      )
   ENDIF()

   IF (CMAKE_C_COMPILER_VERSION VERSION_GREATER 6.0 OR CMAKE_C_COMPILER_VERSION VERSION_EQUAL 6.0)
      ADD_DEFINITIONS(
         -Wnull-dereference
         -Wshift-negative-value
         -Wshift-overflow
         -Wduplicated-cond
         -Wmisleading-indentation
      )
   ENDIF()

   IF (CMAKE_C_COMPILER_VERSION VERSION_GREATER 7.0 OR CMAKE_C_COMPILER_VERSION VERSION_EQUAL 7.0)
      ADD_DEFINITIONS(
         -Wimplicit-fallthrough
         -Wduplicated-branches
         -Wswitch-unreachable
         -Wformat-overflow
         -Wformat-truncation
         -Wnonnull
      )
   ENDIF()

   IF (CMAKE_C_COMPILER_VERSION VERSION_GREATER 8.0 OR CMAKE_C_COMPILER_VERSION VERSION_EQUAL 8.0)
      ADD_DEFINITIONS(
        -Wmultistatement-macros
        -Wstringop-truncation
        -Wif-not-aligned
        -Wmissing-attributes
      )
   ENDIF()
ELSE()
    ADD_DEFINITIONS(-Wno-unknown-pragmas -Wno-unknown-warning-option)
ENDIF()

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
   ADD_DEFINITIONS(
      -Wno-unknown-pragmas
      -Wno-documentation-unknown-command
      -Wno-padded
      -Wno-old-style-cast
      -Wno-sign-conversion
      -Wno-exit-time-destructors
      -Wno-global-constructors
      -Wno-shorten-64-to-32
      -Wno-weak-vtables
      -Wno-float-conversion
      -Wno-reserved-id-macro
      #-Weverything

      # technically C11 features that are available in all major C99 compilers
      -Wno-typedef-redefinition
   )
endif()
