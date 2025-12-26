include(MDEBase)
include(MDEPkgConfig)
mde_setup_project(mpanel ${CMAKE_INSTALL_BINDIR})

target_link_libraries(
	mpanel
	PRIVATE
	xemil
)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
	target_link_libraries(
		mpanel
		PRIVATE
		dl
	)
endif()

add_subdirectory(separator)
add_subdirectory(startbutton)
add_subdirectory(time)
add_subdirectory(taskbar)
add_subdirectory(launcher)

include(GNUInstallDirs)
configure_file(mpanelrc.in mpanelrc)

install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/mpanelrc
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/examples/mpanel
)
install(
	FILES mde-stripe.png
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mpanel
)
