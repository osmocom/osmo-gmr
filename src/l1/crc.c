/* GMR-1 CRC */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 4.3 */

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

/*! \addtogroup crc
 *  @{
 */

/*! \file l1/crc.c
 *  \brief Osmocom GMR-1 CRC implementation
 */

#include <stdint.h>

#include <osmocom/core/bits.h>
#include <osmocom/core/crcgen.h>


/*! \brief GMR-1 CRC8
 *  g8(D) = D8 + D7 + D4 + D3 + D + 1
 */
const struct osmo_crc8gen_code gmr1_crc8 = {
	.bits = 8,
	.poly = 0x9b,
	.init = 0x00,
	.remainder = 0x00,
};

/*! \brief GMR-1 CRC12
 *  g12(D) = D12 + D11 + D3 + D2 + D + 1
 */
const struct osmo_crc16gen_code gmr1_crc12 = {
	.bits = 12,
	.poly = 0x80f,
	.init = 0x0000,
	.remainder = 0x0000,
};

/*! \brief GMR-1 CRC16
 *  g16(D) = D16 + D12 + D5 + 1
 */
const struct osmo_crc16gen_code gmr1_crc16 = {
	.bits = 16,
	.poly = 0x1021,
	.init = 0x0000,
	.remainder = 0x0000,
};

/*! @} */
