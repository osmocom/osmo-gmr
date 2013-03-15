/* GMR-1 TCH9 channel coding */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 5.3 */

/* (C) 2012 by Sylvain Munaut <tnt@246tNt.com>
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

#ifndef __OSMO_GMR1_L1_TCH9_H__
#define __OSMO_GMR1_L1_TCH9_H__

/*! \defgroup tch9 TCH9 channel coding
 *  \ingroup l1_chan
 *  @{
 */

/*! \file l1/tch9.h
 *  \brief Osmocom GMR-1 TCH9 channel coding header
 */

#include <stdint.h>
#include <osmocom/core/bits.h>

struct gmr1_interleaver;


enum gmr1_tch9_mode {
	GMR1_TCH9_2k4,
	GMR1_TCH9_4k8,
	GMR1_TCH9_9k6,
	GMR1_TCH9_MAX
};

void gmr1_tch9_encode(ubit_t *bits_e, const uint8_t *l2, enum gmr1_tch9_mode mode,
                      const ubit_t *bits_sacch, const ubit_t *bits_status,
                      const ubit_t *ciph, struct gmr1_interleaver *il);
void gmr1_tch9_decode(uint8_t *l2, sbit_t *bits_sacch, sbit_t *bits_status,
                      const sbit_t *bits_e, enum gmr1_tch9_mode mode,
                      const ubit_t *ciph, struct gmr1_interleaver *il,
                      int *conv_rv);


/*! @} */

#endif /* __OSMO_GMR1_L1_TCH9_H__ */
