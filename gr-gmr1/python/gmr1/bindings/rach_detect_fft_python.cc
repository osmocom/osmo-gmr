/*
 * Copyright 2022 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(rach_detect_fft.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(b86e9c62afb5f0a3c02ecace39d1ef15)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/gmr1/rach_detect_fft.h>
// pydoc.h is automatically generated in the build directory
#include <rach_detect_fft_pydoc.h>

void bind_rach_detect_fft(py::module& m)
{

    using rach_detect_fft = ::gr::gmr1::rach_detect_fft;


    py::class_<rach_detect_fft,
               gr::block,
               gr::basic_block,
               std::shared_ptr<rach_detect_fft>>(m, "rach_detect_fft", D(rach_detect_fft))

        .def(py::init(&rach_detect_fft::make),
             py::arg("sample_rate"),
             py::arg("overlap_ratio"),
             py::arg("threshold"),
             py::arg("burst_length"),
             py::arg("burst_offset"),
             py::arg("peak_freq"),
             py::arg("len_tag_key"),
             D(rach_detect_fft, make))


        ;
}
