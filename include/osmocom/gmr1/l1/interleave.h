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

#ifndef __OSMO_GMR1_L1_INTERLEAVE_H__
#define __OSMO_GMR1_L1_INTERLEAVE_H__

/*! \defgroup interleave Interleaving
 *  \ingroup l1_prim
 *  @{
 */

/*! \file l1/interleave.h
 *  \brief Osmocom GMR-1 interleaving header
 */


/* Intra burst interleaving */

void gmr1_interleave_intra(void *out, const void *in, int N);
void gmr1_deinterleave_intra(void *out, const void *in, int N);


/* Inter burst interleaving */

/*! \brief GMR1 inter-burst (de)interleaver state */
struct gmr1_interleaver
{
	int N;			/*!< \brief Interleaver depth */
	int K;			/*!< \brief Interleaver width */
	int n;			/*!< \brief Current burst number */
	uint8_t *bits_cpp;	/*!< \brief c'' bit state storage */
};

int  gmr1_interleaver_init(struct gmr1_interleaver *il, int N, int K);
void gmr1_interleaver_fini(struct gmr1_interleaver *il);
void gmr1_interleave_inter(struct gmr1_interleaver *il,
                           void *bits_epp, void *bits_ep);
void gmr1_deinterleave_inter(struct gmr1_interleaver *il,
                             void *bits_ep, void *bits_epp);


/*! @} */

#endif /* __OSMO_GMR1_L1_INTERLEAVE_H__ */
