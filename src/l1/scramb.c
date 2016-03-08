/* GMR-1 scrambling */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 4.9 */

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

/*! \addtogroup scramb
 *  @{
 */

/*! \file l1/scramb.c
 *  \brief Osmocom GMR-1 scrambling implementation
 */

#include <stdint.h>

#include <osmocom/core/bits.h>


/*
 * h(D) = 1 + D + D^15
 * i(D) = 1 + D + D^3 + D^6 + D^8 + D^10 + D^11 + D^14
 */

#define GMR1_SCRAMBLE_REG_INIT 0x4d4b

static inline int
gmr1_scramble_reg_next(uint16_t *reg)
{
	int b;
	uint16_t reg_val;

	reg_val = *reg;
	b = ((reg_val >> 14) ^ reg_val) & 1;
	*reg = (reg_val << 1) | b;

	return b;
}


/*! \brief Scrambles/Unscrambles a softbit vector
 *  \param[out] out output sbit_t array
 *  \param[in] in input sbit_t array
 *  \param[in] len length of the array to convert
 *
 * The output array can be equal to the input array for in-place
 * scrambling/unscrambling
 */
void
gmr1_scramble_sbit(sbit_t *out, const sbit_t *in, int len)
{
	uint16_t r = GMR1_SCRAMBLE_REG_INIT;
	int i;

	for (i=0; i<len; i++) {
		sbit_t v = in[i];
		out[i] = gmr1_scramble_reg_next(&r) ? -v : v;
	}
}

/*! \brief Scrambles/Unscrambles an unpacked hard bit vector
 *  \param[out] out output ubit_t array
 *  \param[in] in input ubit_t array
 *  \param[in] len length of the array to convert
 *
 * The output array can be equal to the input array for in-place
 * scrambling/unscrambling
 */
void
gmr1_scramble_ubit(ubit_t *out, const ubit_t *in, int len)
{
	uint16_t r = GMR1_SCRAMBLE_REG_INIT;
	int i;

	for (i=0; i<len; i++) {
		ubit_t v = in[i];
		out[i] = v ^ gmr1_scramble_reg_next(&r);
	}
}

/*! @} */
