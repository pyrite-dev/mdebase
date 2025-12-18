include(MDEBase)
include(MDEPkgConfig)
mde_setup_library(StartButtonModule ${CMAKE_INSTALL_LIBDIR}/mpanel)

mde_pkg_config_add(StartButtonModule libconfig)

