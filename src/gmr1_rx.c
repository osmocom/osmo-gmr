/* GMR-1 Demo RX application */

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
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <osmocom/core/bits.h>
#include <osmocom/core/utils.h>
#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>

#include <osmocom/dsp/cfile.h>
#include <osmocom/dsp/cxvec.h>
#include <osmocom/dsp/cxvec_math.h>

#include <osmocom/gmr1/gsmtap.h>
#include <osmocom/gmr1/l1/a5.h>
#include <osmocom/gmr1/l1/bcch.h>
#include <osmocom/gmr1/l1/ccch.h>
#include <osmocom/gmr1/l1/facch3.h>
#include <osmocom/gmr1/l1/facch9.h>
#include <osmocom/gmr1/l1/interleave.h>
#include <osmocom/gmr1/l1/tch3.h>
#include <osmocom/gmr1/l1/tch9.h>
#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/dkab.h>
#include <osmocom/gmr1/sdr/fcch.h>
#include <osmocom/gmr1/sdr/pi4cxpsk.h>
#include <osmocom/gmr1/sdr/nb.h>


#define START_DISCARD	8000


static struct gsmtap_inst *g_gti;

static const struct gmr1_fcch_burst *fcch_type = &gmr1_fcch_burst;


struct tch3_state {
	/* Status */
	int active;

	/* Channel params */
	int tn;
	int p;
	int ciph;

	/* Energy */
	float energy_dkab;
	float energy_burst;

	int weak_cnt;

	/* FACCH state */
	sbit_t ebits[104*4];
	uint32_t bi_fn[4];
	int sync_id;
	int burst_cnt;
};

struct tch9_state {
	/* Status */
	int active;

	/* Channel params */
	int tn;

	/* Interleaver */
	struct gmr1_interleaver il;
};

struct chan_desc {
	/* Sample source */
	struct cfile *bcch;
	struct cfile *tch;
	struct cfile *tch_csd;
	int sps;

	/* SDR alignement */
	int align;
	float freq_err;

	/* TDMA alignement */
	int fn;
	int sa_sirfn_delay;
	int sa_bcch_stn;

	/* TCH */
	struct tch3_state tch3_state;
	struct tch9_state tch9_state;

	/* A5 */
	uint8_t kc[8];
};


/* Helpers ---------------------------------------------------------------- */

static inline float
to_ms(struct chan_desc *cd, int s)
{
	return (1000.0f * (float)s) / (cd->sps * GMR1_SYM_RATE);
}

static inline float
to_hz(float f_rps)
{
	return (GMR1_SYM_RATE * f_rps) / (2.0f * M_PIf);
}

static inline float
to_db(float v)
{
	return 10.0f * log10f(v);
}

static int
win_map(struct osmo_cxvec *win, struct cfile *cf, int begin, int len)
{
	if ((begin + len) > cf->len)
		return -1;

	osmo_cxvec_init_from_data(win, &cf->data[begin], len);

	return 0;
}

static int
burst_map(struct osmo_cxvec *burst, struct chan_desc *cd,
          struct gmr1_pi4cxpsk_burst *burst_type, int tn, int win, int tch)
{
	int begin, len;
	int etoa;
	struct cfile *df = tch == 2 ? cd->tch_csd : (tch ? cd->tch : cd->bcch);

	if (!df)
		return -EINVAL;

	etoa  = win >> 1;
	begin = cd->align + (cd->sps * tn * 39) - etoa;
	len   = (burst_type->len * cd->sps) + win;

	if ((begin + len) > cd->bcch->len)
		return -EIO;

	osmo_cxvec_init_from_data(burst, &df->data[begin], len);

	return etoa;
}

static float
burst_energy(struct osmo_cxvec *burst)
{
	int i;
	float e = 0.0f;
	int b = (burst->len >> 5); /* exclude the borders */
	for (i=b; i<burst->len-b; i++)
		e += osmo_normsqf(burst->data[i]);
	e /= burst->len;
	return e;
}


/* Message parsing -------------------------------------------------------- */

static int
bcch_tdma_align(struct chan_desc *cd, uint8_t *l2)
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
	cd->align += (cd->sa_bcch_stn - sa_bcch_stn) * 39 * cd->sps;

	/* Align TDMA */
	cd->fn = fn;
	cd->sa_sirfn_delay = sa_sirfn_delay;
	cd->sa_bcch_stn = sa_bcch_stn;

	return 0;
}

static inline int
ccch_is_imm_ass(const uint8_t *l2)
{
	return (l2[1] == 0x06) && (l2[2] == 0x3f);
}

static void
ccch_imm_ass_parse(const uint8_t *l2, int *rx_tn, int *p)
{
	*p = (l2[8] & 0xfc) >> 2;
	*rx_tn = ((l2[8] & 0x03) << 3) | (l2[9] >> 5);
}

static inline int
facch3_is_ass_cmd_1(const uint8_t *l2)
{
	return (l2[3] == 0x06) && (l2[4] == 0x2e);
}

static void
facch3_ass_cmd_1_parse(const uint8_t *l2, int *rx_tn)
{
	*rx_tn = ((l2[5] & 0x03) << 3) | (l2[6] >> 5);
}


/* TCH9 Procesing --------------------------------------------------------- */

static void
rx_tch9_init(struct chan_desc *cd, const uint8_t *ass_cmd)
{
	/* Activate */
	cd->tch9_state.active = 1;

	/* Extract TN */
	facch3_ass_cmd_1_parse(ass_cmd, &cd->tch9_state.tn);

	/* Init interleaver */
	gmr1_interleaver_init(&cd->tch9_state.il, 3, 648);
}

static int
rx_tch9(struct chan_desc *cd)
{
	struct osmo_cxvec _burst, *burst = &_burst;
	int e_toa, rv, sync_id, crc, conv;
	sbit_t ebits[662], bits_sacch[10], bits_status[4];
	ubit_t ciph[658];
	float toa;

	/* Is TCH active at all ? */
	if (!cd->tch9_state.active)
		return 0;

	/* Map potential burst */
	e_toa = burst_map(burst, cd, &gmr1_nt9_burst,
	                  cd->tch9_state.tn, cd->sps + (cd->sps/2), 2);
	if (e_toa < 0)
		return e_toa;

	/* Demodulate burst */
	rv = gmr1_pi4cxpsk_demod(
		&gmr1_nt9_burst,
		burst, cd->sps, -cd->freq_err,
		ebits, &sync_id, &toa, NULL
	);

	fprintf(stderr, "[.]   %s\n", sync_id ? "TCH9" : "FACCH9");
	fprintf(stderr, "toa=%.1f, sync_id=%d\n", toa, sync_id);

	/* Process depending on type */
	if (!sync_id) { /* FACCH9 */
		uint8_t l2[38];

		/* Generate cipher stream */
		gmr1_a5(1, cd->kc, cd->fn, 658, ciph, NULL);

		/* Decode */
		crc = gmr1_facch9_decode(l2, bits_sacch, bits_status, ebits, ciph, &conv);
		fprintf(stderr, "crc=%d, conv=%d\n", crc, conv);

		/* Send to GSMTap if correct */
		if (!crc)
			gsmtap_sendmsg(g_gti, gmr1_gsmtap_makemsg(
				GSMTAP_GMR1_TCH9 | GSMTAP_GMR1_FACCH,
				0, cd->fn, cd->tch9_state.tn, l2, 38));
	} else { /* TCH9 */
		uint8_t l2[60];
		int i, s = 0;

		/* Generate cipher stream */
		gmr1_a5(1, cd->kc, cd->fn, 658, ciph, NULL);

		for (i=0; i<662; i++)
			s += ebits[i] < 0 ? -ebits[i] : ebits[i];
		s /= 662;

		/* Decode */
		gmr1_tch9_decode(l2, bits_sacch, bits_status, ebits, GMR1_TCH9_9k6, ciph, &cd->tch9_state.il, &conv);
		fprintf(stderr, "fn=%d, conv9=%d, avg=%d\n", cd->fn, conv, s);

		/* Forward to GSMTap (no CRC to validate :( ) */
		gsmtap_sendmsg(g_gti, gmr1_gsmtap_makemsg(
			GSMTAP_GMR1_TCH9,
			0, cd->fn, cd->tch9_state.tn, l2, 60));

		/* Save to file */
		{
			static FILE *f = NULL;
			if (!f)
				f = fopen("/tmp/csd.data", "wb");
			fwrite(l2, 60, 1, f);
		}
	}

	/* Done */
	return rv;

}


/* TCH3 Procesing --------------------------------------------------------- */

static void
rx_tch3_init(struct chan_desc *cd, const uint8_t *imm_ass, float ref_energy)
{
	/* Activate */
	cd->tch3_state.active = 1;

	/* Extract TN & DKAB position */
	ccch_imm_ass_parse(imm_ass, &cd->tch3_state.tn, &cd->tch3_state.p);

	/* Estimate energy threshold */
	cd->tch3_state.energy_burst = ref_energy * 0.75f;
	cd->tch3_state.energy_dkab  = cd->tch3_state.energy_burst / 8.0f; /* ~ 8 times less pwr */

	cd->tch3_state.weak_cnt = 0;

	/* Init FACCH state */
	cd->tch3_state.sync_id = 0;
	memset(&cd->tch3_state.ebits, 0x00, sizeof(sbit_t) * 104 * 4);
}

static int
_rx_tch3_dkab(struct chan_desc *cd, struct osmo_cxvec *burst)
{
	sbit_t ebits[8];
	float toa;
	int rv;

	fprintf(stderr, "[.]   DKAB\n");

	rv = gmr1_dkab_demod(burst, cd->sps, -cd->freq_err, cd->tch3_state.p, ebits, &toa);

	fprintf(stderr, "toa=%f\n", toa);

	return rv;
}

static int
_rx_tch3_facch_flush(struct chan_desc *cd)
{
	struct tch3_state *st = &cd->tch3_state;
	ubit_t _ciph[96*4], *ciph;
	uint8_t l2[10];
	ubit_t sbits[8*4];
	int i, crc, conv;

	/* Cipher stream ? */
	if (st->ciph) {
		ciph = _ciph;
		for (i=0; i<4; i++)
			gmr1_a5(1, cd->kc, st->bi_fn[i], 96, ciph+(96*i), NULL);
	} else
		ciph = NULL;

	/* Decode the burst */
	crc = gmr1_facch3_decode(l2, sbits, st->ebits, ciph, &conv);

	fprintf(stderr, "crc=%d, conv=%d\n", crc, conv);

	/* Retry with ciphering ? */
	if (!st->ciph && crc) {
		ciph = _ciph;
		for (i=0; i<4; i++)
			gmr1_a5(1, cd->kc, st->bi_fn[i], 96, ciph+(96*i), NULL);

		crc = gmr1_facch3_decode(l2, sbits, st->ebits, ciph, &conv);

		fprintf(stderr, "crc=%d, conv=%d\n", crc, conv);

		if (!crc)
			st->ciph = 1;
	}

	/* Send to GSMTap if correct */
	if (!crc)
		gsmtap_sendmsg(g_gti, gmr1_gsmtap_makemsg(
			GSMTAP_GMR1_TCH3 | GSMTAP_GMR1_FACCH,
			0, cd->fn-3, st->tn, l2, 10));

	/* Parse for assignement */
	if (!crc && facch3_is_ass_cmd_1(l2))
	{
		/* Follow if we have the data */
		if (cd->tch_csd)
			rx_tch9_init(cd, l2);
	}

	/* Clear state */
	st->sync_id ^= 1;
	st->burst_cnt = 0;
	memset(st->bi_fn, 0xff, sizeof(uint32_t) * 4);
	memset(st->ebits, 0x00, sizeof(sbit_t) * 104 * 4);

	/* Done */
	return 0;
}

static int
_rx_tch3_facch(struct chan_desc *cd, struct osmo_cxvec *burst)
{
	struct tch3_state *st = &cd->tch3_state;
	sbit_t ebits[104];
	int rv, bi, sync_id;
	float toa;

	/* Burst index */
	bi = cd->fn & 3;

	/* Debug */
	fprintf(stderr, "[.]   FACCH3 (bi=%d)\n", bi);

	/* Demodulate burst */
	rv = gmr1_pi4cxpsk_demod(
		&gmr1_nt3_facch_burst,
		burst, cd->sps, -cd->freq_err,
		ebits, &sync_id, &toa, NULL
	);
	if (rv < 0)
		return rv;

	fprintf(stderr, "toa=%.1f, sync_id=%d\n", toa, sync_id);

	/* Does this burst belong with previous ones ? */
	if (sync_id != st->sync_id)
		_rx_tch3_facch_flush(cd);

	/* Store this burst */
	memcpy(&st->ebits[104*bi], ebits, sizeof(sbit_t) * 104);
	st->sync_id = sync_id;
	st->bi_fn[bi] = cd->fn;
	st->burst_cnt += 1;

	/* Is it time to flush ? */
	if (st->burst_cnt == 4)
		_rx_tch3_facch_flush(cd);

	return 0;
}

static int
_rx_tch3_speech(struct chan_desc *cd, struct osmo_cxvec *burst)
{
	sbit_t ebits[212];
	ubit_t sbits[4], ciph[208];
	uint8_t frame0[10], frame1[10];
	int rv, conv[2];
	float toa;

	/* Debug */
	fprintf(stderr, "[.]   TCH3\n");

	/* Demodulate burst */
	rv = gmr1_pi4cxpsk_demod(
		&gmr1_nt3_speech_burst,
		burst, cd->sps, -cd->freq_err,
		ebits, NULL, &toa, NULL
	);
	if (rv < 0)
		return rv;

	/* Decode it */
	gmr1_a5(cd->tch3_state.ciph, cd->kc, cd->fn, 208, ciph, NULL);

	gmr1_tch3_decode(frame0, frame1, sbits, ebits, ciph, 0, &conv[0], &conv[1]);

	/* More debug */
	fprintf(stderr, "toa=%.1f\n", toa);
	fprintf(stderr, "conv=%3d,%3d\n", conv[0], conv[1]);
	fprintf(stderr, "frame0=%s\n", osmo_hexdump_nospc(frame0, 10));
	fprintf(stderr, "frame1=%s\n", osmo_hexdump_nospc(frame1, 10));

	return 0;
}

static int
rx_tch3(struct chan_desc *cd)
{
	static struct gmr1_pi4cxpsk_burst *burst_types[] = {
		&gmr1_nt3_facch_burst,
		&gmr1_nt3_speech_burst,
		NULL
	};

	struct osmo_cxvec _burst, *burst = &_burst;
	int e_toa, rv, btid, sid;
	float be, det, toa;

	/* Is TCH active at all ? */
	if (!cd->tch3_state.active)
		return 0;

	/* Map potential burst (use FACCH3 as reference) */
	e_toa = burst_map(burst, cd, &gmr1_nt3_facch_burst,
	                  cd->tch3_state.tn, cd->sps + (cd->sps/2), 1);
	if (e_toa < 0)
		return e_toa;

	/* Burst energy (and check for DKAB) */
	be = burst_energy(burst);

	det = (cd->tch3_state.energy_dkab + cd->tch3_state.energy_burst) / 4.0f;

	if (be < det) {
		rv = _rx_tch3_dkab(cd, burst);

		if (rv < 0)
			return rv;
		else if (rv == 1) {
			if (cd->tch3_state.weak_cnt++ > 8) {
				fprintf(stderr, "END @%d\n", cd->fn);
				cd->tch3_state.active = 0;
			}
		} else {
			cd->tch3_state.energy_dkab =
				(0.1f * be) +
				(0.9f * cd->tch3_state.energy_dkab);
		}

		return 0;
	} else
		cd->tch3_state.weak_cnt = 0;

	cd->tch3_state.energy_burst =
		(0.1f * be) +
		(0.9f * cd->tch3_state.energy_burst);

	/* Detect burst type */
	rv = gmr1_pi4cxpsk_detect(
		burst_types, (float)e_toa,
		burst, cd->sps, -cd->freq_err,
		&btid, &sid, &toa
	);
	if (rv < 0)
		return rv;

	/* Delegate appropriately */
	if (btid == 0)
		rv = _rx_tch3_facch(cd, burst);
	else
		rv = _rx_tch3_speech(cd, burst);

	/* Done */
	return rv;
}


/* Procesing -------------------------------------------------------------- */

static int
fcch_single_init(struct chan_desc *cd)
{
	struct osmo_cxvec _win, *win = &_win;
	int rv, toa;

	/* FCCH rough detection in the first 330 ms */
	rv = win_map(win, cd->bcch, cd->align, (330 * GMR1_SYM_RATE * cd->sps) / 1000);
	if (rv) {
		fprintf(stderr, "[!] Not enough samples\n");
		return rv;
	}

	rv = gmr1_fcch_rough(fcch_type, win, cd->sps, 0.0f, &toa);
	if (rv) {
		fprintf(stderr, "[!] Error during FCCH rough acquisition (%d)\n", rv);
		return rv;
	}

	cd->align += toa;

	/* Fine FCCH detection*/
	win_map(win, cd->bcch, cd->align, fcch_type->len * cd->sps);

	rv = gmr1_fcch_fine(fcch_type, win, cd->sps, 0.0f, &toa, &cd->freq_err);
	if (rv) {
		fprintf(stderr, "[!] Error during FCCH fine acquisition (%d)\n", rv);
		return rv;
	}

	cd->align += toa;

	/* Done */
	return 0;
}

typedef int (*fcch_multi_cb_t)(struct chan_desc *cd);

static int
fcch_multi_process(struct chan_desc *cd, fcch_multi_cb_t cb)
{
	struct osmo_cxvec _win, *win = &_win;
	int base_align, mtoa[16];
	int i, j, rv, n_fcch;
	float ref_snr, ref_freq_err;

	fprintf(stderr, "[+] FCCH multi acquisition\n");

	/* Multi FCCH detection (need 650 ms of signals) */
	base_align = cd->align - fcch_type->len * cd->sps;
	if (base_align < 0)
		base_align = 0;

	rv = win_map(win, cd->bcch, base_align, (650 * GMR1_SYM_RATE * cd->sps) / 1000);
	if (rv) {
		fprintf(stderr, "[!] Not enough samples\n");
		return rv;
	}

	rv = gmr1_fcch_rough_multi(fcch_type, win, cd->sps, -cd->freq_err, mtoa, 16);
	if (rv < 0) {
		fprintf(stderr, "[!] Error during FCCH rough mutli-acquisition (%d)\n", rv);
		return rv;
	}

	n_fcch = rv;

	/* Check each of them for validity */
	ref_snr = ref_freq_err = 0.0f;

	for (i=0, j=0; i<n_fcch; i++) {
		float freq_err, e_fcch, e_cich, snr;
		int toa;

		/* Perform fine acquisition */
		win_map(win, cd->bcch, base_align + mtoa[i], fcch_type->len * cd->sps);

		rv = gmr1_fcch_fine(fcch_type, win, cd->sps, -cd->freq_err, &toa, &freq_err);
		if (rv) {
			fprintf(stderr, "[!] Error during FCCH fine acquisition (%d)\n", rv);
			return rv;
		}

		/* Compute SNR */
		win_map(win, cd->bcch,
			base_align + mtoa[i] + toa,
			fcch_type->len * cd->sps
		);

		rv = gmr1_fcch_snr(fcch_type, win, cd->sps, -(cd->freq_err + freq_err), &snr);
		if (rv) {
			fprintf(stderr, "[!] Error during FCCH SNR estimation (%d)\n", rv);
		}

		/* Check against strongest */
		if (i==0) {
			/* This _is_ the reference */
			ref_snr = snr;
			ref_freq_err = freq_err;
		} else {
			/* Check if SNR is 'good enough' */
			if (snr < 2.0f)
				continue;

			if (snr < (ref_snr / 6.0f))
				continue;

			/* Check if frequency error is not too "off" */
			if (to_hz(fabs(ref_freq_err - freq_err)) > 500.0f)
				continue;
		}

		/* Debug print */
		fprintf(stderr, "[.]  Potential FCCH @%d (%.3f ms). [snr = %.1f dB, freq_err = %.1f Hz]\n",
			base_align + mtoa[i] + toa,
			to_ms(cd, base_align + mtoa[i] + toa),
			to_db(snr),
			to_hz(freq_err + cd->freq_err)
		);

		/* Save it */
		mtoa[j++] = mtoa[i] + toa;
	}

	n_fcch = j;

	/* Now process each survivor */
	for (i=0; i<n_fcch; i++) {
		struct chan_desc _cdl, *cdl = &_cdl;

		memcpy(cdl, cd, sizeof(struct chan_desc));
		cdl->align = base_align + mtoa[i];

		rv = cb(cdl);
		if (rv)
			break;
	}

	return rv;
}

static int
rx_bcch(struct chan_desc *cd, float *energy)
{
	struct osmo_cxvec _burst, *burst = &_burst;
	sbit_t ebits[424];
	uint8_t l2[24];
	float freq_err, toa;
	int rv, crc, conv, e_toa;

	/* Debug */
	fprintf(stderr, "[.]   BCCH\n");

	/* Demodulate burst */
	e_toa = burst_map(burst, cd, &gmr1_bcch_burst, cd->sa_bcch_stn, 20 * cd->sps, 0);
	if (e_toa < 0)
		return e_toa;

	rv = gmr1_pi4cxpsk_demod(
		&gmr1_bcch_burst,
		burst, cd->sps, -cd->freq_err,
		ebits, NULL, &toa, &freq_err
	);

	if (rv)
		return rv;

	/* Measure energy as a reference */
	if (energy)
		*energy = burst_energy(burst);

	/* Decode burst */
	crc = gmr1_bcch_decode(l2, ebits, &conv);

	fprintf(stderr, "crc=%d, conv=%d\n", crc, conv);

	/* If burst turned out OK, use data to align channel */
	if (!crc) {
		/* SDR alignement */
		cd->align += ((int)roundf(toa)) - e_toa;
		cd->freq_err += freq_err;

		/* Acquire TDMA alignement */
		bcch_tdma_align(cd, l2);
	}

	/* Send to GSMTap if correct */
	if (!crc)
		gsmtap_sendmsg(g_gti, gmr1_gsmtap_makemsg(
			GSMTAP_GMR1_BCCH,
			0, cd->fn, cd->sa_bcch_stn, l2, 24));

	return 0;
}

static int
rx_ccch(struct chan_desc *cd, float min_energy)
{
	struct osmo_cxvec _burst, *burst = &_burst;
	sbit_t ebits[432];
	uint8_t l2[24];
	int rv, crc, conv, e_toa;

	/* Map potential burst */
	e_toa = burst_map(burst, cd, &gmr1_dc6_burst, cd->sa_bcch_stn, 10 * cd->sps, 0);
	if (e_toa < 0)
		return e_toa;

	/* Energy detection */
	if (burst_energy(burst) < min_energy)
		return 0; /* Nothing to do */

	/* Debug */
	fprintf(stderr, "[.]   CCCH\n");

	/* Demodulate burst */
	rv = gmr1_pi4cxpsk_demod(
		&gmr1_dc6_burst,
		burst, cd->sps, -cd->freq_err,
		ebits, NULL, NULL, NULL
	);

	if (rv)
		return rv;

	/* Decode burst */
	crc = gmr1_ccch_decode(l2, ebits, &conv);

	fprintf(stderr, "crc=%d, conv=%d\n", crc, conv);

	/* Check for IMM.ASS */
	if (!crc) {
		if (ccch_is_imm_ass(l2)) {
			rx_tch3_init(cd, l2, min_energy);
			fprintf(stderr, "\n[+] TCH3 assigned on TN %d\n", cd->tch3_state.tn);
		}
	}

	/* Send to GSMTap if correct */
	if (!crc)
		gsmtap_sendmsg(g_gti, gmr1_gsmtap_makemsg(
			GSMTAP_GMR1_CCCH,
			0, cd->fn, cd->sa_bcch_stn, l2, 24));

	return 0;
}

static int
process_bcch(struct chan_desc *cd)
{
	int frame_len;
	int sirfn;
	float bcch_energy = nan("inf");

	fprintf(stderr, "[+] Processing BCCH @%d (%.3f ms). [freq_err = %.1f Hz]\n",
		cd->align, to_ms(cd, cd->align), to_hz(cd->freq_err));

	/* Process frame by frame */
	frame_len = cd->sps * 24 * 39;

	while (1) {
		/* Debug */
		fprintf(stderr, "[-]  FN: %6d (%10.3f ms)\n", cd->fn, to_ms(cd, cd->align));

		/* SI relative frame number inside an hyperframe */
		sirfn = (cd->fn - cd->sa_sirfn_delay) & 63;

		/* BCCH */
		if (sirfn % 8 == 2)
			rx_bcch(cd, &bcch_energy);

		/* CCCH */
		if ((sirfn % 8 != 0) && (sirfn % 8 != 2))
			rx_ccch(cd, bcch_energy / 2.0f);

		/* TCH */
		rx_tch3(cd);
		rx_tch9(cd);

		/* Next frame */
		cd->fn++;
		cd->align += frame_len;

		/* Stop if we don't have 2 complete frame
		 * (with TN offset, we can go beyond one) */
		if ((cd->align + 2*frame_len) > cd->bcch->len)
			break;
	}

	return 0;
}


/* Main ------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	struct chan_desc _cd, *cd = &_cd;
	int rv=0;

	/* Init channel description */
	memset(cd, 0x00, sizeof(struct chan_desc));

	cd->align = START_DISCARD;
	cd->freq_err = 0.0f;

	/* Arg check */
	if (argc < 3 || argc > 6) {
		fprintf(stderr, "Usage: %s sps bcch.cfile [tch.cfile [key [tch_csd.cfile]]]\n", argv[0]);
		return -EINVAL;
	}

	cd->sps = atoi(argv[1]);

	if (cd->sps < 1 || cd->sps > 16) {
		fprintf(stderr, "[!] sps must be within [1,16]\n");
		return -EINVAL;
	}

	cd->bcch = cfile_load(argv[2]);
	if (!cd->bcch) {
		fprintf(stderr, "[!] Failed to load bcch input file\n");
		rv = -EIO;
		goto err;
	}

	if (argc > 3) {
		cd->tch = cfile_load(argv[3]);
		if (!cd->tch) {
			fprintf(stderr, "[!] Failed to load tch input file\n");
			rv = -EIO;
			goto err;
		}
	}

	if (argc > 4) {
		if (osmo_hexparse(argv[4], cd->kc, 8) != 8) {
			fprintf(stderr, "[!] Invalid key\n");
			rv = -EINVAL;
			goto err;
		}
	}

	if (argc > 5) {
		cd->tch_csd = cfile_load(argv[5]);
		if (!cd->tch_csd) {
			fprintf(stderr, "[!] Failed to load tch CSD input file\n");
			rv = -EIO;
			goto err;
		}
	}

	/* Init GSMTap */
	g_gti = gsmtap_source_init("127.0.0.1", GSMTAP_UDP_PORT, 0);
	gsmtap_source_add_sink(g_gti);

	/* Use best FCCH for inital sync / freq error */
	rv = fcch_single_init(cd);
	if (rv) {
		fprintf(stderr, "[!] Failed to acquired primary FCCH\n");
		goto err;
	}

	fprintf(stderr, "[+] Primary FCCH found @%d (%.3f ms). [freq_err = %.1f Hz]\n",
		cd->align, to_ms(cd, cd->align), to_hz(cd->freq_err));

	/* Detect all 'visible' FCCH and process them */
	rv = fcch_multi_process(cd, process_bcch);
	if (rv)
		goto err;

	/* Done ! */
	rv = 0;

	/* Clean up */
err:
	if (cd->tch_csd)
		cfile_release(cd->tch_csd);

	if (cd->tch)
		cfile_release(cd->tch);

	if (cd->bcch)
		cfile_release(cd->bcch);

	return rv;
}
