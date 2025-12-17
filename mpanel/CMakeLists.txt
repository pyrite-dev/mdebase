include(MDEBase)
include(MDEPkgConfig)
mde_setup_project(mpanel ${CMAKE_INSTALL_BINDIR})

mde_pkg_config_add(mpanel libconfig)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
	target_link_libraries(
		mpanel
		PRIVATE
		dl
	)
endif()

add_subdirectory(separator)

include(GNUInstallDirs)
configure_file(mpanelrc.in mpanelrc)

install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/mpanelrc
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/examples/mpanel
)
