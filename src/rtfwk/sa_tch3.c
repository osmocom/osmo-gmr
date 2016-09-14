/* GMR-1 RT framework: TCH3 task */

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
#include <osmocom/gmr1/l1/facch3.h>
#include <osmocom/gmr1/l1/tch3.h>
#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/dkab.h>
#include <osmocom/gmr1/sdr/pi4cxpsk.h>
#include <osmocom/gmr1/sdr/nb.h>

#include "common.h"
#include "sampbuf.h"
#include "sa_tch3.h"
#include "sa_tch9.h"


#define TCH3_MARGIN	10


struct tch3_sink_priv {
	/* App */
	struct app_state *as;
	int chan_id;

	/* Alignement time/freq */
	uint32_t fn;
	uint64_t align;
	int align_err;
	float freq_err;

	int tn;
	int dkab_pos;

	int aligned;

	/* Energy thresholds */
	float energy_dkab;
	float energy_burst;

	int weak_cnt;

	/* FAACH state */
	sbit_t ebits[104*4];
	uint32_t bi_fn[4];
	int sync_id;
	int burst_cnt;

	int followed;

	/* Cipher */
	int ciph;
	uint8_t kc[8];

	/* Output data */
	FILE *data;
};

static int
tch3_sink_init(struct sample_actor *sa, void *params_ptr)
{
	struct tch3_sink_priv *priv = sa->priv;
	struct tch3_sink_params *params = params_ptr;

	priv->as = params->as;
	priv->chan_id = params->chan_id;

	priv->fn = params->fn;
	priv->align = params->align;
	priv->freq_err = params->freq_err;

	priv->tn = params->tn;
	priv->dkab_pos = params->dkab_pos;

	priv->energy_burst = params->ref_energy * 0.75f;
	priv->energy_dkab  = priv->energy_burst / 8.0f; /* ~ 8 times less pwr */

	priv->weak_cnt = 0;

	priv->ciph = 0;

	return 0;
}

static void
tch3_sink_fini(struct sample_actor *sa)
{
	struct tch3_sink_priv *priv = sa->priv;

	if (priv->data)
		fclose(priv->data);
}

static int
_rx_tch3_dkab(struct sample_actor *sa, struct osmo_cxvec *burst, float *toa)
{
	struct tch3_sink_priv *priv = sa->priv;
	sbit_t ebits[8];
	int rv;

	fprintf(stderr, "[.]   DKAB\n");

	rv = gmr1_dkab_demod(burst, priv->as->sps, -priv->freq_err, priv->dkab_pos, ebits, toa);

	fprintf(stderr, "toa=%f\n", *toa);

	return rv;
}

static inline int
_facch3_is_ass_cmd_1(const uint8_t *l2)
{
	return (l2[3] == 0x06) && (l2[4] == 0x2e);
}

static void
_facch3_ass_cmd_1_parse(const uint8_t *l2, int *arfcn, int *rx_tn)
{
	*rx_tn = ((l2[5] & 0x03) << 3) | (l2[6] >> 5);
	*arfcn = ((l2[6] & 0x1f) << 6) | (l2[7] >> 2);
}

static int
_rx_tch3_facch_flush(struct sample_actor *sa)
{
	struct tch3_sink_priv *priv = sa->priv;
	ubit_t ciph[96*4];
	uint8_t l2[10];
	ubit_t sbits[8*4];
	int sps, base_align;
	int i, crc, conv;

	/* Get quick params */
	sps = priv->as->sps;
	base_align = sps * TCH3_MARGIN;

	/* Cipher stream */
	for (i=0; i<4; i++)
		gmr1_a5(priv->ciph, priv->kc, priv->bi_fn[i], 96, ciph+(96*i), NULL);

	/* Decode the burst */
	crc = gmr1_facch3_decode(l2, sbits, priv->ebits, ciph, &conv);

	fprintf(stderr, "crc=%d, conv=%d\n", crc, conv);

	/* Retry with ciphering ? */
#if 0
	if (!st->ciph && crc) {
		ciph = _ciph;
		for (i=0; i<4; i++)
			gmr1_a5(1, cd->kc, st->bi_fn[i], 96, ciph+(96*i), NULL);

		crc = gmr1_facch3_decode(l2, sbits, st->ebits, ciph, &conv);

		fprintf(stderr, "crc=%d, conv=%d\n", crc, conv);

		if (!crc)
			st->ciph = 1;
	}
#endif

	/* Send to GSMTap if correct */
	if (!crc)
		gsmtap_sendmsg(priv->as->gti, gmr1_gsmtap_makemsg(
			GSMTAP_GMR1_TCH3 | GSMTAP_GMR1_FACCH,
			priv->as->chans[priv->chan_id].arfcn,
			priv->fn-3, priv->tn, l2, 10));

	/* Parse for assignement */
	if (!crc && _facch3_is_ass_cmd_1(l2) && !priv->followed)
	{
		struct tch9_sink_params p;
		struct sample_actor *nsa;
		int arfcn, tn;
		int i;

		/* Extract TN & ARFCN */
		_facch3_ass_cmd_1_parse(l2, &arfcn, &tn);

		/* Debug print */
		fprintf(stderr, "[+] TCH9 assigned on ARFCN %d TN %d\n",
			arfcn, tn);

		/* Find matching channel ID */
		for (i=0; i<priv->as->n_chans; i++)
			if (priv->as->chans[i].arfcn == arfcn)
				break;

		if (i == priv->as->n_chans) {
			fprintf(stderr, "No data stream available for that ARFCN\n");
			goto nofollow;
		}

		/* Start TCH9 task */
		p.as       = priv->as;
		p.chan_id  = i;
		p.fn       = priv->fn;
		p.align    = sa->time + base_align;
		p.freq_err = priv->freq_err;
		p.tn       = tn;
		p.ciph     = priv->ciph;
		memcpy(p.kc, priv->kc, 8);

		nsa = sbuf_add_consumer(priv->as->buf, p.chan_id, &tch9_sink, &p);
		if (!nsa) {
			fprintf(stderr, "[!] Failed to create TCH9 sink for stream #%d\n", p.chan_id);
			return -ENOMEM;
		}

		/* Stop current TCH3 task */
			/* FIXME should only happen later, ass message spans several
			 * FACCH3 ... */
		priv->followed = 1;
	}
nofollow:

	/* Clear state */
	priv->sync_id ^= 1;
	priv->burst_cnt = 0;
	memset(priv->bi_fn, 0xff, sizeof(uint32_t) * 4);
	memset(priv->ebits, 0x00, sizeof(sbit_t) * 104 * 4);

	/* Done */
	return 0;
}

static int
_rx_tch3_facch(struct sample_actor *sa, struct osmo_cxvec *burst, float *toa)
{
	struct tch3_sink_priv *priv = sa->priv;
	sbit_t ebits[104];
	int rv, bi, sync_id;

	/* Burst index */
	bi = priv->fn & 3;

	/* Debug */
	fprintf(stderr, "[.]   FACCH3 (bi=%d)\n", bi);

	/* Demodulate burst */
	rv = gmr1_pi4cxpsk_demod(
		&gmr1_nt3_facch_burst,
		burst, priv->as->sps, -priv->freq_err,
		ebits, &sync_id, toa, NULL
	);

	fprintf(stderr, "toa=%.1f, sync_id=%d\n", *toa, sync_id);

	/* Does this burst belong with previous ones ? */
	if (sync_id != priv->sync_id)
		_rx_tch3_facch_flush(sa);

	/* Store this burst */
	memcpy(&priv->ebits[104*bi], ebits, sizeof(sbit_t) * 104);
	priv->sync_id = sync_id;
	priv->bi_fn[bi] = priv->fn;
	priv->burst_cnt += 1;

	/* Is it time to flush ? */
	if (priv->burst_cnt == 4)
		_rx_tch3_facch_flush(sa);

	return rv;
}

static int
_rx_tch3_speech(struct sample_actor *sa, struct osmo_cxvec *burst, float *toa)
{
	struct tch3_sink_priv *priv = sa->priv;
	sbit_t ebits[212];
	ubit_t sbits[4], ciph[208];
	uint8_t frame0[10], frame1[10];
	int rv, conv[2];

	/* Debug */
	fprintf(stderr, "[.]   TCH3\n");

	/* Demodulate burst */
	rv = gmr1_pi4cxpsk_demod(
		&gmr1_nt3_speech_burst,
		burst, priv->as->sps, -priv->freq_err,
		ebits, NULL, toa, NULL
	);
	if (rv < 0)
		return rv;

	/* Decode it */
	gmr1_a5(priv->ciph, priv->kc, priv->fn, 208, ciph, NULL);

	gmr1_tch3_decode(frame0, frame1, sbits, ebits, ciph, 0, &conv[0], &conv[1]);

	/* More debug */
	fprintf(stderr, "toa=%.1f\n", *toa);
	fprintf(stderr, "conv=%3d,%3d\n", conv[0], conv[1]);
	fprintf(stderr, "frame0=%s\n", osmo_hexdump_nospc(frame0, 10));
	fprintf(stderr, "frame1=%s\n", osmo_hexdump_nospc(frame1, 10));

	/* Save to file */
	{
		if (!priv->data) {
			char fname[256];
			sprintf(fname, "/tmp/gmr1_speech_%d_%d_%d.dat", priv->as->chans[priv->chan_id].arfcn, priv->tn, priv->fn);
			priv->data = fopen(fname, "wb");
		}
		fwrite(frame0, 10, 1, priv->data);
		fwrite(frame1, 10, 1, priv->data);
	}

	return 0;
}

static int
tch3_sink_work(struct sample_actor *sa,
               float complex *data, unsigned int data_len)
{
	static struct gmr1_pi4cxpsk_burst *burst_types[] = {
		&gmr1_nt3_facch_burst,
		&gmr1_nt3_speech_burst,
		NULL
	};

	struct tch3_sink_priv *priv = sa->priv;
	struct osmo_cxvec _burst, *burst = &_burst;
	int sps, base_align, frame_len;
	int e_toa, btid, sid;
	float be, det, toa;
	int rv;

	/* Get quick params */
	sps = priv->as->sps;
	frame_len = sps * 39 * 24;
	base_align = sps * TCH3_MARGIN;

	/* If not aligned ... do that first */
	if (!priv->aligned) {
		/* Basically we want to have :
		 * |#|Frame|#| with the # being margin.
		 */

		uint64_t target = priv->align - sps * TCH3_MARGIN;
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
	if (data_len < (frame_len + 2*TCH3_MARGIN))
		return 0;

	/* Map potential burst (use FACCH3 as reference) */
	e_toa = burst_map(burst, data, data_len, base_align, sps,
	                  &gmr1_nt3_facch_burst, priv->tn, sps + sps/2);
	if (e_toa < 0)
		return e_toa;

	/* Burst energy (and check for DKAB) */
	be = burst_energy(burst);

	det = (priv->energy_dkab + priv->energy_burst) / 4.0f;

	if (be < det) {
		rv = _rx_tch3_dkab(sa, burst, &toa);

		if (rv < 0)
			return rv;
		else if (rv == 1) {
			if (priv->weak_cnt++ > 8) {
				fprintf(stderr, "END @%d\n", priv->fn);
				return -1;
			}
		} else {
			priv->energy_dkab =
				(0.1f * be) +
				(0.9f * priv->energy_dkab);
		}

		goto done;
	} else
		priv->weak_cnt = 0;

	priv->energy_burst =
		(0.1f * be) +
		(0.9f * priv->energy_burst);

	/* Detect burst type */
	rv = gmr1_pi4cxpsk_detect(
		burst_types, (float)e_toa,
		burst, sps, -priv->freq_err,
		&btid, &sid, &toa
	);
	if (rv < 0)
		return rv;

	/* Delegate appropriately */
	if (btid == 0)
		rv = _rx_tch3_facch(sa, burst, &toa);
	else
		rv = _rx_tch3_speech(sa, burst, &toa);

	if (rv < 0)
		return rv;

done:
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

const struct sample_actor_desc tch3_sink = {
	.init = tch3_sink_init,
	.fini = tch3_sink_fini,
	.work = tch3_sink_work,
	.priv_size = sizeof(struct tch3_sink_priv),
};
