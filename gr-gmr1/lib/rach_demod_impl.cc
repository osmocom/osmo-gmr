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

#include "rach_demod_impl.h"


#include "cxvec_compat.h"

extern "C" {
#include <osmocom/core/bits.h>
#include <osmocom/gmr1/l1/rach.h>
#include <osmocom/gmr1/sdr/pi4cxpsk.h>
#include <osmocom/gmr1/sdr/nb.h>
}


namespace gr {
  namespace gmr1 {

	/* FIXME: Those should be in a common include */
static const pmt::pmt_t FREQ_KEY = pmt::string_to_symbol("freq");
static const pmt::pmt_t SB_MASK_KEY = pmt::string_to_symbol("sb_mask");


rach_demod::sptr
rach_demod::make(int sps, int etoa, const std::string& len_tag_key)
{
	return gnuradio::get_initial_sptr(
		new rach_demod_impl(sps, etoa, len_tag_key)
	);
}

rach_demod_impl::rach_demod_impl(int sps, int etoa, const std::string& len_tag_key)
    : gr::tagged_stream_block("rach_demod",
                io_signature::make(1, 1, sizeof(gr_complex)),
                io_signature::make(0, 0, 0),
                len_tag_key),
      d_sps(sps), d_etoa(etoa)
{
    this->message_port_register_out(pmt::mp("port"));
}

rach_demod_impl::~rach_demod_impl()
{
	/* Nothing to do */
}


int
rach_demod_impl::sps() const
{
	return this->d_sps;
}

int
rach_demod_impl::etoa() const
{
	return this->d_etoa;
}


/*
 * Attempts to detect a burst assuming a given TOA
 */
float
rach_demod_impl::estimate(struct osmo_cxvec *burst, int etoa, float *cw_freq)
{
	const int guard = 8 * this->d_sps;
	struct osmo_cxvec _cw1, *cw1 = &_cw1;
	struct osmo_cxvec _cw2, *cw2 = &_cw2;
	float complex c1, c2;
	float r;
	int i;

	/* Map the CW1 and CW2 part of the sync sequence */
	osmo_cxvec_init_from_data(cw1,
		burst->data + etoa + this->d_sps * 127,
		this->d_sps * 32
	);

	osmo_cxvec_init_from_data(cw2,
		burst->data + etoa + this->d_sps * 191,
		this->d_sps * 32
	);

	/* Coarse (average phase change during the CW1 & CW2) */
	{
		float complex a[2] = { 0.0f, 0.0f };
		float complex b[2];
		r = 0.0f;

		for (i=guard; i<guard+5; i++)
		{
			float complex e = cexpf(I * i * -M_PIf / (4*this->d_sps));

			a[0] += cw1->data[i] * e;
			a[1] += cw2->data[i] * e;
		}

		for (i=guard; i<cw1->len-guard-5; i++)
		{
			float complex e = cexpf(I * i     * -M_PIf / (4*this->d_sps));
			float complex f = cexpf(I * (i+5) * -M_PIf / (4*this->d_sps));

			b[0] = a[0] + cw1->data[i+5] * f - cw1->data[i] * e;
			b[1] = a[1] + cw2->data[i+5] * f - cw2->data[i] * e;

			r += cargf(b[0] * conjf(a[0]));
			r += cargf(b[1] * conjf(a[1]));

			a[0] = b[0];
			a[1] = b[1];
		}

		r /= 2.0f * (cw1->len-2*guard-5);

		r += M_PIf / (4*this->d_sps);
	}

	/* Fine (looking at phase different between CW1 and CW2) */
	{
		float complex a[2] = { 0.0f, 0.0f };
		float complex eo = cexpf(I * 64.0f * this->d_sps * -r);

		for (i=guard; i<(cw1->len-guard); i++)
		{
			float complex e = cexpf(I * i * -r);

			a[0] += cw1->data[i] * e;
			a[1] += cw2->data[i] * e * eo;
		}

		r += cargf(a[1] * conjf(a[0])) / (64.0f * this->d_sps);
	}

	/* Compute the correlation value using that speed to revert rotation */
	c1 = c2 = 0.0f;

	for (i=0; i<cw1->len; i++)
	{
		float complex e = cexpf(I * i * -r);
		c1 += cw1->data[i] * e;
		c2 += cw2->data[i] * e;
	}

	/* Return the power & cw freq */
	*cw_freq = r;

	return cabsf(c1) + cabsf(c2);
}

int
rach_demod_impl::process(struct osmo_cxvec *burst, float freq_corr, uint8_t *rach, uint8_t *sb_mask)
{
	sbit_t ebits[494];
	int sync_id;
	float toa, freq_err;
	int crc[2], conv;
	int rv;

	/* Demodulate to soft bits */
	rv = gmr1_pi4cxpsk_demod(
		&gmr1_rach_burst,
		burst, this->d_sps, freq_corr,
		ebits, &sync_id, &toa, &freq_err
	);

	/* Decode */
	rv = gmr1_rach_decode(rach, ebits, 0x00, &conv, crc, sb_mask);

	/* Debug */
	//printf("rv: %d | crc: %d %d | conv: %d | sb_mask: %02hhx\n", rv, crc[0], crc[1], conv, *sb_mask);

	return crc[1];
}


int
rach_demod_impl::work(int noutput_items,
                      gr_vector_int &ninput_items,
                      gr_vector_const_void_star &input_items,
                      gr_vector_void_star &output_items)
{
	const gr_complex *in = reinterpret_cast<const gr_complex *>(input_items[0]);
	struct osmo_cxvec _burst, *burst = &_burst;
	uint8_t rach[18], sb_mask;
	float peak_corr, peak_cw_freq;
	int ws, peak_etoa, etoa;
	int rv;

	/* Init a burst with the data */
	osmo_cxvec_init_from_data(burst, (float complex *)in, ninput_items[0]);
	ws = burst->len - gmr1_rach_burst.len * this->d_sps + 1;

	/* Scan all possible TOA for the best fit */
	peak_corr = 0.0f;

	for (etoa=0; etoa<ws; etoa++)
	{
		float cw_freq, corr;

		corr = this->estimate(burst, etoa, &cw_freq);

		if (corr > peak_corr) {
			peak_corr = corr;
			peak_etoa = etoa;
			peak_cw_freq = cw_freq;
		}
	}

	/* Narrow down the window to 6 symbols around position we found */
	if (peak_etoa < (3 * this->d_sps))
		peak_etoa = 3 * this->d_sps;
	if (peak_etoa > (ws - 3 * this->d_sps))
		peak_etoa = ws - 3 * this->d_sps;

	burst->data += peak_etoa - 3 * this->d_sps;
	burst->len   = (gmr1_rach_burst.len + 6) * this->d_sps;

	/* Attempt demodulation and decoding */
	rv = this->process(burst, -((d_sps * peak_cw_freq) - (M_PIf / 4.0f)), rach, &sb_mask);

	/* Send as PDU */
	if (!rv)
	{
		std::vector<tag_t> tags;
		std::vector<tag_t>::iterator tags_itr;
		pmt::pmt_t pdu_meta, pdu_vector, msg;
		double freq = 0.0;

		/* Grab the tags into a dict */
		get_tags_in_range(tags, 0,
			nitems_read(0),
			nitems_read(0) + ninput_items[0]
		);

		pdu_meta = pmt::make_dict();
		for (tags_itr = tags.begin(); tags_itr != tags.end(); tags_itr++) {
			/* Intercept freq */
			if (pmt::eqv((*tags_itr).key, FREQ_KEY))
			{
				freq = pmt::to_double((*tags_itr).value);
				continue;
			}

			/* Add it */
			pdu_meta = dict_add(pdu_meta, (*tags_itr).key, (*tags_itr).value);
		}

		/* Add the guessed SB_Mask */
		pdu_meta = dict_add(pdu_meta,
			SB_MASK_KEY,
			pmt::from_long(sb_mask)
		);

		/* Add the freq */
		freq += ((d_sps * peak_cw_freq) - (M_PIf / 4.0f)) * 23400.0f / (2.0f * M_PIf);

		pdu_meta = dict_add(pdu_meta,
			FREQ_KEY,
			pmt::from_double(freq)
		);

		/* Build vector with the data bytes */
		pdu_vector = pdu::make_pdu_vector(gr::types::byte_t, rach, 18);

		/* Builds message */
		msg = pmt::cons(pdu_meta, pdu_vector);

		/* Send it out */
                this->message_port_register_out(pmt::mp("port"));

	}

	/* Consume the burst */
	return ninput_items[0];
}

  } // namespace gmr1
} // namespace gr
