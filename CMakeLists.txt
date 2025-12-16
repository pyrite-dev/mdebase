cmake_minimum_required(VERSION 3.25)
project(
	mdebase
)

include(GNUInstallDirs)
configure_file(wizard.sh.in wizard.sh)

add_subdirectory(mdm)
add_subdirectory(milkwm)
add_subdirectory(mpanel)
add_subdirectory(startmde)
