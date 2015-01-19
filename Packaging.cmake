if(NOT VERSION)
	set(VERSION "unknown")
endif()

string(REGEX REPLACE "^v?([0-9.]+)-?(.*)$"
	"\\1;\\2" version_result ${VERSION})

list(LENGTH version_result version_result_list_length)

if(version_result_list_length EQUAL 2)
	list(GET version_result 0 version_num)
	list(GET version_result 1 version_gitstamp)
else(version_result_list_length EQUAL 2)
	message("Unable to deduce a meaningful version number. \
Set OVERRIDE_VERSION when you run CMake (cmake .. -DOVERRIDE_VERSION=3.14.159), or \
just build from a git repository.")
	set(version_num "0.0.0")
	set(version_gitstamp "")
endif(version_result_list_length EQUAL 2)

if(version_gitstamp)
	set(version_gitsuffix "~git${version_gitstamp}")
else(version_gitstamp)
	set(version_gitsuffix "")
endif(version_gitstamp)

string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)"
	"\\1;\\2;\\3" version_num_split ${version_num})

list(APPEND version_num_split 0 0 0) #ensure the list(GET )) commands below never fail

list(GET version_num_split 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET version_num_split 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET version_num_split 2 CPACK_PACKAGE_VERSION_PATCH)

set(version_num "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

message(STATUS "Package version will be set to ${version_num}${version_gitsuffix}.")

set(CPACK_PACKAGE_VERSION "${version_num}${version_gitsuffix}")

if(NOT CPACK_GENERATOR)
	find_program(rpmbuild_path "rpmbuild")
	if(rpmbuild_path)
		message(STATUS "rpmbuild found, enabling RPM for the 'package' target")
		list(APPEND CPACK_GENERATOR RPM)
	endif(rpmbuild_path)

	find_program(dpkg_path "dpkg")
	if (dpkg_path)
		message(STATUS "dpkg found, enabling DEB for the 'package' target")
		list(APPEND CPACK_GENERATOR DEB)
	endif(dpkg_path)
endif(NOT CPACK_GENERATOR)

set(CPACK_PACKAGE_NAME "awesome")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "devnull@example.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A tiling window manager")

if(CPACK_GENERATOR)
	include(CPack)
endif()
