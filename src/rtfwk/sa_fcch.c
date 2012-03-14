/* GMR-1 RT framework: FCCH task */

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
#include <stdio.h>

#include <osmocom/dsp/cxvec.h>

#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/fcch.h>

#include "common.h"
#include "sampbuf.h"
#include "sa_fcch.h"
#include "sa_bcch_ccch.h"


struct fcch_sink_priv {
	struct app_state *as;
	int chan_id;

	int start_discard;
	const struct gmr1_fcch_burst *burst_type;

	enum {
		FCCH_STATE_SINGLE = 0,
		FCCH_STATE_MULTI = 1,
	} state;

	float freq_err;
};

static int
fcch_sink_init(struct sample_actor *sa, void *params_ptr)
{
	struct fcch_sink_priv *priv = sa->priv;
	struct fcch_sink_params *params = params_ptr;

	priv->as = params->as;
	priv->chan_id = params->chan_id;

	priv->start_discard = params->start_discard;
	priv->burst_type = params->burst_type;

	return 0;
}

static void
fcch_sink_fini(struct sample_actor *sa)
{
	/* struct fcch_sink_priv *priv = sa->priv; */

	/* Nothing to do */
}

static int
_fcch_sink_work_single(struct sample_actor *sa,
                       float complex *data, unsigned int data_len)
{
	struct fcch_sink_priv *priv = sa->priv;
	struct osmo_cxvec _win, *win = &_win;
	int sps, win_len, base_align, toa;
	int rv;

	/* Params */
	sps = priv->as->sps;
	base_align = priv->start_discard;

	/* Get large enough window (330 ms) */
	win_len = ((330 * GMR1_SYM_RATE * sps) / 1000);

	rv = win_map(win, data, data_len, base_align, win_len);
	if (rv < 0)
		return 0; /* Not enough data yet */

	/* FCCH rough retect */
	rv = gmr1_fcch_rough(priv->burst_type, win, sps, 0.0f, &toa);
	if (rv < 0) {
		fprintf(stderr, "[!] Error during FCCH rough acquisition (%d)\n", rv);
		return rv;
	}

	/* Fine FCCH detection */
	win_map(win, data, data_len, base_align + toa, priv->burst_type->len * sps);

	rv = gmr1_fcch_fine(priv->burst_type, win, sps, 0.0f, &toa, &priv->freq_err);
	if (rv < 0) {
		fprintf(stderr, "[!] Error during FCCH fine acquisition (%d)\n", rv);
		return rv;
	}

	base_align += toa;

	/* Debug print */
	fprintf(stderr, "[+] Primary FCCH found @%d:%d [freq_err = %.1f Hz]\n",
			priv->chan_id, base_align, to_hz(priv->freq_err));

	/* Take a safety margin for next step */
	base_align -= priv->burst_type->len * sps;
	if (base_align < 0)
		base_align = 0;

	/* Next step is multi */
	priv->state = FCCH_STATE_MULTI;

	/* Done. We discard what we won't use */
	return base_align;
}

static int
_fcch_sink_work_multi(struct sample_actor *sa,
                      float complex *data, unsigned int len)
{
	struct fcch_sink_priv *priv = sa->priv;
	struct osmo_cxvec _win, *win = &_win;
	int win_len, sps, mtoa[16], n_fcch;
	float ref_snr = 0.0f, ref_freq_err = 0.0f;
	int rv, i, j;

	/* Params */
	sps = priv->as->sps;

	/* Get large enough window */
	win_len = ((650 * GMR1_SYM_RATE * sps) / 1000);

	rv = win_map(win, data, len, 0, win_len);
	if (rv < 0)
		return 0; /* Not enough data yet */

	/* Multi FCCH detection */
	rv = gmr1_fcch_rough_multi(priv->burst_type, win, sps, -priv->freq_err, mtoa, 16);
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
		win_map(win, data, len,
		        mtoa[i], priv->burst_type->len * sps);

		rv = gmr1_fcch_fine(priv->burst_type, win, sps, -priv->freq_err, &toa, &freq_err);
		if (rv) {
			fprintf(stderr, "[!] Error during FCCH fine acquisition (%d)\n", rv);
			return rv;
		}

		/* Compute SNR (comparing energy with neighboring CICH) */
		win_map(win, data, len,
		        mtoa[i] + toa + 5 * sps,
		        (117 - 10) * sps);

		e_fcch = burst_energy(win);

		win_map(win, data, len,
		        mtoa[i] + toa + (5 + 117) * sps,
		        (117 - 10) * sps);

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
		fprintf(stderr, "[.]  Potential FCCH @%d:%d [snr = %.1f dB, freq_err = %.1f Hz]\n",
			priv->chan_id,
			(int)(sa->time + mtoa[i] + toa),
			to_db(snr),
			to_hz(freq_err + priv->freq_err)
		);

		/* Save it */
		mtoa[j++] = mtoa[i] + toa;
	}

	n_fcch = j;

	/* Create processing tasks for survivors */
	for (i=0; i<n_fcch; i++) {
		struct bcch_sink_params p = {
			.as = priv->as,
			.chan_id = priv->chan_id,
			.align = sa->time + mtoa[i],
			.freq_err = priv->freq_err,
		};
		sa = sbuf_add_consumer(priv->as->buf, priv->chan_id, &bcch_sink, &p);
		if (!sa) {
			fprintf(stderr, "[!] Failed to create BCCH sink for stream #%d\n", i);
			return -ENOMEM;
		}
	}

	/* All done here */
	return -1;
}

static int
fcch_sink_work(struct sample_actor *sa,
               float complex *data, unsigned int len)
{
	struct fcch_sink_priv *priv = sa->priv;

	if (priv->state == FCCH_STATE_SINGLE)
		return _fcch_sink_work_single(sa, data, len);
	else if (priv->state == FCCH_STATE_MULTI)
		return _fcch_sink_work_multi(sa, data, len);
	else
		return -EINVAL;
}

const struct sample_actor_desc fcch_sink = {
	.init = fcch_sink_init,
	.fini = fcch_sink_fini,
	.work = fcch_sink_work,
	.priv_size = sizeof(struct fcch_sink_priv),
};
