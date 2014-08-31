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


#ifndef INCLUDED_GR_GMR1_RACH_DEMOD_IMPL_H
#define INCLUDED_GR_GMR1_RACH_DEMOD_IMPL_H

#include <gnuradio/gmr1/rach_demod.h>

struct osmo_cxvec;

namespace gr {
  namespace gmr1 {

    /*!
     * \brief
     * \ingroup gmr1
     */
    class rach_demod_impl : public rach_demod
    {
     private:
      int d_sps;
      int d_etoa;

      float estimate(struct osmo_cxvec *burst, int etoa, float *cw_freq);
      int process(struct osmo_cxvec *burst, float freq_corr, uint8_t *rach, uint8_t *sb_mask);

     public:
      rach_demod_impl(int sps, int etoa,
                      const std::string& len_tag_key);
      virtual ~rach_demod_impl();

      int sps() const;
      int etoa() const;

      int work(int noutput_items,
               gr_vector_int &ninput_items,
               gr_vector_const_void_star &input_items,
               gr_vector_void_star &output_items);
    };

  } // namespace gmr1
} // namespace gr

#endif /* INCLUDED_GR_GMR1_RACH_DEMOD_IMPL_H */
