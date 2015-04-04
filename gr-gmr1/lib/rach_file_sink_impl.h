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


#ifndef INCLUDED_GR_GMR1_RACH_FILE_SINK_IMPL_H
#define INCLUDED_GR_GMR1_RACH_FILE_SINK_IMPL_H

#include <gnuradio/gmr1/rach_file_sink.h>

#include <cstdio>

namespace gr {
  namespace gmr1 {

    /*!
     * \brief
     * \ingroup gmr1
     */
    class rach_file_sink_impl : public rach_file_sink
    {
     private:
      std::string d_filename;
      double d_center_freq;
      bool d_invert_freq;

      FILE *d_fh;

      void send_pdu(pmt::pmt_t pdu);

     public:
      rach_file_sink_impl(const std::string &filename,
                          const double center_freq, const bool invert_freq);
      virtual ~rach_file_sink_impl();

      std::string filename() const;
      double center_freq() const;
      bool invert_freq() const;

      void set_center_freq(const double center_freq);
      void set_invert_freq(const bool invert_freq);

      bool start();
      bool stop();
    };

  } // namespace gmr1
} // namespace gr

#endif /* INCLUDED_GR_GMR1_RACH_FILE_SINK_IMPL_H */
