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

#include <stdio.h>

#include <gnuradio/io_signature.h>
#include <gnuradio/fft/window.h>
#include <volk/volk.h>

#include "rach_detect_fft_impl.h"


namespace gr {
  namespace gmr1 {

	/* FIXME: Those should be in a common include */
static const pmt::pmt_t SOB_KEY  = pmt::string_to_symbol("sob");
static const pmt::pmt_t EOB_KEY  = pmt::string_to_symbol("eob");
static const pmt::pmt_t FREQ_KEY = pmt::string_to_symbol("freq");


rach_detect_fft::sptr
rach_detect_fft::make(
	int fft_size, int overlap_ratio, float threshold,
	int burst_length, int burst_offset, float freq_offset,
	const std::string& len_tag_key)
{
	return gnuradio::get_initial_sptr(
		new rach_detect_fft_impl(
			fft_size, overlap_ratio, threshold,
			burst_length, burst_offset, freq_offset,
			len_tag_key
		)
	);
}

rach_detect_fft_impl::rach_detect_fft_impl(
	int fft_size, int overlap_ratio, float threshold,
	int burst_length, int burst_offset, float freq_offset,
	const std::string& len_tag_key)
    : gr::block("rach_detect_fft",
                io_signature::make(1, 1, sizeof(gr_complex)),
                io_signature::make(1, 1, sizeof(gr_complex))),
      d_fft_size(fft_size), d_overlap_ratio(overlap_ratio),
      d_threshold(threshold),
      d_burst_length(burst_length), d_burst_offset(burst_offset),
      d_freq_offset(freq_offset),
      d_len_tag_key(pmt::string_to_symbol(len_tag_key)),
      d_burst_length_pmt(pmt::from_long(burst_length))
{
	this->d_fft = new gr::fft::fft_complex(this->d_fft_size, true, 1);

	this->d_buf = (gr_complex *) volk_malloc(this->d_fft_size * sizeof(gr_complex), 128);
	this->d_win = (float *) volk_malloc(this->d_fft_size * sizeof(float), 128);
	this->d_pwr = (float *) volk_malloc(this->d_fft_size * sizeof(float), 128);
	this->d_avg = (float *) volk_malloc(this->d_fft_size * sizeof(float), 128);

	memset(this->d_avg, 0x00, this->d_fft_size * sizeof(float));

	this->d_in_pos = 0;
	this->d_out_pos = 0;

	std::vector<float> win = gr::fft::window::blackmanharris(this->d_fft_size);
	memcpy(this->d_win, &win[0], this->d_fft_size * sizeof(float));

	this->set_history(burst_length + 1);
}

rach_detect_fft_impl::~rach_detect_fft_impl()
{
	volk_free(this->d_avg);
	volk_free(this->d_pwr);
	volk_free(this->d_win);
	volk_free(this->d_buf);

	delete this->d_fft;
}


void
rach_detect_fft_impl::peak_detect(uint64_t position)
{
	std::vector<peak>::iterator it1, it2;
	const int avg_hwin = 15;
	const int avg_win = (avg_hwin * 2) + 1;
	float sum;
	int i;

	/* Prime the moving average */
	sum = 0.0f;
	for (i=0; i<avg_win; i++)
		sum += this->d_pwr[i];

	/* Do a scan and compare with avg with peak */
	for (i=avg_hwin; i<this->d_fft_size-avg_hwin; i++)
	{
		/* Is this a peak ? */
		if (this->d_pwr[i] > (this->d_threshold * sum / (float)avg_win) &&
		    this->d_pwr[i] > (this->d_threshold * this->d_avg[i]))
		{
			bool merged = false;

			/* Attempt merge with existing */
			for (it1=this->d_peaks_l1.begin(); it1!=this->d_peaks_l1.end() && !merged; it1++)
			{
				merged |= it1->merge(position, i);
			}

#if 0
			printf("%lld %d %d (%f %f %f)\n", position, i, merged, this->d_pwr[i], sum / (float)avg_win, this->d_avg[i]);
#endif

			/* No match, insert new peak */
			if (!merged)
				this->d_peaks_l1.push_back(peak(position, i));
		}

		/* Update moving average */
		sum += this->d_pwr[i+avg_hwin+1] - this->d_pwr[i-avg_hwin];
	}

	/* Scan for expired peak at Level 1 */
	for (it1=this->d_peaks_l1.begin(); it1!=this->d_peaks_l1.end();)
	{
		/* Expired ? */
		if (it1->expired(position - 20))
		{
			bool matched = false;

			/* Scan Level 2 for a match at the right distance */
			for (it2=this->d_peaks_l2.begin(); it2 != this->d_peaks_l2.end() && !matched; it2++)
			{
				if ((fabsf(it1->bin() - it2->bin()) < 1.0f) &&
				    (it1->d_time[1] - it2->d_time[0] <= 4 * this->d_overlap_ratio) &&
				    (it1->d_time[1] - it2->d_time[0] >= 2 * this->d_overlap_ratio))
				{
					matched = true;

#if 0
					printf("Match: (%llu %llu %d %d) (%llu %llu %d %d)\n",
						it1->d_time[0], it1->d_time[1], it1->d_bin[0], it1->d_bin[1],
						it2->d_time[0], it2->d_time[1], it2->d_bin[0], it2->d_bin[1]
					);
#endif

					this->d_peaks_pending.push_back(*it1);
					this->d_peaks_l2.erase(it2);
				}
			}

			/* No match, insert in Level 2 */
			if (!matched)
				this->d_peaks_l2.push_back(*it1);

			/* Remove from Level 1 */
			it1 = this->d_peaks_l1.erase(it1);
		} else {
			/* Just move on */
			it1++;
		}
	}

	/* Scan for expired peaks at Level 2 */
	for (it1=this->d_peaks_l2.begin(); it1!=this->d_peaks_l2.end();)
	{
		if (it1->expired(position - 40)) {
			it1 = this->d_peaks_l2.erase(it1);
		} else {
			it1++;
		}
	}
}


int
rach_detect_fft_impl::general_work(
	int noutput_items,
	gr_vector_int& ninput_items,
	gr_vector_const_void_star &input_items,
	gr_vector_void_star &output_items)
{
	int read, max_read;

	/* Buffers pointer */
	const gr_complex *sig_in = reinterpret_cast<const gr_complex *>(input_items[0]);
	gr_complex *burst_out = reinterpret_cast<gr_complex *>(output_items[0]);
	gr_complex *fft_in  = this->d_fft->get_inbuf();
	gr_complex *fft_out = this->d_fft->get_outbuf();

	/* Skip over history */
	sig_in += this->history() - 1;

	/* If there is pending output, process this first */
	if (!this->d_peaks_pending.empty())
	{
		peak pk = this->d_peaks_pending.back();
		int to_copy = this->d_burst_length - this->d_out_pos;

		if (to_copy > noutput_items)
			to_copy = noutput_items;

		if (to_copy == 0)
			return 0;

		if (this->d_out_pos == 0)
		{
			float phase_inc;

			/* Configure the rotator */
			phase_inc = - (2.0f * (float)M_PI / this->d_fft_size) * (
				fmodf(
					pk.bin() + (float)(this->d_fft_size / 2),
					(float)this->d_fft_size
				) - (float)(this->d_fft_size / 2)
			);

			phase_inc += this->d_freq_offset;

			this->d_r.set_phase_incr( exp(gr_complex(0, phase_inc)) );

			/* Burst start */
			add_item_tag(
				0,
				this->nitems_written(0),
				this->d_len_tag_key,
				this->d_burst_length_pmt
			);

			/* Burst SOB marker */
			add_item_tag(
				0,
				this->nitems_written(0),
				SOB_KEY,
				pmt::PMT_NIL
			);

			/* Burst angular frequency */
			add_item_tag(
				0,
				this->nitems_written(0),
				FREQ_KEY,
				pmt::from_double(phase_inc)
			);
		}

		this->d_r.rotateN(
			burst_out,
			sig_in + this->d_out_pos - this->history() + 1,
			to_copy
		);

		this->d_out_pos += to_copy;

		if (this->d_out_pos == this->d_burst_length)
		{
			/* Burst EOB marker */
			add_item_tag(
				0,
				this->nitems_written(0) + to_copy - 1,
				EOB_KEY,
				pmt::PMT_NIL
			);

			/* Reset for next */
			this->d_peaks_pending.pop_back();
			this->d_out_pos = 0;
		}

		return to_copy;
	}

	/* Process as much input as we can */
	max_read = ninput_items[0] - this->history() + 1;

	for (read=0; read<max_read;)
	{
		int n_adv = this->d_fft_size / this->d_overlap_ratio;
		int n_reuse = this->d_fft_size - n_adv;
		int n_fill;

		/* Fill our internal buffer */
		n_fill = this->d_fft_size - this->d_in_pos;
		if (n_fill > max_read - read)
			n_fill = max_read - read;

		memcpy(this->d_buf + this->d_in_pos,
		       sig_in + read,
		       n_fill * sizeof(gr_complex));

		read += n_fill;
		this->d_in_pos += n_fill;

		if (this->d_in_pos != this->d_fft_size)
			break;

		/* Apply window */
		volk_32fc_32f_multiply_32fc(
			fft_in,
			this->d_buf,
			this->d_win,
			this->d_fft_size
		);

		/* Compute FFT */
		this->d_fft->execute();

		/* Compute the squared power */
		volk_32fc_magnitude_squared_32f(this->d_pwr, fft_out, this->d_fft_size);

		/* Compute the per-bin IIR average */
		for (int i=0; i<this->d_fft_size; i++)
		{
			const float alpha = 0.01f, one_minus_alpha = 1.0f - alpha;
			this->d_avg[i] = (this->d_avg[i] * one_minus_alpha)
			               + (this->d_pwr[i] * alpha);
		}

		/* Run the peak detection */
		this->peak_detect(
			(this->nitems_read(0) + read) / (this->d_fft_size / this->d_overlap_ratio)
		);

		/* Handle overlap */
		if (this->d_overlap_ratio > 1) {
			memmove(this->d_buf, this->d_buf + n_adv, n_reuse * sizeof(gr_complex));
			this->d_in_pos = n_reuse;
		} else {
			this->d_in_pos = 0;
		}

		/* If we have anything pending, don't continue */
		if (!this->d_peaks_pending.empty())
			break;
	}

	/* We read some stuff */
	this->consume_each(read);

	return 0;
}


rach_detect_fft_impl::peak::peak(uint64_t time, int bin)
{
	this->d_time[0] = this->d_time[1] = time;
	this->d_bin[0]  = this->d_bin[1]  = bin;
}


bool
rach_detect_fft_impl::peak::merge(uint64_t time, int bin)
{
	/* Check if mergeable */
	if (bin > (this->d_bin[1] + 2))
		return false;

	if (bin < (this->d_bin[0] - 2))
		return false;

	if (time > (this->d_time[1] + 2))
		return false;

	if (time < (this->d_time[0] - 2))
		return false;

	/* Do the merge */
	if (bin > this->d_bin[1])
		this->d_bin[1] = bin;
	else if (bin < this->d_bin[0])
		this->d_bin[0] = bin;

	if (time > this->d_time[1])
		this->d_time[1] = time;
	else if (time < this->d_time[0])
		this->d_time[0] = time;

	return true;
}

bool
rach_detect_fft_impl::peak::expired(uint64_t time_limit) const
{
	return time_limit > this->d_time[1];
}

uint64_t
rach_detect_fft_impl::peak::time() const
{
	return this->d_time[0] + ((this->d_time[1] - this->d_time[0]) >> 1);
}

float
rach_detect_fft_impl::peak::bin() const
{
	return (float)this->d_bin[0] + ((float)(this->d_bin[1] - this->d_bin[0]) / 2.0f);
}


  } // namespace gmr1
} // namespace gr
