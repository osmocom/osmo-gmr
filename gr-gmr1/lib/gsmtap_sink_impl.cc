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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <gnuradio/io_signature.h>

#include "gsmtap_sink_impl.h"

extern "C" {
#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>
#include <osmocom/gmr1/gsmtap.h>
}


namespace gr {
  namespace gmr1 {

gsmtap_sink::sptr
gsmtap_sink::make(const std::string &host, uint16_t port)
{
	return gnuradio::get_initial_sptr(
		new gsmtap_sink_impl(host, port)
	);
}

gsmtap_sink_impl::gsmtap_sink_impl(const std::string &host, uint16_t port)
    : gr::block("gsmtap_sink",
                io_signature::make(0, 0, 0),
                io_signature::make(0, 0, 0)),
      d_host(host), d_port(port)
{
	this->d_gti = gsmtap_source_init(host.c_str(), port, 0);
	gsmtap_source_add_sink(this->d_gti);

	message_port_register_in(PDU_PORT_ID);
	set_msg_handler(PDU_PORT_ID, boost::bind(&gsmtap_sink_impl::send_pdu, this, _1));
}

gsmtap_sink_impl::~gsmtap_sink_impl()
{
	/* FIXME: No destroy */
}


std::string
gsmtap_sink_impl::host() const
{
	return this->d_host;
}

uint16_t
gsmtap_sink_impl::port() const
{
	return this->d_port;
}


void
gsmtap_sink_impl::send_pdu(pmt::pmt_t pdu)
{
	pmt::pmt_t meta = pmt::car(pdu);
	pmt::pmt_t vector = pmt::cdr(pdu);
	size_t len = pmt::length(vector);
	size_t offset(0);
	const uint8_t* rach = (const uint8_t*) pmt::uniform_vector_elements(vector, offset);

	/* Send to GSMTap */
	gsmtap_sendmsg(this->d_gti, gmr1_gsmtap_makemsg(
		GSMTAP_GMR1_RACH,
		0, 0, rach, len
	));
}

  } // namespace gmr1
} // namespace gr
