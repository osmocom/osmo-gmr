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

#ifndef INCLUDED_GR_GMR1_RACH_FILE_SINK_H
#define INCLUDED_GR_GMR1_RACH_FILE_SINK_H

#include <gnuradio/gmr1/api.h>

#include <gnuradio/blocks/pdu.h>
#include <gnuradio/block.h>

namespace gr {
  namespace gmr1 {

    /*!
     * \brief
     * \ingroup
     */
    class GR_GMR1_API rach_file_sink : virtual public block
    {
     public:
      typedef boost::shared_ptr<rach_file_sink> sptr;

      static sptr make(const std::string &filename,
                       const double center_freq, const double sample_rate);

      virtual std::string filename() const = 0;

      virtual double center_freq() const = 0;
      virtual double sample_rate() const = 0;

      virtual void set_center_freq(const double center_freq) = 0;
      virtual void set_sample_rate(const double sample_rate) = 0;
    };

  } // namespace gmr1
} // namespace gr

#endif /* INCLUDED_GR_GMR1_RACH_FILE_SINK_H */
