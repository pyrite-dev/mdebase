project(
	mdm
)

find_package(PkgConfig)

file(
	GLOB
	mdm_SRC
	*.c
)

add_executable(
	mdm
	${mdm_SRC}
)

PKGCONF_ADD(mdm x11)
PKGCONF_ADD(mdm inih)

target_link_libraries(
	mdm
	PRIVATE
	MDELib Mw
)


include(GNUInstallDirs)
configure_file(mdmrc.in mdmrc)

install(
	TARGETS mdm
	RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
)
install(
	FILES pam
	RENAME mdm
	DESTINATION /etc/pam.d
)
install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/mdmrc
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/examples/mdm
)
install(
	FILES mdm.png
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mdm
)
