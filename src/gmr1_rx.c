/* GMR-1 Demo RX application */

/* (C) 2011 by Sylvain Munaut <tnt@246tNt.com>
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
#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>

#include <osmocom/sdr/cfile.h>
#include <osmocom/sdr/cxvec.h>
#include <osmocom/sdr/cxvec_math.h>

#include <osmocom/gmr1/gsmtap.h>
#include <osmocom/gmr1/l1/bcch.h>
#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/fcch.h>
#include <osmocom/gmr1/sdr/pi4cxpsk.h>
#include <osmocom/gmr1/sdr/nb.h>


#define START_DISCARD	8000


static struct gsmtap_inst *g_gti;


struct chan_desc {
	struct cfile *bcch;
	int sps;
	int align;
	float freq_err;
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
          struct gmr1_pi4cxpsk_burst *burst_type, int tn, int win)
{
	int begin, len;
	int etoa;

	etoa  = win >> 1;
	begin = cd->align + (cd->sps * tn * 39) - etoa;
	len   = (burst_type->len * cd->sps) + win;

	if ((begin + len) > cd->bcch->len)
		return -1;

	osmo_cxvec_init_from_data(burst, &cd->bcch->data[begin], len);

	return etoa;
}

static float
burst_energy(struct osmo_cxvec *burst)
{
	int i;
	float e = 0.0f;
	for (i=0; i<burst->len; i++)
		e += osmo_normsqf(burst->data[i]);
	e /= burst->len;
	return e;
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

	rv = gmr1_fcch_rough(win, cd->sps, 0.0f, &toa);
	if (rv) {
		fprintf(stderr, "[!] Error during FCCH rough acquisition (%d)\n", rv);
		return rv;
	}

	cd->align += toa;

	/* Fine FCCH detection*/
	win_map(win, cd->bcch, cd->align, GMR1_FCCH_SYMS * cd->sps);

	rv = gmr1_fcch_fine(win, cd->sps, 0.0f, &toa, &cd->freq_err);
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
	base_align = cd->align - GMR1_FCCH_SYMS * cd->sps;

	rv = win_map(win, cd->bcch, base_align, (650 * GMR1_SYM_RATE * cd->sps) / 1000);
	if (rv) {
		fprintf(stderr, "[!] Not enough samples\n");
		return rv;
	}

	rv = gmr1_fcch_rough_multi(win, cd->sps, -cd->freq_err, mtoa, 16);
	if (rv < 0) {
		fprintf(stderr, "[!] Error during FCCH rough mutli-acquisition (%d)\n", rv);
		return rv;
	}

	n_fcch = rv;

	/* Check each of them for validity */
	for (i=0, j=0; i<n_fcch; i++) {
		float freq_err, e_fcch, e_cich, snr;
		int toa;

		/* Perform fine acquisition */
		win_map(win, cd->bcch, base_align + mtoa[i], GMR1_FCCH_SYMS * cd->sps);

		rv = gmr1_fcch_fine(win, cd->sps, -cd->freq_err, &toa, &freq_err);
		if (rv) {
			fprintf(stderr, "[!] Error during FCCH fine acquisition (%d)\n", rv);
			return rv;
		}

		/* Compute SNR (comparing energy with neighboring CICH) */
		win_map(win, cd->bcch,
			base_align + mtoa[i] + toa + 5 * cd->sps,
			(117 - 10) * cd->sps
		);

		e_fcch = burst_energy(win);

		win_map(win, cd->bcch,
			base_align + mtoa[i] + toa + (5 + 117) * cd->sps,
			(117 - 10) * cd->sps
		);

		e_cich = burst_energy(win);

		snr = e_fcch / e_cich;

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
rx_bcch(struct chan_desc *cd)
{
	struct osmo_cxvec _burst, *burst = &_burst;
	sbit_t ebits[424];
	uint8_t l2[24];
	float freq_err, toa;
	int rv, crc, conv, e_toa;

	/* Demodulate burst */
	e_toa = burst_map(burst, cd, &gmr1_bcch_burst, 0, 20 * cd->sps);
	if (e_toa < 0)
		return e_toa;

	rv = gmr1_pi4cxpsk_demod(
		&gmr1_bcch_burst,
		burst, cd->sps, -cd->freq_err,
		ebits, NULL, &toa, &freq_err
	);

	if (rv)
		return rv;

	/* Decode burst */
	crc = gmr1_bcch_decode(l2, ebits, &conv);

	fprintf(stderr, "crc=%d, conv=%d\n", crc, conv);

	/* If burst turned out OK, use data to align channel */
	if (!crc) {
		cd->align += ((int)roundf(toa)) - e_toa;
		cd->freq_err += freq_err / 4.0f;
	}

	/* Send to GSMTap if correct */
	if (!crc)
		gsmtap_sendmsg(g_gti, gmr1_gsmtap_makemsg(GSMTAP_GMR1_BCCH, l2, 24));

	return 0;
}

static int
process_bcch(struct chan_desc *cd)
{
	int frame_len;
	int rfn, sirfn;

	fprintf(stderr, "[+] Processing BCCH @%d (%.3f ms). [freq_err = %.1f Hz]\n",
		cd->align, to_ms(cd, cd->align), to_hz(cd->freq_err));

	/* Process frame by frame */
	frame_len = cd->sps * 24 * 39;
	rfn = 0;

	while (1) {
		/* Debug */
		fprintf(stderr, "[-]  RFN: %d (%f ms)\n", rfn, to_ms(cd, cd->align));

		/* SI relative frame number inside an hyperframe */
		sirfn = rfn % 64;

		/* BCCH */
		if (sirfn % 8 == 2)
		{
			fprintf(stderr, "[.]   BCCH\n");
			rx_bcch(cd);
		}

		/* Next frame */
		rfn++;
		cd->align += frame_len;

		/* Stop if we don't have a complete frame */
		if ((cd->align + frame_len) > cd->bcch->len)
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
	cd->bcch = NULL;
	cd->align = START_DISCARD;
	cd->freq_err = 0.0f;

	/* Arg check */
	if (argc != 3) {
		fprintf(stderr, "Usage: %s sps bcch.cfile\n", argv[0]);
		return -EINVAL;
	}

	cd->sps = atoi(argv[1]);

	if (cd->sps < 1 || cd->sps > 16) {
		fprintf(stderr, "[!] sps must be withing [1,16]\n");
		return -EINVAL;
	}

	cd->bcch = cfile_load(argv[2]);
	if (!cd->bcch) {
		fprintf(stderr, "[!] Failed to load fcch input file\n");
		rv = -EIO;
		goto err;
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
	if (cd->bcch)
		cfile_release(cd->bcch);

	return rv;
}
