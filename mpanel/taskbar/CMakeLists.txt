include(MDEBase)
include(MDEPkgConfig)
mde_setup_library(TaskBarModule ${CMAKE_INSTALL_LIBDIR}/mpanel)

mde_pkg_config_add(TaskBarModule libconfig)

