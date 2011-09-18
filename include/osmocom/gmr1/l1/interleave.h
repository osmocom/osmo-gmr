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

/*! \defgroup interleave GMR-1 interleaving
 *  @{
 */

/*! \file interleave.h
 *  \file Osmocom GMR-1 interleaving header
 */


void gmr1_interleave_intra(void *out, const void *in, int N);
void gmr1_deinterleave_intra(void *out, const void *in, int N);


/*! }@ */

#endif /* __OSMO_GMR1_L1_INTERLEAVE_H__ */
