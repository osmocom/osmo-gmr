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
/* BINDTOOL_HEADER_FILE(rach_demod.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(2f4f24b56889fe37e3d14bc9911b18ab)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/gmr1/rach_demod.h>
// pydoc.h is automatically generated in the build directory
#include <rach_demod_pydoc.h>

void bind_rach_demod(py::module& m)
{

    using rach_demod = ::gr::gmr1::rach_demod;


    py::class_<rach_demod,
               gr::tagged_stream_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<rach_demod>>(m, "rach_demod", D(rach_demod))

        .def(py::init(&rach_demod::make),
             py::arg("sps"),
             py::arg("etoa"),
             py::arg("len_tag_key"),
             D(rach_demod, make))


        .def("sps", &rach_demod::sps, D(rach_demod, sps))


        .def("etoa", &rach_demod::etoa, D(rach_demod, etoa))

        ;
}