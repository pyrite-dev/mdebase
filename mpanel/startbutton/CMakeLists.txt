include(MDEBase)
include(MDEPkgConfig)
mde_setup_library(StartButtonModule ${CMAKE_INSTALL_LIBDIR}/mpanel)

target_link_libraries(
	StartButtonModule
	PRIVATE
	xemil
)

mde_pkg_config_add(StartButtonModule inih)

mde_math_add(StartButtonModule)
