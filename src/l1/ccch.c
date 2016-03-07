/* GMR-1 CCCH channel coding */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 6.2 & 6.3 */

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

/*! \addtogroup ccch
 *  @{
 */

/*! \file l1/ccch.c
 *  \brief Osmocom GMR-1 CCCH (PCH/AGCH) channel coding implementation
 */

#include <stdint.h>
#include <string.h>

#include <osmocom/core/bits.h>
#include <osmocom/core/conv.h>
#include <osmocom/core/crc16gen.h>

#include <osmocom/gmr1/l1/conv.h>
#include <osmocom/gmr1/l1/crc.h>
#include <osmocom/gmr1/l1/interleave.h>
#include <osmocom/gmr1/l1/scramb.h>


static struct osmo_conv_code gmr1_conv_ccch;

static void __attribute__ ((constructor))
gmr1_ccch_init(void)
{
	/* Init convolutional coder */
	memcpy(&gmr1_conv_ccch, &gmr1_conv_k5_12, sizeof(struct osmo_conv_code));
	gmr1_conv_ccch.len = 208;
}


/*! \brief Stateless GMR-1 CCCH channel coder
 *  \param[out] bits_e Data bits of a burst
 *  \param[in] l2 L2 packet data
 *
 * L2 data is 24 byte long, and bits_e is a 432 hard bit array to be
 * mapped on a burst.
 */
void
gmr1_ccch_encode(ubit_t *bits_e, const uint8_t *l2)
{
	ubit_t bits_u[208];
	ubit_t bits_c[428];
	ubit_t bits_ep[432];
	int i;

	for (i=0; i<4; i++)
		bits_ep[i] = bits_ep[431-i] = 0;

	osmo_pbit2ubit_ext(bits_u, 0, l2, 0, 192, 1);
	osmo_crc16gen_set_bits(&gmr1_crc16, bits_u, 192, bits_u+192);
	osmo_conv_encode(&gmr1_conv_ccch, bits_u, bits_c);
	gmr1_interleave_intra(&bits_ep[4], bits_c, 53);
	gmr1_scramble_ubit(bits_e, bits_ep, 432);
}

/*! \brief Stateless GMR-1 CCCH channel decoder
 *  \param[out] l2 L2 packet data
 *  \param[in] bits_e Data bits of a burst
 *  \param[out] conv_rv Return of the convolutional decode (can be NULL)
 *  \return 0 if CRC check pass, any other value for fail.
 *
 * L2 data is 24 byte long, and bits_e is a 432 soft bit array
 * unmapped from a burst.
 */
int
gmr1_ccch_decode(uint8_t *l2, const sbit_t *bits_e, int *conv_rv)
{
	sbit_t bits_ep[432];
	sbit_t bits_c[428];
	ubit_t bits_u[208];
	int rv;

	gmr1_scramble_sbit(bits_ep, bits_e, 432);
	gmr1_deinterleave_intra(bits_c, &bits_ep[4], 53);

	rv = osmo_conv_decode(&gmr1_conv_ccch, bits_c, bits_u);
	if (conv_rv)
		*conv_rv = rv;

	rv = osmo_crc16gen_check_bits(&gmr1_crc16, bits_u, 192, bits_u+192);

	osmo_ubit2pbit_ext(l2, 0, bits_u, 0, 192, 1);

	return rv;
}

/*! @} */
