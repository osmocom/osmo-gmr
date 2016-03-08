/* GMR-1 BCCH channel coding */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 6.1 */

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

#ifndef __OSMO_GMR1_L1_BCCH_H__
#define __OSMO_GMR1_L1_BCCH_H__

/*! \defgroup bcch BCCH channel coding
 *  \ingroup l1_chan
 *  @{
 */

/*! \file l1/bcch.h
 *  \brief Osmocom GMR-1 BCCH channel coding header
 */

#include <stdint.h>
#include <osmocom/core/bits.h>


void gmr1_bcch_encode(ubit_t *bits_e, const uint8_t *l2);
int  gmr1_bcch_decode(uint8_t *l2, const sbit_t *bits_e, int *conv_rv);


/*! @} */

#endif /* __OSMO_GMR1_L1_BCCH_H__ */
