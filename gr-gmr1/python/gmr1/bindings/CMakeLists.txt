# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT gmr1_sources)
    MESSAGE(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT gmr1_sources)

########################################################################
# Check for pygccxml
########################################################################
GR_PYTHON_CHECK_MODULE_RAW(
    "pygccxml"
    "import pygccxml"
    PYGCCXML_FOUND
    )

include(GrPybind)

########################################################################
# Python Bindings
########################################################################

list(APPEND gmr1_python_files
        burst_to_tagged_stream_python.cc
        gsmtap_sink_python.cc
        rach_demod_python.cc
        rach_detect_fft_python.cc
        rach_file_sink_python.cc
    python_bindings.cc)

GR_PYBIND_MAKE_OOT(gmr1
   ../../..
   gr::gmr1
   "${gmr1_python_files}")

# copy bindings extension for use in QA test module
add_custom_command(TARGET gmr1_python POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:gmr1_python>
    ${PROJECT_BINARY_DIR}/test_modules/gnuradio/gmr1/
)

install(TARGETS gmr1_python DESTINATION ${GR_PYTHON_DIR}/gnuradio/gmr1 COMPONENT pythonapi)
