/* GMR-1 AMBE vocoder - Speech parameters to/from frame */

/* (C) 2013 by Sylvain Munaut <tnt@246tNt.com>
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

/*! \addtogroup codec_private
 *  @{
 */

/*! \file codec/frame.c
 *  \brief Osmocom GMR-1 AMBE speech parameters to/from frame
 */

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "private.h"


/*! \brief Grab the requested bits from a frame and shift them up as requested
 *  \param[in] frame Frame data bytes
 *  \param[in] p Position of the first bit to grab
 *  \param[in] l Number of bits to grab (max 8)
 *  \param[in] s How many bits to shift the result up
 *  \returns The selected bits as a uint8_t
 */
static inline uint8_t
_get_bits(const uint8_t *frame, int p, int l, int s)
{
	uint8_t v;

	if ((p & 7) + l > 8) {
		v = ((frame[p>>3] << 8) | frame[(p>>3)+1]) >> (16 - (p&7) - l);
	} else {
		v = frame[p>>3] >> (8 - (p&7) - l);
	}

	return (v & ((1<<l)-1)) << s;
}

/*! \brief Unpack a frame into its raw encoded parameters
 *  \param[out] rp Encoded frame raw parameters to unpack into
 *  \param[in] frame Frame data (10 bytes = 80 bits)
 */
void
ambe_frame_unpack_raw(struct ambe_raw_params *rp, const uint8_t *frame)
{
	const uint8_t *p = frame;

	rp->pitch          = _get_bits(p,  0, 7, 0);
	rp->pitch_interp   = _get_bits(p, 48, 2, 0);
	rp->gain           = _get_bits(p,  7, 6, 2) | _get_bits(p, 50, 2, 0);
	rp->v_uv           = _get_bits(p, 13, 6, 0);
	rp->sf1_prba12     = _get_bits(p, 19, 6, 1) | _get_bits(p, 52, 1, 0);
	rp->sf1_prba34     = _get_bits(p, 25, 3, 3) | _get_bits(p, 53, 3, 0);
	rp->sf1_prba57     = _get_bits(p, 28, 3, 4) | _get_bits(p, 56, 4, 0);
	rp->sf1_hoc[0]     = _get_bits(p, 31, 3, 4) | _get_bits(p, 60, 4, 0);
	rp->sf1_hoc[1]     = _get_bits(p, 34, 3, 3) | _get_bits(p, 64, 3, 0);
	rp->sf1_hoc[2]     = _get_bits(p, 37, 2, 4) | _get_bits(p, 67, 4, 0);
	rp->sf1_hoc[3]     = _get_bits(p, 39, 2, 3) | _get_bits(p, 71, 3, 0);
	rp->sf0_mag_interp = _get_bits(p, 46, 2, 0);
	rp->sf0_perr_14    = _get_bits(p, 41, 3, 3) | _get_bits(p, 74, 3, 0);
	rp->sf0_perr_58    = _get_bits(p, 44, 2, 3) | _get_bits(p, 77, 3, 0);
}

/*! \brief Interpolates fundamental between subframes
 *  \param[in] f0log_prev log(fund(-1)) Previous subframe log freq
 *  \param[in] f0log_cur  log(fund(0))  Current  subframe log freq
 *  \param[in] rule Which interpolation rule to apply
 *  \returns The interpolated log(fund) frequency
 */
static float
ambe_interpolate_f0log(float f0log_prev, float f0log_cur, int rule)
{
	if (f0log_cur != f0log_prev) {
		switch (rule) {
		case 0:
			return f0log_cur;

		case 1:
			return (0.65f * f0log_cur)
			     + (0.35f * f0log_prev);

		case 2:
			return (f0log_cur + f0log_prev) / 2.0f;

		case 3:
			return f0log_prev;
		}
	} else {
		float step = 4.2672e-2f;

		switch (rule) {
		case 0:
		case 1:
			return f0log_cur;

		case 2:
			return f0log_cur + step;

		case 3:
			return f0log_cur - step;
		}
	}

	return 0.0f;	/* Not reached */
}

/*! \brief Computes and fill-in L and Lb vaues for a given subframe (from f0)
 *  \param[in] sf Subframe
 */
static void
ambe_subframe_compute_L_Lb(struct ambe_subframe *sf)
{
	sf->L = (int)floorf(0.4751f / sf->f0);

	if (sf->L < 9)
		sf->L = 9;
	else if (sf->L > 56)
		sf->L = 56;

	sf->Lb[0] = ambe_hpg_tbl[sf->L - 9][0];
	sf->Lb[1] = ambe_hpg_tbl[sf->L - 9][1];
	sf->Lb[2] = ambe_hpg_tbl[sf->L - 9][2];
	sf->Lb[3] = ambe_hpg_tbl[sf->L - 9][3];
}

/*! \brief Resample and "ac-couple" (remove mean) a magnitude array to a new L
 *  \param[in] mag_dst Destination magnitude array (L_dst elements)
 *  \param[in] L_dst Target number of magnitudes
 *  \param[in] mag_src Source magnitude array (L_src elements)
 *  \param[in] L_src Source number of magnitudes
 */
static void
ambe_resample_mag(float *mag_dst, int L_dst, float *mag_src, int L_src)
{
	float avg, step, pos;
	int i;

	avg  = 0.0f;
	step = (float)L_src / (float)L_dst;
	pos  = step;

	for (i=0; i<L_dst; i++)
	{
		int posi = (int)floorf(pos);

		if (posi == 0) {
			mag_dst[i] = mag_src[0];
		} else if (posi >= L_src) {
			mag_dst[i] = mag_src[L_src-1];
		} else {
			float alpha = pos - posi;
			mag_dst[i] = mag_src[posi-1] * (1.0f - alpha)
			           + mag_src[posi]   * alpha;
		}

		avg += mag_dst[i];
		pos += step;
	}

	avg /= L_dst;

	for (i=0; i<L_dst; i++)
		mag_dst[i] -= avg;
}

/*! \brief Compute the spectral magnitudes of subframe 1 from raw params
 *  \param[inout] sf Current subframe1 data
 *  \param[in] sf_prev Previous subframe1 data
 *  \param[in] rp Encoded frame raw parameters
 */
static void
ambe_subframe1_compute_mag(struct ambe_subframe *sf,
                           struct ambe_subframe *sf_prev,
                           struct ambe_raw_params *rp)
{
	int i, j, k;

	/* Prediction */
	ambe_resample_mag(sf->Mlog, sf->L, sf_prev->Mlog, sf_prev->L);

	for (i=0; i<sf->L; i++)
		sf->Mlog[i] *= 0.65f;

	/* PRBA */
	float prba[8];
	float Ri[8];

	prba[0] = 0.0f;
	prba[1] = ambe_prba12_tbl[rp->sf1_prba12][0];
	prba[2] = ambe_prba12_tbl[rp->sf1_prba12][1];
	prba[3] = ambe_prba34_tbl[rp->sf1_prba34][0];
	prba[4] = ambe_prba34_tbl[rp->sf1_prba34][1];
	prba[5] = ambe_prba57_tbl[rp->sf1_prba57][0];
	prba[6] = ambe_prba57_tbl[rp->sf1_prba57][1];
	prba[7] = ambe_prba57_tbl[rp->sf1_prba57][2];

	ambe_idct(Ri, prba, 8, 8);

	/* Process each block */
	float rconst = (1.0f / (2.0f * (float)M_SQRT2));
	float sum = 0.0f;

	k = 0;

	for (i=0; i<4; i++) {
		const float *hoc_tbl[] = {
			ambe_hoc0_tbl[rp->sf1_hoc[0]],
			ambe_hoc1_tbl[rp->sf1_hoc[1]],
			ambe_hoc2_tbl[rp->sf1_hoc[2]],
			ambe_hoc3_tbl[rp->sf1_hoc[3]],
		};
		float C[6], c[17];

		/* From PRBA through 2x2 xform */
		C[0] = (Ri[i<<1] + Ri[(i<<1)+1]) * 0.5f;
		C[1] = (Ri[i<<1] - Ri[(i<<1)+1]) * rconst;

		/* HOC */
		C[2] = hoc_tbl[i][0];
		C[3] = hoc_tbl[i][1];
		C[4] = hoc_tbl[i][2];
		C[5] = hoc_tbl[i][3];

		/* De-DCT */
		ambe_idct(c, C, sf->Lb[i], 6);

		/* Set magnitudes */
		for (j=0; j<sf->Lb[i]; j++)
			sf->Mlog[k++] += c[j];

		sum += C[0] * sf->Lb[i];
	}

	/* Adjust to final gain value */
	float ofs = sf->gain - (0.5f * log2f(sf->L)) - (sum / sf->L);

	for (i=0; i<sf->L; i++)
		sf->Mlog[i] += ofs;
}

/*! \brief Compute the spectral magnitudes of subframe 0 from raw params & sf1
 *  \param[inout] sf Current subframe0 data
 *  \param[in] sf1_prev Previous subframe 1 data
 *  \param[in] sf1_cur  Current subframe 1 data
 *  \param[in] rp Encoded frame raw parameters
 */
static void
ambe_subframe0_compute_mag(struct ambe_subframe *sf,
                           struct ambe_subframe *sf1_prev,
                           struct ambe_subframe *sf1_cur,
                           struct ambe_raw_params *rp)
{
	float mag_p[56], mag_c[56], alpha;
	float perr[9], corr[56];
	float gain;
	int i;

	/* Base for interpolation */
	ambe_resample_mag(mag_p, sf->L, sf1_prev->Mlog, sf1_prev->L);
	ambe_resample_mag(mag_c, sf->L, sf1_cur->Mlog,  sf1_cur->L );

	/* Interpolate / Prediction coefficient */
	alpha = ambe_sf0_interp_tbl[rp->sf0_mag_interp];

	/* Correction */
	perr[0] = 0.0f;
	perr[1] = ambe_sf0_perr14_tbl[rp->sf0_perr_14][0];
	perr[2] = ambe_sf0_perr14_tbl[rp->sf0_perr_14][1];
	perr[3] = ambe_sf0_perr14_tbl[rp->sf0_perr_14][2];
	perr[4] = ambe_sf0_perr14_tbl[rp->sf0_perr_14][3];
	perr[5] = ambe_sf0_perr58_tbl[rp->sf0_perr_58][0];
	perr[6] = ambe_sf0_perr58_tbl[rp->sf0_perr_58][1];
	perr[7] = ambe_sf0_perr58_tbl[rp->sf0_perr_58][2];
	perr[8] = ambe_sf0_perr58_tbl[rp->sf0_perr_58][3];

	ambe_idct(corr, perr, sf->L, 9);

	/* Target gain value */
	gain = sf->gain - (0.5f * log2f(sf->L));

	/* Build final value */
	for (i=0; i<sf->L; i++)
		sf->Mlog[i] = gain + corr[i] + (alpha * mag_p[i]) + ((1.0f - alpha) * mag_c[i]);
}

/*! \brief Decodes the speech parameters for both subframes from raw params
 *  \param[out] sf Array of 2 subframes data to fill-in
 *  \param[in] sf_prev Previous subframe 1 data
 *  \param[in] rp Encoded frame raw parameters
 */
void
ambe_frame_decode_params(struct ambe_subframe *sf,
                         struct ambe_subframe *sf_prev,
                         struct ambe_raw_params *rp)
{
	uint16_t v_uv;
	int i;

	/* Fundamental */
	sf[1].f0log = -4.312f - 2.1336e-2f * (rp->pitch /* + 0.5 */);
	sf[1].f0 = powf(2.0f, sf[1].f0log);

	sf[0].f0log = ambe_interpolate_f0log(sf_prev->f0log, sf[1].f0log,
	                                     rp->pitch_interp);
	sf[0].f0 = powf(2.0f, sf[0].f0log);

	/* Harmonics count (total and per-block) */
	ambe_subframe_compute_L_Lb(&sf[0]);
	ambe_subframe_compute_L_Lb(&sf[1]);

	/* Voicing decision */
	v_uv = ambe_v_uv_tbl[rp->v_uv];

	for (i=0; i<8; i++) {
		sf[0].v_uv[i] = (v_uv >> ( 7-i)) & 1;
		sf[1].v_uv[i] = (v_uv >> (15-i)) & 1;
	}

	/* Gain */
	sf[0].gain = (0.5f * sf_prev->gain) + ambe_gain_tbl[rp->gain][0];
	sf[1].gain = (0.5f * sf_prev->gain) + ambe_gain_tbl[rp->gain][1];

	if (sf[0].gain > 13.0f)
		sf[0].gain = 13.0f;

	if (sf[1].gain > 13.0f)
		sf[1].gain = 13.0f;

	/* Subframe 1 spectral magnitudes */
	ambe_subframe1_compute_mag(&sf[1], sf_prev, rp);

	/* Subframe 0 spectral magnitudes */
	ambe_subframe0_compute_mag(&sf[0], sf_prev, &sf[1], rp);
}

/*! @} */
