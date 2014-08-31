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

#ifndef INCLUDED_GR_GMR1_RACH_DEMOD_H
#define INCLUDED_GR_GMR1_RACH_DEMOD_H

#include <gnuradio/gmr1/api.h>

#include <gnuradio/blocks/pdu.h>
#include <gnuradio/tagged_stream_block.h>

namespace gr {
  namespace gmr1 {

    /*!
     * \brief
     * \ingroup
     */
    class GR_GMR1_API rach_demod : virtual public tagged_stream_block
    {
     public:
      typedef boost::shared_ptr<rach_demod> sptr;

      static sptr make(int sps, int etoa,
                       const std::string& len_tag_key);

      virtual int sps() const = 0;
      virtual int etoa() const = 0;
    };

  } // namespace gmr1
} // namespace gr

#endif /* INCLUDED_GR_GMR1_RACH_DEMOD_H */
