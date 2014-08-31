/* -*- c++ -*- */
/*
 * Copyright 2014 Sylvain Munaut <tnt@246tNt.com>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_GR_GMR1_CXVEC_COMPAT_H
#define INCLUDED_GR_GMR1_CXVEC_COMPAT_H

#include <complex.h>
#include <gnuradio/types.h>

#define GCC_VERSION (		\
	__GNUC__ * 10000 +	\
	__GNUC_MINOR__ * 100 +	\
	__GNUC_PATCHLEVEL__	\
)

#if GCC_VERSION >= 40800
# define complex _Complex
# undef _GLIBCXX_HAVE_COMPLEX_H
#endif

extern "C" {
#include <osmocom/dsp/cxvec.h>
#include <osmocom/dsp/cxvec_math.h>
}

#endif /* INCLUDED_GR_GMR1_CXVEC_COMPAT_H */
