include(MDEBase)
setup_project(mpanel ${CMAKE_INSTALL_BINDIR})

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
	target_link_libraries(
		mpanel
		PRIVATE
		dl
	)
endif()
