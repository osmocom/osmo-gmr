/* -*- c++ -*- */
/*
 * Copyright 2015 Sylvain Munaut <tnt@246tNt.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <inttypes.h>
#include <string.h>
#include <cstdio>

#include <gnuradio/io_signature.h>

#include "rach_file_sink_impl.h"


namespace gr {
  namespace gmr1 {

rach_file_sink::sptr
rach_file_sink::make(const std::string &filename,
                     const double center_freq, const bool invert_freq)
{
	return gnuradio::get_initial_sptr(
		new rach_file_sink_impl(filename, center_freq, invert_freq)
	);
}

rach_file_sink_impl::rach_file_sink_impl(const std::string &filename,
                                         const double center_freq,
                                         const bool invert_freq)
    : gr::block("rach_file_sink",
                io_signature::make(0, 0, 0),
                io_signature::make(0, 0, 0)),
      d_filename(filename),
      d_center_freq(center_freq), d_invert_freq(invert_freq)
{
	message_port_register_in(PDU_PORT_ID);
	set_msg_handler(PDU_PORT_ID, boost::bind(&rach_file_sink_impl::send_pdu, this, _1));

	this->d_fh = fopen(filename.c_str(), "a");
	if (!this->d_fh)
		throw std::runtime_error("Unable to open output file");
}

rach_file_sink_impl::~rach_file_sink_impl()
{
	fclose(this->d_fh);
}


std::string
rach_file_sink_impl::filename() const
{
	return this->d_filename;
}

double
rach_file_sink_impl::center_freq() const
{
	return this->d_center_freq;
}

bool
rach_file_sink_impl::invert_freq() const
{
	return this->d_invert_freq;
}

void
rach_file_sink_impl::set_center_freq(const double center_freq)
{
	this->d_center_freq = center_freq;
}

void
rach_file_sink_impl::set_invert_freq(const bool invert_freq)
{
	this->d_invert_freq = invert_freq;
}


bool
rach_file_sink_impl::start()
{
	/* Nothing yet */
}

bool
rach_file_sink_impl::stop()
{
	fflush(this->d_fh);
}


void
rach_file_sink_impl::send_pdu(pmt::pmt_t pdu)
{
	static const pmt::pmt_t key_sb_mask = pmt::string_to_symbol("sb_mask");
	static const pmt::pmt_t key_freq = pmt::string_to_symbol("freq");
	static const pmt::pmt_t key_time = pmt::string_to_symbol("time");

	pmt::pmt_t meta = pmt::car(pdu);
	pmt::pmt_t vector = pmt::cdr(pdu);
	size_t len = pmt::length(vector);
	size_t offset(0);
	const uint8_t* rach = (const uint8_t*) pmt::uniform_vector_elements(vector, offset);

	/* Validate */
	if (!is_dict(meta) ||
	    !dict_has_key(meta, key_sb_mask) ||
	    !dict_has_key(meta, key_freq))
		throw std::runtime_error("Invalid RACH PDU");

	/* If we have time, start with it */
	if (dict_has_key(meta, key_freq))
	{
		uint64_t time = pmt::to_uint64(pmt::dict_ref(meta, key_time, pmt::PMT_NIL));
		fprintf(this->d_fh, "%" PRIu64 " ", time);
	}

	/* Grab SB_Mask & freq */
	uint8_t sb_mask = pmt::to_long(pmt::dict_ref(meta, key_sb_mask, pmt::PMT_NIL));
	double freq = pmt::to_double(pmt::dict_ref(meta, key_freq, pmt::PMT_NIL));

	if (this->d_invert_freq)
		freq = -freq;

	fprintf(this->d_fh, "%02hhx %.17g %.17g ",
		sb_mask,
		freq,
		this->d_center_freq + freq
	);

	/* Raw bytes */
	for (int i=0; i<len; i++)
		fprintf(this->d_fh, "%02hhx", rach[i]);

	/* EOL */
	fprintf(this->d_fh, "\n");
}

  } // namespace gmr1
} // namespace gr
