project(startmde)

include(GNUInstallDirs)
configure_file(startmde.in startmde)

install(
	PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/startmde
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)
