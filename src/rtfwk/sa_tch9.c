/* GMR-1 RT framework: TCH9 task */

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
#include <string.h>

#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>

#include <osmocom/dsp/cxvec.h>

#include <osmocom/gmr1/gsmtap.h>
#include <osmocom/gmr1/l1/a5.h>
#include <osmocom/gmr1/l1/facch9.h>
#include <osmocom/gmr1/l1/tch9.h>
#include <osmocom/gmr1/l1/interleave.h>
#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/pi4cxpsk.h>
#include <osmocom/gmr1/sdr/nb.h>

#include "common.h"
#include "sampbuf.h"
#include "sa_tch9.h"


#define TCH9_MARGIN	50


struct tch9_sink_priv {
	/* App */
	struct app_state *as;
	int chan_id;

	/* Alignement time/freq */
	uint32_t fn;
	uint64_t align;
	int align_err;
	float freq_err;

	int tn;

	int aligned;

	/* End detection */
	int bad_crc;

	/* Cipher */
	int ciph;
	uint8_t kc[8];

	/* Interleaver */
	struct gmr1_interleaver il;

	/* Output data */
	FILE *data;
};

static int
tch9_sink_init(struct sample_actor *sa, void *params_ptr)
{
	struct tch9_sink_priv *priv = sa->priv;
	struct tch9_sink_params *params = params_ptr;

	priv->as = params->as;
	priv->chan_id = params->chan_id;

	priv->fn = params->fn;
	priv->align = params->align;
	priv->freq_err = params->freq_err;

	priv->tn = params->tn;

	priv->ciph = params->ciph;
	memcpy(priv->kc, params->kc, 8);

	gmr1_interleaver_init(&priv->il, 3, 648);

	return 0;
}

static void
tch9_sink_fini(struct sample_actor *sa)
{
	struct tch9_sink_priv *priv = sa->priv;

	gmr1_interleaver_fini(&priv->il);

	if (priv->data)
		fclose(priv->data);
}

static int
tch9_sink_work(struct sample_actor *sa,
               float complex *data, unsigned int data_len)
{
	struct tch9_sink_priv *priv = sa->priv;
	struct osmo_cxvec _burst, *burst = &_burst;
	int sps, base_align, frame_len;
	int e_toa, rv, sync_id, crc, conv;
	sbit_t ebits[662], bits_sacch[10], bits_status[4];
	ubit_t ciph[658];
	float toa, freq_err;

	/* Get quick params */
	sps = priv->as->sps;
	frame_len = sps * 39 * 24;
	base_align = sps * TCH9_MARGIN;

	/* If not aligned ... do that first */
	if (!priv->aligned) {
		/* Basically we want to have :
		 * |#|Frame|#| with the # being margin.
		 */

		uint64_t target = priv->align - sps * TCH9_MARGIN;
		int discard;

		if (target < sa->time) {
			target += frame_len;
			priv->fn += 1;
			priv->align += frame_len;
		}

		discard = target - sa->time;

		if (discard > data_len)
			return data_len;

		priv->aligned = 1;

		return discard;
	}

	/* Check we have enough data (frame_len) */
	if (data_len < (frame_len + 2*TCH9_MARGIN))
		return 0;

	/* Map potential burst */
	e_toa = burst_map(burst, data, data_len, base_align, sps,
	                  &gmr1_nt9_burst, priv->tn, sps + (sps/2), NULL);
	if (e_toa < 0)
		return e_toa;

	/* Demodulate burst */
	rv = gmr1_pi4cxpsk_demod(
		&gmr1_nt9_burst,
		burst, sps, -priv->freq_err,
		ebits, &sync_id, &toa, &freq_err
	);
	if (rv < 0)
		return rv;

	fprintf(stderr, "[.]   %s\n", sync_id ? "TCH9" : "FACCH9");
	fprintf(stderr, "toa=%.1f, sync_id=%d\n", toa, sync_id);

	/* Track frequency */
	priv->freq_err += freq_err;

	/* Process depending on type */
	if (!sync_id) { /* FACCH9 */
		uint8_t l2[38];

		/* Generate cipher stream */
		gmr1_a5(priv->ciph, priv->kc, priv->fn, 658, ciph, NULL);

		/* Decode */
		crc = gmr1_facch9_decode(l2, bits_sacch, bits_status, ebits, ciph, &conv);
		fprintf(stderr, "crc=%d, conv=%d\n", crc, conv);

		/* Send to GSMTap if correct */
		if (!crc)
			gsmtap_sendmsg(priv->as->gti, gmr1_gsmtap_makemsg(
				GSMTAP_GMR1_TCH9 | GSMTAP_GMR1_FACCH,
				priv->as->chans[priv->chan_id].arfcn,
				priv->fn, priv->tn, l2, 38));

		/* Detect end */
		if (crc)
			if (priv->bad_crc++ > 10)
				return -1;
	} else { /* TCH9 */
		uint8_t l2[60];
		int i, s = 0;

		/* Generate cipher stream */
		gmr1_a5(priv->ciph, priv->kc, priv->fn, 658, ciph, NULL);

		for (i=0; i<662; i++)
			s += ebits[i] < 0 ? -ebits[i] : ebits[i];
		s /= 662;

		/* Decode */
		gmr1_tch9_decode(l2, bits_sacch, bits_status, ebits, GMR1_TCH9_9k6, ciph, &priv->il, &conv);
		fprintf(stderr, "fn=%d, conv9=%d, avg=%d\n", priv->fn, conv, s);

		/* Forward to GSMTap (no CRC to validate :( ) */
		gsmtap_sendmsg(priv->as->gti, gmr1_gsmtap_makemsg(
			GSMTAP_GMR1_TCH9,
			priv->as->chans[priv->chan_id].arfcn,
			priv->fn, priv->tn, l2, 60));

		/* Save to file */
		{
			if (!priv->data) {
				char fname[256];
				sprintf(fname, "/tmp/gmr1_csd_%d_%d_%d.dat", priv->as->chans[priv->chan_id].arfcn, priv->tn, priv->fn);
				priv->data = fopen(fname, "wb");
			}
			fwrite(l2, 60, 1, priv->data);
		}
	}

	/* Accumulate alignement error */
	fprintf(stderr, "toa=%f | %d %d %d\n", toa, e_toa, ((int)roundf(toa)) - e_toa, priv->align_err);
	priv->align_err += ((int)roundf(toa)) - e_toa;

	/* Take align_err into account */
	if (priv->align_err > 4) {
		frame_len++;
		priv->align_err -= 4;
		fprintf(stderr, ">>>> REALIGN +++ %d\n", priv->align_err);
	} else if (priv->align_err < -4) {
		frame_len--;
		priv->align_err += 4;
		fprintf(stderr, ">>>> REALIGN --- %d\n", priv->align_err);
	}

	/* Done, go to next frame */
	priv->fn += 1;

	return frame_len;
}

const struct sample_actor_desc tch9_sink = {
	.name = "TCH9",
	.init = tch9_sink_init,
	.fini = tch9_sink_fini,
	.work = tch9_sink_work,
	.priv_size = sizeof(struct tch9_sink_priv),
};
