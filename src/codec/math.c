/* GMR-1 AMBE vocoder - Math functions */

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

/*! \file codec/math.c
 *  \brief Osmocom GMR-1 AMBE vocoder math functions
 */

#include <math.h>


#define M_PIf ((float)M_PI)	/*!< \brief Value of pi as a float */


/*! \brief Table for \ref cosf_fast */
static float cos_tbl[1024];

/*! \brief Initializes \ref cos_tbl for \ref cosf_fast */
static void __attribute__ ((constructor))
cos_tbl_init(void)
{
	int i;

	for (i=0; i<1024; i++)
		cos_tbl[i] = cosf((M_PIf * i) / 512.0f);
}

/*! \brief Fast Cosinus approximation using a simple table
 *  \param[in] angle The angle value
 *  \returns The cosinus of the angle
 */
float
cosf_fast(float angle)
{
	const float f = 512.0f / M_PIf;
	return cos_tbl[(int)(angle*f) & 1023];
}


/*! \brief Forward Discrete Cosine Transform (fDCT)
 *  \param[out] out fDCT result buffer (freq domain, M elements)
 *  \param[in] in fDCT input buffer (time domain, N elements)
 *  \param[in] N Number of points of the DCT
 *  \param[in] M Limit to the number of frequency components (M <= N)
 */
void
ambe_fdct(float *out, float *in, int N, int M)
{
	int i, j;

	for (i=0; i<M; i++)
	{
		float v = 0.0f;

		for (j=0; j<N; j++)
		{
			v += in[j] * cosf_fast( (M_PIf / N) * (j + .5f) * i );
		}

		out[i] = v / (float)N;
	}
}


/*! \brief Inverse Discrete Cosine Transform (iDCT)
 *  \param[out] out iDCT result buffer (time domain, N elements)
 *  \param[in] in iDCT input buffer (freq domain, M elements)
 *  \param[in] N Number of points of the DCT
 *  \param[in] M Limit to the number of frequency components (M <= N)
 */
void
ambe_idct(float *out, float *in, int N, int M)
{
	int i, j;

	for (i=0; i<N; i++)
	{
		float v = in[0];

		for (j=1; j<M; j++)
		{
			v += 2.0f * in[j] * cosf_fast( (M_PIf / N) * j * (i + .5f) );
		}

		out[i] = v;
	}
}

/*! @} */
