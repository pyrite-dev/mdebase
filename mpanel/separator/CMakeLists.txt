include(MDEBase)
include(MDEPkgConfig)
mde_setup_library(SeparatorModule ${CMAKE_INSTALL_LIBDIR}/mpanel)

target_link_libraries(
	SeparatorModule
	PRIVATE
	xemil
)
