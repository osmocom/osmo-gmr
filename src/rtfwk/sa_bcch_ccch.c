/* GMR-1 RT framework: BCCH/CCCH task */

/* (C) 2011-2019 by Sylvain Munaut <tnt@246tNt.com>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <complex.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>

#include <osmocom/dsp/cxvec.h>

#include <osmocom/gmr1/gsmtap.h>
#include <osmocom/gmr1/l1/bcch.h>
#include <osmocom/gmr1/l1/ccch.h>
#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/pi4cxpsk.h>
#include <osmocom/gmr1/sdr/nb.h>

#include "common.h"
#include "sampbuf.h"
#include "sa_bcch_ccch.h"
#include "sa_tch3.h"


#define BCCH_MARGIN     100


struct bcch_sink_priv {
	struct app_state *as;
	int chan_id;

	uint64_t align;
	int align_err;
	float freq_err;

	uint32_t fn;
	int sa_sirfn_delay;
	int sa_bcch_stn;

	float bcch_energy;
	int bcch_err;

	int la_arfcn;
	int la_tn;
	int la_dkab_pos;

	int aligned;
};

static int
bcch_sink_init(struct sample_actor *sa, void *params_ptr)
{
	struct bcch_sink_priv *priv = sa->priv;
	struct bcch_sink_params *params = params_ptr;

	priv->as = params->as;
	priv->chan_id = params->chan_id;

	priv->align = params->align;
	priv->freq_err = params->freq_err;

	return 0;
}

static void
bcch_sink_fini(struct sample_actor *sa)
{
	/* struct bcch_sink_priv *priv = sa->priv; */

	/* Nothing to do */
}

static int
_bcch_tdma_align(struct bcch_sink_priv *priv, uint8_t *l2)
{
	int sa_sirfn_delay, sa_bcch_stn;
	int superframe_num, multiframe_num, mffn_high_bit;
	int fn;

	/* Check if it's a SI1 */
	if ((l2[0] & 0xf8) != 0x08)
		return 0;

	/* Check if it contains a Seg 2A bis */
	if ((l2[9] & 0xfc) != 0x80)
		return 0;

	/* Retrieve SA_SIRFN_DELAY, SA_BCCH_STN,
	 * Superframe number, Multiframe number, MFFN high bit */
	sa_sirfn_delay =  (l2[10] >> 3) & 0x0f;
	sa_bcch_stn    = ((l2[10] << 2) & 0x1c) | (l2[11] >> 6);

	superframe_num = ((l2[11] & 0x3f) << 7) | (l2[12] >> 1);
	multiframe_num = ((l2[12] & 0x01) << 1) | (l2[13] >> 7);
	mffn_high_bit  = ((l2[13] & 0x40) >> 6);

	/* Compute frame number */
	fn = (superframe_num << 6) |
	     (multiframe_num << 4) |
	     (mffn_high_bit << 3) |
	     ((2 + sa_sirfn_delay) & 7);

	/* Fix SDR alignement */
	priv->align_err += (priv->sa_bcch_stn - sa_bcch_stn) * 39 * priv->as->sps;

	/* Align TDMA */
	priv->fn = fn;
	priv->sa_sirfn_delay = sa_sirfn_delay;
	priv->sa_bcch_stn = sa_bcch_stn;

	return 0;
}

static int
_rx_bcch(struct sample_actor *sa,
         float complex *data, unsigned int data_len)
{
	struct bcch_sink_priv *priv = sa->priv;
	struct osmo_cxvec _burst, *burst = &_burst;
	sbit_t ebits[424];
	uint8_t l2[24];
	float freq_err, toa;
	int sps, base_align;
	int rv, crc, conv, e_toa;

	/* Get quick params */
	sps = priv->as->sps;
	base_align = sps * BCCH_MARGIN;

	/* Debug */
	fprintf(stderr, "[.]   BCCH\n");

	/* Demodulate burst */
	e_toa = burst_map(burst, data, data_len, base_align, sps,
	                  &gmr1_bcch_burst, priv->sa_bcch_stn, 20 * sps);
	if (e_toa < 0)
		return e_toa;

	rv = gmr1_pi4cxpsk_demod(
		&gmr1_bcch_burst,
		burst, sps, -priv->freq_err,
		ebits, NULL, &toa, &freq_err
	);

	if (rv)
		return rv;

	/* Measure energy as a reference */
	priv->bcch_energy = burst_energy(burst);

	/* Decode burst */
	crc = gmr1_bcch_decode(l2, ebits, &conv);

	fprintf(stderr, "crc=%d, conv=%d\n", crc, conv);

	/* If burst turned out OK, use data to align channel */
	if (!crc) {
		/* SDR alignement */
		priv->align_err += ((int)roundf(toa)) - e_toa;
		priv->freq_err += freq_err;

		/* Acquire TDMA alignement */
		_bcch_tdma_align(priv, l2);
	}

	/* Count errors */
	if (!crc)
		priv->bcch_err = 0;
	else
		priv->bcch_err++;

	/* Send to GSMTap if correct */
	if (!crc)
		gsmtap_sendmsg(priv->as->gti, gmr1_gsmtap_makemsg(
			GSMTAP_GMR1_BCCH,
			priv->as->chans[priv->chan_id].arfcn,
			priv->fn, priv->sa_bcch_stn, l2, 24));

	return 0;
}

static inline int
_ccch_is_imm_ass(const uint8_t *l2)
{
	return (l2[1] == 0x06) && (l2[2] == 0x3f);
}

static void
_ccch_imm_ass_parse(const uint8_t *l2, int *arfcn, int *rx_tn, int *p)
{
	*p = (l2[8] & 0xfc) >> 2;
	*rx_tn = ((l2[8] & 0x03) << 3) | (l2[9] >> 5);
	*arfcn = ((l2[9] & 0x1f) << 6) | (l2[10] >> 2);
}

static int
_rx_ccch(struct sample_actor *sa,
         float complex *data, unsigned int data_len)
{
	struct bcch_sink_priv *priv = sa->priv;
	struct osmo_cxvec _burst, *burst = &_burst;
	sbit_t ebits[432];
	uint8_t l2[24];
	int sps, base_align;
	int rv, crc, conv, e_toa;

	/* Get quick params */
	sps = priv->as->sps;
	base_align = sps * BCCH_MARGIN;

	/* Map potential burst */
	e_toa = burst_map(burst, data, data_len, base_align, sps,
	                  &gmr1_dc6_burst, priv->sa_bcch_stn, 10 * sps);
	if (e_toa < 0)
		return e_toa;

	/* Energy detection */
	if (burst_energy(burst) < (priv->bcch_energy / 2.0f))
		return 0; /* Nothing to do */

	/* Debug */
	fprintf(stderr, "[.]   CCCH\n");

	/* Demodulate burst */
	rv = gmr1_pi4cxpsk_demod(
		&gmr1_dc6_burst,
		burst, sps, -priv->freq_err,
		ebits, NULL, NULL, NULL
	);

	if (rv)
		return rv;

	/* Decode burst */
	crc = gmr1_ccch_decode(l2, ebits, &conv);

	fprintf(stderr, "crc=%d, conv=%d\n", crc, conv);

	/* Check for IMM.ASS */
	if (!crc && _ccch_is_imm_ass(l2)) {
		struct tch3_sink_params p;
		struct sample_actor *nsa;
		int arfcn, tn, dkab_pos;
		int i;

		/* Parse ASS */
		_ccch_imm_ass_parse(l2, &arfcn, &tn, &dkab_pos);

		/* Quick & Dirty check for dupes */
		if ((priv->la_arfcn == arfcn) &&
		    (priv->la_tn == tn) &&
		    (priv->la_dkab_pos == dkab_pos))
			goto nofollow;

		priv->la_arfcn = arfcn;
		priv->la_tn = tn;
		priv->la_dkab_pos = dkab_pos;

		/* Debug print */
		fprintf(stderr, "[+] TCH3 assigned on ARFCN %d TN %d DKAB %d\n",
			arfcn, tn, dkab_pos);

		/* Find matching channel ID */
		for (i=0; i<priv->as->n_chans; i++)
			if (priv->as->chans[i].arfcn == arfcn)
				break;

		if (i == priv->as->n_chans) {
			fprintf(stderr, "No data stream available for that ARFCN\n");
			goto nofollow;
		}

		/* Start TCH3 task */
		p.as       = priv->as;
		p.chan_id  = i;
		p.fn       = priv->fn;
		p.align    = sa->time + base_align;
		p.freq_err = priv->freq_err;
		p.tn       = tn;
		p.dkab_pos = dkab_pos;
		p.ref_energy = priv->bcch_energy / 2.0f;

		nsa = sbuf_add_consumer(priv->as->buf, p.chan_id, &tch3_sink, &p);
		if (!nsa) {
			fprintf(stderr, "[!] Failed to create TCH3 sink for stream #%d\n", p.chan_id);
			return -ENOMEM;
		}
	}
nofollow:

	/* Send to GSMTap if correct */
	if (!crc)
		gsmtap_sendmsg(priv->as->gti, gmr1_gsmtap_makemsg(
			GSMTAP_GMR1_CCCH,
			priv->as->chans[priv->chan_id].arfcn,
			priv->fn, priv->sa_bcch_stn, l2, 24));

	return 0;
}

static int
bcch_sink_work(struct sample_actor *sa,
               float complex *data, unsigned int data_len)
{
	struct bcch_sink_priv *priv = sa->priv;
	int sps, base_align, frame_len, sirfn;

	/* Get quick params */
	sps = priv->as->sps;
	frame_len = sps * 39 * 24;
	base_align = sps * BCCH_MARGIN;

	/* If not aligned ... do that first */
	if (!priv->aligned) {
		/* Basically we want to have :
		 * |#|Frame0|Frame1|#|  with the # being
		 * margin blocks
		 */

		uint64_t target = priv->align - sps * BCCH_MARGIN;
		int discard;

		if (target < sa->time) {
			target += frame_len;
			priv->align += frame_len;
		}

		discard = target - sa->time;

		if (discard > data_len)
			return data_len;

		priv->aligned = 1;

		return discard;
	}

	/* Check we have enough data (2*BCCH_MARGIN + 2*frame_len) */
	if (data_len < (2*BCCH_MARGIN*sps + 2*frame_len))
		return 0;

	/* Debug print */
	fprintf(stderr, "[-]  FN: %6d (@%d:%llu)\n",
		priv->fn, priv->chan_id, (long long unsigned int)(sa->time + base_align));

	/* SI relative frame number inside an hyperframe */
	sirfn = (priv->fn - priv->sa_sirfn_delay) & 63;

	/* BCCH */
	if (sirfn % 8 == 2)
		_rx_bcch(sa, data, data_len);

	if (priv->bcch_err > 10)
		return -1;

	/* CCCH */
	if ((sirfn % 8 != 0) && (sirfn % 8 != 2))
		_rx_ccch(sa, data, data_len);

	/* Next frame */
	priv->fn += 1;

	frame_len += priv->align_err;
	priv->align_err = 0;

	return frame_len;
}

const struct sample_actor_desc bcch_sink = {
	.init = bcch_sink_init,
	.fini = bcch_sink_fini,
	.work = bcch_sink_work,
	.priv_size = sizeof(struct bcch_sink_priv),
};
