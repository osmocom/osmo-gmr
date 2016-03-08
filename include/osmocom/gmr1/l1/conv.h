/* GMR-1 convolutional coding */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 4.4 */

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

#ifndef __OSMO_GMR1_L1_CONV_H__
#define __OSMO_GMR1_L1_CONV_H__

/*! \defgroup conv Convolutional coding
 *  \ingroup l1_prim
 *  @{
 */

/*! \file l1/conv.h
 *  \brief Osmocom GMR-1 convolutional coding header
 */

#include <osmocom/core/conv.h>


extern const struct osmo_conv_code gmr1_conv_12;
extern const struct osmo_conv_code gmr1_conv_13;
extern const struct osmo_conv_code gmr1_conv_14;
extern const struct osmo_conv_code gmr1_conv_15;
extern const struct osmo_conv_code gmr1_conv_tch3;


/*! @} */

#endif /* __OSMO_GMR1_L1_CONV_H__ */
