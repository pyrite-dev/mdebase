project(
	mpanel
)

file(
	GLOB
	mpanel_SRC
	*.c
)

add_executable(
	mpanel
	${mpanel_SRC}
)

target_link_libraries(
	mdm
	PRIVATE
	MDELib
)

include(GNUInstallDirs)
install(
	TARGETS mpanel
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
