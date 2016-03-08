/* GMR-1 FACCH9 channel coding */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 6.11 */

/* (C) 2011-2016 by Sylvain Munaut <tnt@246tNt.com>
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

#ifndef __OSMO_GMR1_L1_FACCH9_H__
#define __OSMO_GMR1_L1_FACCH9_H__

/*! \defgroup facch9 FACCH9 channel coding
 *  \ingroup l1_chan
 *  @{
 */

/*! \file l1/facch9.h
 *  \brief Osmocom GMR-1 FACCH9 channel coding header
 */

#include <stdint.h>
#include <osmocom/core/bits.h>


void gmr1_facch9_encode(ubit_t *bits_e, const uint8_t *l2,
                        const ubit_t *bits_sacch, const ubit_t *bits_status,
                        const ubit_t *ciph);
int gmr1_facch9_decode(uint8_t *l2, sbit_t *bits_sacch, sbit_t *bits_status,
                       const sbit_t *bits_e, const ubit_t *ciph, int *conv_rv);


/*! @} */

#endif /* __OSMO_GMR1_L1_FACCH9_H__ */
