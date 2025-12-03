include(MDEBase)
include(MDEPkgConfig)
setup_project(milkwm ${CMAKE_INSTALL_BINDIR})

pkg_config_add(milkwm x11)
pkg_config_add(milkwm inih)

target_link_libraries(
	milkwm
	PRIVATE
	pthread
)
