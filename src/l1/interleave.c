/* GMR-1 interleaving */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 4.8 */

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

/*! \addtogroup interleave
 *  @{
 */

/*! \file interleave.c
 *  \file Osmocom GMR-1 interleaving implementation
 */

#include <stdint.h>


/*! \brief GMR-1 intra burst inteleaver
 *  \param[out] out Interleaved bit array to write to
 *  \param[in] in Original bit array to read from
 *  \param[in] N Dimension of the interleaving matrix
 *
 * Both arrays need to have a length of (8*N).
 * This routine works for any type that has the same size as uint8_t like
 * sbit_t or ubit_t.
 */
void
gmr1_interleave_intra(void *out, void *in, int N)
{
	const uint8_t *inb = (uint8_t *)in;
	uint8_t *outb = (uint8_t *)out;
	int len = N << 3;
	int kc, kep, i, j;

	for (kc=0; kc<len; kc++) {
		i = kc >> 3;
		j = (5 * kc) & 7;
		kep = N * j + i;
		outb[kep] = inb[kc];
	}
}

/*! \brief GMR-1 intra burst de-interleaver
 *  \param[out] out Deinterleaved bit array to write to
 *  \param[in] in Interleaved bit array to read from
 *  \param[in] N Dimension of the interleaving matrix
 *
 * Both arrays need to have a length of (8*N).
 * This routine works for any type that has the same size as uint8_t like
 * sbit_t or ubit_t.
 */
void
gmr1_deinterleave_intra(void *out, void *in, int N)
{
	const uint8_t *inb = (uint8_t *)in;
	uint8_t *outb = (uint8_t *)out;
	int len = N << 3;
	int kc, kep, i, j;

	for (kc=0; kc<len; kc++) {
		i = kc >> 3;
		j = (5 * kc) & 7;
		kep = N * j + i;
		outb[kc] = inb[kep];
	}
}

/*! }@ */
