cmake_minimum_required(VERSION 3.25)
project(
	mdebase
)

add_subdirectory(mdm)
add_subdirectory(milkwm)

configure_file(include/config.h.in config.h)
