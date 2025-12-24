include(MDEBase)
include(MDEPkgConfig)
mde_setup_library(TaskBarModule ${CMAKE_INSTALL_LIBDIR}/mpanel)

target_link_libraries(
	TaskBarModule
	PRIVATE
	xemil
)
