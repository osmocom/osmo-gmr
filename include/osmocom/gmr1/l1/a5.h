/* GMR-1 A5 Ciphering algorithm */

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

#ifndef __OSMO_GMR1_L1_A5_H__
#define __OSMO_GMR1_L1_A5_H__

/*! \defgroup a5 A5 ciphering algorithm
 *  \ingroup l1
 *  @{
 */

/*! \file l1/a5.h
 *  \brief Osmocom GMR-1 A5 ciphering algorithm header
 */

#include <stdint.h>

#include <osmocom/core/bits.h>


void gmr1_a5(int n, uint8_t *key, uint32_t fn, int nbits,
             ubit_t *dl, ubit_t *ul);

void gmr1_a5_1(uint8_t *key, uint32_t fn, int nbits,
               ubit_t *dl, ubit_t *ul);


/*! @} */

#endif /* __OSMO_GMR1_L1_A5_H__ */
