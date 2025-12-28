include(MDEBase)
include(MDEPkgConfig)
mde_setup_project(milkwm ${CMAKE_INSTALL_BINDIR})

mde_pkg_config_add(milkwm x11)

mde_math_add(milkwm)

target_link_libraries(
	milkwm
	PRIVATE
	xemil pthread
)

include(GNUInstallDirs)
configure_file(milkwmrc.in milkwmrc)

install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/milkwmrc
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/examples/milkwm
)
