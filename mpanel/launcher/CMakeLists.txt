include(MDEBase)
include(MDEPkgConfig)
mde_setup_library(LauncherModule ${CMAKE_INSTALL_LIBDIR}/mpanel)

target_link_libraries(
	LauncherModule
	PRIVATE
	xemil
)
