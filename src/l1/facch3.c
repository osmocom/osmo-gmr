/* GMR-1 FACCH3 channel coding */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 6.9 */

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

/*! \addtogroup facch3
 *  @{
 */

/*! \file l1/facch3.c
 *  \brief Osmocom GMR-1 FACCH3 channel coding implementation
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


static struct osmo_conv_code gmr1_conv_facch3;

static void __attribute__ ((constructor))
gmr1_facch3_init(void)
{
	/* Init convolutional coder */
	memcpy(&gmr1_conv_facch3, &gmr1_conv_k5_14, sizeof(struct osmo_conv_code));
	gmr1_conv_facch3.len = 92;
}


/*! \brief Stateless GMR-1 FACCH3 channel coder
 *  \param[out] bits_e 4*104 encoded bits of 4 bursts
 *  \param[in] l2 L2 packet data
 *  \param[in] bits_s 4*8 status bits to be multiplexed
 *  \param[in] ciph 4*96 bits of cipher stream (can be NULL)
 *
 * L2 data is 10 byte long.
 * bits_s is 32 bits, 8 bits for each of the 4 burts, organized as 4 s_n
 * followed by 4 s_p, as shown in section 7.3.2.2.
 * bits_e is a 432 hard bit array to be mapped on 4 bursts.
 * ciph is the A5 cipher stream to use, 96 bits for each of the 4 burts.
 */
void
gmr1_facch3_encode(ubit_t *bits_e, const uint8_t *l2,
                   const ubit_t *bits_s, const ubit_t *ciph)
{
	ubit_t bits_u[92];
	ubit_t bits_c[384];
	ubit_t bits_cp[96*4];
	ubit_t bits_ep[96*4];
	ubit_t bits_xmy[96*4];
	int i, j;

	osmo_pbit2ubit_ext(bits_u, 0, l2, 0, 76, 1);
	osmo_crc16gen_set_bits(&gmr1_crc16, bits_u, 76, bits_u+76);

	osmo_conv_encode(&gmr1_conv_facch3, bits_u, bits_c);

	for (i=0; i<384; i++)
		bits_cp[(i&3)*96 + (i>>2)] = bits_c[i];

	for (i=0; i<4; i++)
	{
		ubit_t *b_bits_cp  = bits_cp  +  96*i;
		ubit_t *b_bits_ep  = bits_ep  +  96*i;
		ubit_t *b_bits_xmy = bits_xmy +  96*i;
		ubit_t *b_bits_e   = bits_e   + 104*i;
		const ubit_t *b_bits_s = bits_s + 8*i;
		const ubit_t *b_ciph = ciph + 96*i;

		gmr1_interleave_intra(b_bits_ep, b_bits_cp, 12);
		gmr1_scramble_ubit(b_bits_xmy, b_bits_ep, 96);

		if (ciph) {
			for (j=0; j<96; j++)
				b_bits_xmy[j] ^= b_ciph[j];
		}

		memcpy(b_bits_e   , b_bits_xmy,    22);
		memcpy(b_bits_e+22, b_bits_s,       8);
		memcpy(b_bits_e+30, b_bits_xmy+22, 74);
	}
}

/*! \brief Stateless GMR-1 FACCH3 channel decoder
 *  \param[out] l2 L2 packet data
 *  \param[out] bits_s 4*8 status bits de-multiplexed
 *  \param[in] bits_e 4*104 encoded bits of 4 bursts
 *  \param[in] ciph 4*96 bits of cipher stream (can be NULL)
 *  \param[out] conv_rv Return of the convolutional decode (can be NULL)
 *  \return 0 if CRC check pass, any other value for fail.
 *
 * L2 data is 10 byte long.
 * bits_s is 32 bits, 8 bits for each of the 4 burts, organized as 4 s_n
 * followed by 4 s_p, as shown in section 7.3.2.2.
 * bits_e is a 424 soft bits array unmapped from 4 bursts.
 * ciph is the A5 cipher stream to use, 96 bits for each of the 4 burts.
 */
int
gmr1_facch3_decode(uint8_t *l2, ubit_t *bits_s,
                   const sbit_t *bits_e, const ubit_t *ciph, int *conv_rv)
{
	sbit_t bits_xmy[96*4];
	sbit_t bits_ep[96*4];
	sbit_t bits_cp[96*4];
	sbit_t bits_c[384];
	ubit_t bits_u[92];
	int rv, i, j;

	for (i=0; i<4; i++)
	{
		const sbit_t *b_bits_e = bits_e + 104*i;
		ubit_t *b_bits_s   = bits_s   +   8*i;
		sbit_t *b_bits_xmy = bits_xmy +  96*i;
		sbit_t *b_bits_ep  = bits_ep  +  96*i;
		sbit_t *b_bits_cp  = bits_cp  +  96*i;
		const ubit_t *b_ciph = ciph + 96*i;

		for (j=0; j<8; j++)
			b_bits_s[j] = b_bits_e[22+j] < 0;

		memcpy(b_bits_xmy,    b_bits_e,    22);
		memcpy(b_bits_xmy+22, b_bits_e+30, 74);

		if (ciph) {
			for (j=0; j<96; j++)
				if (b_ciph[j])
					b_bits_xmy[j] *= -1;
		}

		gmr1_scramble_sbit(b_bits_ep, b_bits_xmy, 96);
		gmr1_deinterleave_intra(b_bits_cp, b_bits_ep, 12);
	};

	for (i=0; i<384; i++)
		bits_c[i] = bits_cp[(i&3)*96 + (i>>2)];

	rv = osmo_conv_decode(&gmr1_conv_facch3, bits_c, bits_u);
	if (conv_rv)
		*conv_rv = rv;

	rv = osmo_crc16gen_check_bits(&gmr1_crc16, bits_u, 76, bits_u+76);

	l2[9] = 0; /* upper nibble won't be written */
	osmo_ubit2pbit_ext(l2, 0, bits_u, 0, 76, 1);

	return rv;
}

/*! @} */
