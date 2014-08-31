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


#ifndef INCLUDED_GR_GMR1_GSMTAP_SINK_IMPL_H
#define INCLUDED_GR_GMR1_GSMTAP_SINK_IMPL_H

#include <gnuradio/gmr1/gsmtap_sink.h>

struct gsmtap_inst;

namespace gr {
  namespace gmr1 {

    /*!
     * \brief
     * \ingroup gmr1
     */
    class gsmtap_sink_impl : public gsmtap_sink
    {
     private:
      std::string d_host;
      uint16_t d_port;

      struct gsmtap_inst *d_gti;

      void send_pdu(pmt::pmt_t pdu);

     public:
      gsmtap_sink_impl(const std::string &host, uint16_t port);
      virtual ~gsmtap_sink_impl();

      std::string host() const;
      uint16_t port() const;
    };

  } // namespace gmr1
} // namespace gr

#endif /* INCLUDED_GR_GMR1_GSMTAP_SINK_IMPL_H */
