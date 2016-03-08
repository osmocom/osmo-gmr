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

/*! \addtogroup facch9
 *  @{
 */

/*! \file l1/facch9.c
 *  \brief Osmocom GMR-1 FACCH9 channel coding implementation
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


static struct osmo_conv_code gmr1_conv_facch9;

static void __attribute__ ((constructor))
gmr1_facch9_init(void)
{
	/* Init convolutional coder */
	memcpy(&gmr1_conv_facch9, &gmr1_conv_12, sizeof(struct osmo_conv_code));
	gmr1_conv_facch9.len = 316;
}


/*! \brief Stateless GMR-1 FACCH9 channel coder
 *  \param[out] bits_e 662 encoded bits of one NT9 burst
 *  \param[in] l2 L2 packet data (38 bytes, last nibble unused)
 *  \param[in] bits_sacch 10 saach bits to be multiplexed
 *  \param[in] bits_status 4 status bits to be multiplexed
 *  \param[in] ciph 658 bits of cipher stream (can be NULL)
 */
void
gmr1_facch9_encode(ubit_t *bits_e, const uint8_t *l2,
                   const ubit_t *bits_sacch, const ubit_t *bits_status,
                   const ubit_t *ciph)
{
	ubit_t bits_u[316];
	ubit_t bits_c[640];
	ubit_t bits_epp_x[648];
	ubit_t bits_my[658];
	int i;

	osmo_pbit2ubit_ext(bits_u, 0, l2, 0, 300, 1);
	osmo_crc16gen_set_bits(&gmr1_crc16, bits_u, 300, bits_u+300);

	osmo_conv_encode(&gmr1_conv_facch9, bits_u, bits_c);

	memset(bits_epp_x, 0, 4);
	memset(bits_epp_x+644, 0, 4);
	gmr1_interleave_intra(bits_epp_x+4, bits_c, 80);

	gmr1_scramble_ubit(bits_epp_x, bits_epp_x, 648);

	memcpy(bits_my,    bits_epp_x,     52);
	memcpy(bits_my+52, bits_sacch,     10);
	memcpy(bits_my+62, bits_epp_x+52, 596);

	if (ciph) {
		for (i=0; i<658; i++)
			bits_my[i] ^= ciph[i];
	}

	memcpy(bits_e,    bits_my,      52);
	memcpy(bits_e+52, bits_status,   4);
	memcpy(bits_e+56, bits_my+52,  606);
}

/*! \brief Stateless GMR-1 FACCH9 channel decoder
 *  \param[out] l2 L2 packet data (38 bytes, last nibble unused)
 *  \param[out] bits_sacch 10 saach bits demultiplexed
 *  \param[out] bits_status 4 status bits demultiplexed
 *  \param[in] bits_e 662 encoded bits of one NT9 burst
 *  \param[in] ciph 658 bits of cipher stream (can be NULL)
 *  \param[out] conv_rv Return of the convolutional decode (can be NULL)
 *  \return 0 if CRC check pass, any other value for fail.
 */

int
gmr1_facch9_decode(uint8_t *l2, sbit_t *bits_sacch, sbit_t *bits_status,
                   const sbit_t *bits_e, const ubit_t *ciph, int *conv_rv)
{
	sbit_t bits_my[658];
	sbit_t bits_epp_x[648];
	sbit_t bits_c[640];
	ubit_t bits_u[316];
	int i, rv;

	memcpy(bits_my,     bits_e,     52);
	memcpy(bits_status, bits_e+52,   4);
	memcpy(bits_my+52,  bits_e+56, 606);

	if (ciph) {
		for (i=0; i<658; i++)
			if (ciph[i])
				bits_my[i] *= -1;
	}

	memcpy(bits_epp_x,    bits_my,     52);
	memcpy(bits_sacch,    bits_my+52,  10);
	memcpy(bits_epp_x+52, bits_my+62, 596);

	gmr1_scramble_sbit(bits_epp_x, bits_epp_x, 648);

	gmr1_deinterleave_intra(bits_c, bits_epp_x+4, 80);

	rv = osmo_conv_decode(&gmr1_conv_facch9, bits_c, bits_u);
	if (conv_rv)
		*conv_rv = rv;

	rv = osmo_crc16gen_check_bits(&gmr1_crc16, bits_u, 300, bits_u+300);

	l2[37] = 0; /* upper nibble won't be written */
	osmo_ubit2pbit_ext(l2, 0, bits_u, 0, 300, 1);

	return rv;
}

/*! @} */
