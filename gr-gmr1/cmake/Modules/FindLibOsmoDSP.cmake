if(NOT LIBOSMODSP_FOUND)
  pkg_check_modules (LIBOSMODSP_PKG libosmodsp)
  find_path(LIBOSMODSP_INCLUDE_DIRS NAMES osmocom/dsp/cxvec.h
    PATHS
    ${LIBOSMODSP_PKG_INCLUDE_DIRS}
    /usr/include
    /usr/local/include
  )

  find_library(LIBOSMODSP_LIBRARIES NAMES osmodsp
    PATHS
    ${LIBOSMODSP_PKG_LIBRARY_DIRS}
    /usr/lib
    /usr/local/lib
  )

  if(LIBOSMODSP_INCLUDE_DIRS AND LIBOSMODSP_LIBRARIES)
    set(LIBOSMODSP_FOUND TRUE CACHE INTERNAL "libosmodsp found")
    message(STATUS "Found libosmodsp: ${LIBOSMODSP_INCLUDE_DIR}, ${LIBOSMODSP_LIBRARIES}")
  else(LIBOSMODSP_INCLUDE_DIRS AND LIBOSMODSP_LIBRARIES)
    set(LIBOSMODSP_FOUND FALSE CACHE INTERNAL "libosmodsp found")
    message(STATUS "libosmodsp not found.")
  endif(LIBOSMODSP_INCLUDE_DIRS AND LIBOSMODSP_LIBRARIES)

  mark_as_advanced(LIBOSMODSP_INCLUDE_DIRS LIBOSMODSP_LIBRARIES)

endif(NOT LIBOSMODSP_FOUND)
