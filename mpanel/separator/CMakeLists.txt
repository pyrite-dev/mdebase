include(MDEBase)
include(MDEPkgConfig)
mde_setup_library(SeparatorModule ${CMAKE_INSTALL_LIBDIR}/mpanel)

mde_pkg_config_add(SeparatorModule libconfig)

