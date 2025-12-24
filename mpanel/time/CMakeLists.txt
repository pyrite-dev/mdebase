include(MDEBase)
include(MDEPkgConfig)
mde_setup_library(TimeModule ${CMAKE_INSTALL_LIBDIR}/mpanel)

target_link_libraries(
	TimeModule
	PRIVATE
	xemil
)
