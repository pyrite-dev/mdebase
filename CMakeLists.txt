cmake_minimum_required(VERSION 3.25)
project(
	mdebase
)

macro(PKGCONF_ADD dest name)
	pkg_check_modules(${name} REQUIRED ${name})
	target_include_directories(
		${dest}
		PRIVATE
		${${name}_INCLUDE_DIRS}
	)
	target_link_directories(
		${dest}
		PRIVATE
		${${name}_LIBRARY_DIRS}
	)
	target_link_libraries(
		${dest}
		PRIVATE
		${${name}_LIBRARIES}
	)
endmacro()

include(GNUInstallDirs)
configure_file(include/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(mdm)
add_subdirectory(milkwm)
add_subdirectory(mpanel)
