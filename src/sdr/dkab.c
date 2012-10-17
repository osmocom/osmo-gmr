/* GMR-1 SDR - DKABs bursts  */
/* See GMR-1 05.004 (ETSI TS 101 376-5-4 V1.2.1) - Section 6.1
 *     GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) - Section 7.4.6 */

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

/*! \addtogroup dkab
 *  @{
 */

/*! \file sdr/dkab.c
 *  \brief Osmocom GMR-1 DKABs bursts implementation
 */

#include <complex.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>
#include <stdint.h>
#include <stdio.h>

#include <osmocom/core/bits.h>

#include <osmocom/dsp/cxvec.h>
#include <osmocom/dsp/cxvec_math.h>

#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/dkab.h>


/*! \brief Ratio between peak power and valley power for DKAB detection */
#define DKAB_PWR_RATIO_THRESHOLD	10.0f


/*! \brief Finds the precise TOA of a DKAB burts by looking for power spikes
 *  \param[in] burst Complex signal of the burst
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] p DKAB position
 *  \param[out] toa_p Pointer to TOA return variable
 *  \returns 0 for success, 1 if DKAB not found, -errno for fatal errors
 */
static int
_gmr1_dkab_find_toa(struct osmo_cxvec *burst, int sps, int p, float *toa_p)
{
	struct osmo_cxvec *pwr = NULL;
	int rv, w, i, ofs[2], d, mi;
	float mp, toa;
	float egy_peak, egy_valley;
	int l_peak, l_valley, toa_i;

	/* Window size */
	w = burst->len - (GMR1_DKAB_SYMS * sps) + 1;
	if (w <= 0)
		return -EINVAL;

	/* Energy vector */
	pwr = osmo_cxvec_alloc(w);
	if (!pwr)
		return -ENOMEM;

	ofs[0] = sps * (2 + p);		/* First  KAB position */
	ofs[1] = sps * (2 + p + 59);	/* Second KAB position */
	d = sps * 5;			/* Length of KAB */

	pwr->data[0] = 0.0f;

	for (i=0; i<d; i++) {
		pwr->data[0] +=
			osmo_normsqf(burst->data[ofs[0]+i]) +
			osmo_normsqf(burst->data[ofs[1]+i]);
	}

	mi = 0;				/* Max index */
	mp = pwr->data[0];		/* Max pwr */

	for (i=0; i<w-1; i++)
	{
		float np;

		np = pwr->data[i]
			- osmo_normsqf(burst->data[ofs[0]+i])
			- osmo_normsqf(burst->data[ofs[1]+i])
			+ osmo_normsqf(burst->data[ofs[0]+d+i])
			+ osmo_normsqf(burst->data[ofs[1]+d+i]);

		pwr->data[i+1] = np;

		if (np > mp) {
			mi = i+1;
			mp = np;
		}
	}

	pwr->len = w;

	/* Weigh & center peak */
	toa = (float)mi;
	if ((mi > 0) && (mi < (w-1)))
		toa += 0.5f * (-pwr->data[mi-1] + pwr->data[mi+1]) /
		       (-pwr->data[mi-1] + 2.0f * pwr->data[mi] - pwr->data[mi+1]);
	toa += ((float)(sps-1)) / 2.0f;

	*toa_p = toa;

	toa_i = (int)roundf(toa);

	/* Check the ratio between the peaks and valley to validate */
	egy_peak = 0.0f;
	l_peak = d * 2;
	for (i=0; i<d; i++) {
		egy_peak +=
			osmo_normsqf(burst->data[toa_i+ofs[0]+i]) +
			osmo_normsqf(burst->data[toa_i+ofs[1]+i]);
	}
	egy_peak /= l_peak;

	egy_valley = 0.0f;
	l_valley = ofs[1] - ofs[0] - d;
	for (i=0; i<l_valley; i++)
		egy_valley += osmo_normsqf(burst->data[toa_i+ofs[0]+d+i]);
	egy_valley /= l_valley;

	rv = ((egy_peak /egy_valley) > DKAB_PWR_RATIO_THRESHOLD) ? 0 : 1;

	/* Done */
	osmo_cxvec_free(pwr);

	return rv;
}

/*! \brief Converts a burst into softbits given proper TOA
 *  \param[in] burst Complex signal of the burst
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] p DKAB position
 *  \param[in] toa The TOA to use to extract symbols
 *  \param[out] ebits Encoded soft bits return array
 *  \returns 0 for success. -errno for errors
 */
static int
_gmr1_dkab_soft_bits(struct osmo_cxvec *burst, int sps, int p, float toa,
                     sbit_t *ebits)
{
	int i, toa_i, ofs[2], o;
	float pd;

	toa_i = (int)roundf(toa);
	ofs[0] = toa_i + sps * (2 + p);		/* First DKAB */
	ofs[1] = toa_i + sps * (2 + p + 159);	/* Second DKAB */

	for (i=0; i<8; i++) {
		o = ofs[i>>2] + (i&3);
		pd = cargf(burst->data[o] * conjf(burst->data[o+sps]));
		ebits[i] = (sbit_t)roundf((0.5f - (fabsf(pd) / M_PIf)) * 254.0f);
	}

	return 0;
}

/*! \brief All-in-one finding and demodulation of DKAB bursts
 *  \param[in] burst_in Complex signal of the burst
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] freq_shift Frequency shift to pre-apply to burst_in (rad/sym)
 *  \param[in] p DKAB position
 *  \param[out] ebits Encoded soft bits return array
 *  \param[out] toa_p Pointer to TOA return variable
 *  \returns 0 for success, 1 if DKAB not found, -errno for fatal errors
 *
 * burst_in is expected to be longer than necessary. Any extra length will be
 * used as 'search window' to find proper alignement. Good practice is to have
 * a few samples too much in front and a few samples after the expected TOA.
 */
int
gmr1_dkab_demod(struct osmo_cxvec *burst_in, int sps, float freq_shift, int p,
                sbit_t *ebits, float *toa_p)
{
	struct osmo_cxvec *burst = NULL;
	int rv;

	/* Normalize the burst and counter rotate by pi/4 */
	burst = osmo_cxvec_sig_normalize(burst_in, 1, (freq_shift - (M_PIf/4)) / sps, NULL);
	if (!burst) {
		rv = -ENOMEM;
		goto err;
	}

	/* Find TOA */
	rv = _gmr1_dkab_find_toa(burst, sps, p, toa_p);
	if (rv)
		goto err;

	/* Demodulate into soft bits */
	rv = _gmr1_dkab_soft_bits(burst, sps, p, *toa_p, ebits);

	/* Done */
err:
	osmo_cxvec_free(burst);

	return rv;
}

/*! @} */
