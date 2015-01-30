# Copyright 2011 Free Software Foundation, Inc.
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

list(APPEND gmr1_sources
	../../src/sdr/dkab.c
	../../src/sdr/fcch.c
	../../src/sdr/nb.c
	../../src/sdr/pi4cxpsk.c
	../../src/l1/a5.c
	../../src/l1/bcch.c
	../../src/l1/ccch.c
	../../src/l1/conv.c
	../../src/l1/crc.c
	../../src/l1/facch3.c
	../../src/l1/facch9.c
	../../src/l1/interleave.c
	../../src/l1/punct.c
	../../src/l1/rach.c
	../../src/l1/scramb.c
	../../src/l1/tch3.c
	../../src/l1/tch9.c
	../../src/gsmtap.c
	burst_to_tagged_stream_impl.cc
	gsmtap_sink_impl.cc
	rach_demod_impl.cc
	rach_detect_fft_impl.cc
)

add_library(gnuradio-gmr1 SHARED ${gmr1_sources})
target_link_libraries(gnuradio-gmr1
	${Boost_LIBRARIES}
	${FFTW3F_LIBRARIES}
	${LIBOSMOCORE_LIBRARIES}
	${LIBOSMODSP_LIBRARIES}
	${GNURADIO_ALL_LIBRARIES}
)
set_target_properties(gnuradio-gmr1 PROPERTIES DEFINE_SYMBOL "gnuradio_gmr1_EXPORTS")

########################################################################
# Install built library files
########################################################################
install(TARGETS gnuradio-gmr1
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Build and register unit test
########################################################################
find_package(Boost COMPONENTS unit_test_framework)

include(GrTest)
set(GR_TEST_TARGET_DEPS gnuradio-gmr1)
#turn each test cpp file into an executable with an int main() function
add_definitions(-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MAIN)
