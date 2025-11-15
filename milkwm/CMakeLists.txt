project(
	milkwm
)

file(
	GLOB
	milkwm_SRC
	*.c
)

add_executable(
	milkwm
	${milkwm_SRC}
)

include(GNUInstallDirs)
install(
	TARGETS milkwm
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
