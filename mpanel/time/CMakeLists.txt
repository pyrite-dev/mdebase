include(MDEBase)
include(MDEPkgConfig)
mde_setup_library(TimeModule ${CMAKE_INSTALL_LIBDIR}/mpanel)

mde_pkg_config_add(TimeModule libconfig)

