find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_GMR1 gnuradio-gmr1)

FIND_PATH(
    GR_GMR1_INCLUDE_DIRS
    NAMES gnuradio/gmr1/api.h
    HINTS $ENV{GMR1_DIR}/include
        ${PC_GMR1_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_GMR1_LIBRARIES
    NAMES gnuradio-gmr1
    HINTS $ENV{GMR1_DIR}/lib
        ${PC_GMR1_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-gmr1Target.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_GMR1 DEFAULT_MSG GR_GMR1_LIBRARIES GR_GMR1_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_GMR1_LIBRARIES GR_GMR1_INCLUDE_DIRS)
