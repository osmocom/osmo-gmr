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

/*! \addtogroup codec/private
 *  @{
 */

/*! \file codec/frame.c
 *  \brief Osmocom GMR-1 AMBE speech parameters to/from frame
 */

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "private.h"


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

int
ambe_frame_decode_params(struct ambe_subframe *sf,
                         struct ambe_subframe *sf_prev,
                         struct ambe_raw_params *rp)
{
	uint16_t v_uv;
	int i;

	/* w0 : fundamental */
	sf[1].f0 = powf(2.0, -4.312 - 2.1336e-2 * (rp->pitch /* + 0.5 */));

		/* FIXME: sf[0] interpolation */

	/* Harmonics count (total and per-block) */
	sf[1].L = (int)floorf(0.4761f / sf[1].f0);

	if (sf[1].L < 9)
		sf[1].L = 9;
	else if (sf[1].L > 56)
		sf[1].L = 56;

	sf[1].Lb[0] = ambe_hpg_tbl[sf[1].L - 9][0];
	sf[1].Lb[1] = ambe_hpg_tbl[sf[1].L - 9][1];
	sf[1].Lb[2] = ambe_hpg_tbl[sf[1].L - 9][2];
	sf[1].Lb[3] = ambe_hpg_tbl[sf[1].L - 9][3];

	/* Voicing decision */
	v_uv = ambe_v_uv_tbl[rp->v_uv];

	for (i=0; i<8; i++) {
		sf[0].v_uv[i] = (v_uv >> ( 7-i)) & 1;
		sf[1].v_uv[i] = (v_uv >> (15-i)) & 1;
	}

	/* Gain */
	float gain = ambe_gain_tbl[rp->gain][1];

	gain += 0.5f * sf_prev->gain;
	if (gain > 13.0f)
		gain = 13.0f;
	sf[1].gain = gain;

	gain -= 0.5f * log2f(sf[1].L);

	/* Prediction */
	{
		float avg, step, pos;

		avg  = 0.0f;
		step = (float)sf_prev->L / (float)sf[1].L;
		pos  = step;

		for (i=0; i<sf[1].L; i++)
		{
			int posi = (int)floorf(pos);

			if (posi == 0) {
				sf[1].Mlog[i] = sf_prev->Mlog[0];
			} else if (posi >= sf_prev->L) {
				sf[1].Mlog[i] = sf_prev->Mlog[sf_prev->L-1];
			} else {
				float alpha = pos - posi;
				sf[1].Mlog[i] =
					sf_prev->Mlog[posi-1] * (1.0f - alpha) +
					sf_prev->Mlog[posi]   * alpha;
			}

			avg += sf[1].Mlog[i];
			pos += step;
		}

		avg /= sf[1].L;

		for (i=0; i<sf[1].L; i++)
			sf[1].Mlog[i] = 0.65f * (sf[1].Mlog[i] - avg);
	}

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
	int j=0;
	int k;

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
		ambe_idct(c, C, sf[1].Lb[i], 6);

		/* Set magnitudes */
		for (k=0; k<sf[1].Lb[i]; k++)
			sf[1].Mlog[j++] += c[k];

		sum += C[0] * sf[1].Lb[i];
	}

	float g = gain - (sum / sf[1].L);

	for (i=0; i<sf[1].L; i++)
		sf[1].Mlog[i] += g;

	return 0;
}

/*! @} */
