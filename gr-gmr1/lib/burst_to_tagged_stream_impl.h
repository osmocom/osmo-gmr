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


#ifndef INCLUDED_GR_GMR1_BURST_TO_TAGGED_STREAM_IMPL_H
#define INCLUDED_GR_GMR1_BURST_TO_TAGGED_STREAM_IMPL_H

#include <gnuradio/gmr1/burst_to_tagged_stream.h>

namespace gr {
  namespace gmr1 {

    /*!
     * \brief
     * \ingroup gmr1
     */
    class burst_to_tagged_stream_impl : public burst_to_tagged_stream
    {
     private:
      bool d_active;
      int  d_offset;
      pmt::pmt_t d_len_tag_key;


     public:
      burst_to_tagged_stream_impl(int max_length, const std::string& len_tag_key);
      virtual ~burst_to_tagged_stream_impl();

      void forecast(int noutput_items,
                    gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items);
    };

  } // namespace gmr1
} // namespace gr

#endif /* INCLUDED_GR_GMR1_BURST_TO_TAGGED_STREAM_IMPL_H */
