/* GMR-1 RACH channel coding */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 6.5 */

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

/*! \addtogroup rach
 *  @{
 */

/*! \file l1/rach.c
 *  \brief Osmocom GMR-1 RACH channel coding implementation
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <osmocom/core/bits.h>
#include <osmocom/core/conv.h>
#include <osmocom/core/crcgen.h>

#include <osmocom/gmr1/l1/conv.h>
#include <osmocom/gmr1/l1/crc.h>
#include <osmocom/gmr1/l1/interleave.h>
#include <osmocom/gmr1/l1/scramb.h>

static struct osmo_conv_code gmr1_conv_rach;

static void __attribute__ ((constructor))
gmr1_rach_init(void)
{
	int i, *p;

	/* Init convolutional coder */
	memcpy(&gmr1_conv_rach, &gmr1_conv_14, sizeof(struct osmo_conv_code));
	gmr1_conv_rach.len = 159;

	/* Generate puncturer (only b[0] .. b[539] punctured) */
	p = malloc((135*2 + 1) * sizeof(int));
	if (!p)
		abort();

	for (i=0; i<135; i++) {
		p[(i<<1)  ] = (i << 2) + 2;
		p[(i<<1)+1] = (i << 2) + 3;
	}

	p[270] = -1;

	gmr1_conv_rach.puncture = p;
}


/*! \brief Stateless GMR-1 RACH channel coder
 *  \param[out] bits_e Data bits of a burst
 *  \param[in] rach RACH packet data (2 class-1 bytes, 16 class-2 bytes)
 *  \param[in] sb_mask RACH SB Mask value (see GMR-1 04.008)
 *
 * RACH data is 18 bytes long (2 class-1, 16 class-2), and bits_e is a
 * 494 hard bits array to be mapped on a RACH burst.
 */
void
gmr1_rach_encode(ubit_t *bits_e, const uint8_t *rach, uint8_t sb_mask)
{
	ubit_t bits_u[159], *bits_u1, *bits_u2;
	ubit_t bits_c[382];
	ubit_t bits_e1p[112], bits_e2p[270];
	ubit_t bits_ep[494];
	ubit_t bits_x[494];
	int i;

	/* rach -> d : unpacking */
	bits_u1 = bits_u + 135;
	bits_u2 = bits_u;

	osmo_pbit2ubit_ext(bits_u1, 0, rach,  0,  16, 1);
	osmo_pbit2ubit_ext(bits_u2, 0, rach, 16, 123, 1);

	/* d -> u : CRC addition */
	osmo_crc8gen_set_bits (&gmr1_crc8,  bits_u1,  16, bits_u1+16);
	osmo_crc16gen_set_bits(&gmr1_crc12, bits_u2, 123, bits_u2+123);

	/* u -> u' : masking */
	for (i=0; i<8; i++)
		bits_u1[16+i] ^= (sb_mask >> (7-i)) & 1;

	/* u' -> c : convolutional coding */
	osmo_conv_encode(&gmr1_conv_rach, bits_u, bits_c);

	/* c -> e' : interleaving */
	gmr1_interleave_intra(bits_e1p, bits_c+270, 14);
	gmr1_interleave_intra(bits_e2p, bits_c,     33);

	memcpy(bits_e2p+264, bits_c+264, 6);

	memcpy(bits_ep,     bits_e1p, 112);
	memcpy(bits_ep+112, bits_e2p, 270);
	memcpy(bits_ep+382, bits_e1p, 112);

	/* e' -> x  : scrambling */
	gmr1_scramble_ubit(bits_x, bits_ep, 494);

	/* x -> e=m : multiplex */
	memcpy(bits_e,     bits_x+112, 136);
	memcpy(bits_e+136, bits_x,     112);
	memcpy(bits_e+248, bits_x+382, 112);
	memcpy(bits_e+360, bits_x+248, 134);
}

/*! \brief Stateless GMR-1 RACH channel decoder
 *  \param[out] rach RACH packet data (2 class-1 bytes, 16 class-2 bytes)
 *  \param[in] bits_e Data bits of a burst
 *  \param[in] sb_mask RACH SB Mask value (see GMR-1 04.008)
 *  \param[out] conv_rv Return of the convolutional decode (can be NULL)
 *  \param[out] crc_rv Return array of the 2 CRC checks (can be NULL)
 *  \return 0 if all CRC check pass, any other value for fail.
 *
 * RACH data is 18 bytes long (2 class-1, 16 class-2), and bits_e is a
 * 494 soft bits array unmapped from a RACH burst.
 */
int
gmr1_rach_decode(uint8_t *rach, const sbit_t *bits_e, uint8_t sb_mask,
                 int *conv_rv, int *crc_rv)
{
	sbit_t bits_x[494];
	sbit_t bits_ep[494];
	sbit_t bits_e1p[112], bits_e2p[270];
	sbit_t bits_c[382];
	ubit_t bits_u[159], *bits_u1, *bits_u2;
	int i, rv, crc[2];

	/* e=m -> x : de-multiplex */
	memcpy(bits_x,     bits_e+136, 112);
	memcpy(bits_x+112, bits_e,     136);
	memcpy(bits_x+248, bits_e+360, 134);
	memcpy(bits_x+382, bits_e+248, 112);

	/* x -> e' : de-scrambling */
	gmr1_scramble_sbit(bits_ep, bits_x, 494);

	/* e' -> c : de-interleaving */
	memcpy(bits_e2p, bits_ep+112, 270);

	for (i=0; i<112; i++)
		bits_e1p[i] = (sbit_t)(((int)bits_ep[i] + (int)bits_ep[i+382]) >> 1);

	gmr1_deinterleave_intra(bits_c+270, bits_e1p, 14);
	gmr1_deinterleave_intra(bits_c,     bits_e2p, 33);
	memcpy(bits_c+264, bits_e2p+264, 6);

	/* c -> u' / u : convolutional decoding */
	rv = osmo_conv_decode(&gmr1_conv_rach, bits_c, bits_u);
	if (conv_rv)
		*conv_rv = rv;

	bits_u1 = bits_u + 135;
	bits_u2 = bits_u;

	/* CRC checks */
	crc[0] = osmo_crc8gen_check_bits (&gmr1_crc8,  bits_u1,  16, bits_u1+16);
	crc[1] = osmo_crc16gen_check_bits(&gmr1_crc12, bits_u2, 123, bits_u2+123);

	if (crc[0]) {
		for (i=0; i<8; i++)
			bits_u1[16+i] ^= (sb_mask >> (7-i)) & 1;
		crc[0] = osmo_crc8gen_check_bits (&gmr1_crc8,  bits_u1,  16, bits_u1+16);
	}

	if (crc_rv) {
		crc_rv[0] = crc[0];
		crc_rv[1] = crc[1];
	}

	/* CRC removal & packing */
	rach[17] = 0x00;

	osmo_ubit2pbit_ext(rach,  0, bits_u1, 0,  16, 1);
	osmo_ubit2pbit_ext(rach, 16, bits_u2, 0, 123, 1);

	return crc[0] || crc[1];
}

/*! @} */
