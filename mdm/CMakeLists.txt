include(MDEBase)
include(MDEPkgConfig)
setup_project(mdm ${CMAKE_INSTALL_SBINDIR})

pkg_config_add(mdm x11)
pkg_config_add(mdm inih)

target_link_libraries(
	mdm
	PRIVATE
	pam pthread
)

include(GNUInstallDirs)
configure_file(mdmrc.in mdmrc)

install(
	FILES pam
	RENAME mdm
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/examples/pam.d
)
install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/mdmrc
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/examples/mdm
)
install(
	FILES mdm.png
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mdm
)
