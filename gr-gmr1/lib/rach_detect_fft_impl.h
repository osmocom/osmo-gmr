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


#ifndef INCLUDED_GR_GMR1_RACH_DETECT_FFT_IMPL_H
#define INCLUDED_GR_GMR1_RACH_DETECT_FFT_IMPL_H

#include <gnuradio/gmr1/rach_detect_fft.h>

#include <gnuradio/blocks/rotator.h>
#include <gnuradio/fft/fft.h>

namespace gr {
  namespace gmr1 {

    /*!
     * \brief
     * \ingroup gmr1
     */
    class rach_detect_fft_impl : public rach_detect_fft
    {
     private:

      class peak
      {
       public:
        uint64_t d_time[2];
        int d_bin[2];

        peak(uint64_t time, int bin);
        bool merge(uint64_t time, int bin);

        bool expired(uint64_t time_limit) const;
        uint64_t time() const;
        float bin() const;
      };

      double d_sample_rate;
      int d_fft_size;
      int d_overlap_ratio;
      float d_threshold;
      int d_burst_length;
      int d_burst_offset;
      float d_peak_freq;

      pmt::pmt_t d_len_tag_key;
      pmt::pmt_t d_burst_length_pmt;

      gr::fft::fft_complex *d_fft;
      gr_complex *d_buf;
      float *d_win;
      float *d_pwr;
      float *d_avg;

      int d_in_pos;
      std::vector<peak> d_peaks_l1;
      std::vector<peak> d_peaks_l2;

      int d_out_pos;
      gr::blocks::rotator d_r;
      std::vector<peak> d_peaks_pending;

      void peak_detect(uint64_t position);

     public:
      rach_detect_fft_impl(const double sample_rate,
                           const int overlap_ratio, const float threshold,
                           const int burst_length, const int burst_offset,
                           const float peak_freq,
                           const std::string& len_tag_key);
      virtual ~rach_detect_fft_impl();

      int general_work(int noutput_items,
                       gr_vector_int& ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items);
    };

  } // namespace gmr1
} // namespace gr

#endif /* INCLUDED_GR_GMR1_RACH_DETECT_FFT_IMPL_H */
