/* GMR-1 TCH9 channel coding */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 5.3 */

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

/*! \addtogroup tch9
 *  @{
 */

/*! \file l1/tch9.c
 *  \brief Osmocom GMR-1 TCH9 channel coding implementation
 */

#include <stdint.h>
#include <string.h>

#include <osmocom/core/bits.h>
#include <osmocom/core/conv.h>
#include <osmocom/core/crc16gen.h>

#include <osmocom/gmr1/l1/conv.h>
#include <osmocom/gmr1/l1/crc.h>
#include <osmocom/gmr1/l1/interleave.h>
#include <osmocom/gmr1/l1/punct.h>
#include <osmocom/gmr1/l1/scramb.h>

#include <osmocom/gmr1/l1/tch9.h>


static struct osmo_conv_code gmr1_conv_tch9_24;
static struct osmo_conv_code gmr1_conv_tch9_48;
static struct osmo_conv_code gmr1_conv_tch9_96;

static const struct osmo_conv_code *gmr1_conv_tch9[GMR1_TCH9_MAX] = {
	[GMR1_TCH9_2k4] = &gmr1_conv_tch9_24,
	[GMR1_TCH9_4k8] = &gmr1_conv_tch9_48,
	[GMR1_TCH9_9k6] = &gmr1_conv_tch9_96,
};

static void __attribute__ ((constructor))
gmr1_tch9_init(void)
{
	/* Init convolutional coders */
	memcpy(&gmr1_conv_tch9_24, &gmr1_conv_15, sizeof(struct osmo_conv_code));
	gmr1_conv_tch9_24.len = 144;
	gmr1_puncturer_generate(
		&gmr1_conv_tch9_24,
		&gmr1_punct15_P53, &gmr1_punct15_P23, &gmr1_punct15_Ps53, 41
	);

	memcpy(&gmr1_conv_tch9_48, &gmr1_conv_13, sizeof(struct osmo_conv_code));
	gmr1_conv_tch9_48.len = 240;
	gmr1_puncturer_generate(
		&gmr1_conv_tch9_48,
		&gmr1_punct13_P15, &gmr1_punct13_P25, &gmr1_punct13_Ps15, 41
	);

	memcpy(&gmr1_conv_tch9_96, &gmr1_conv_12, sizeof(struct osmo_conv_code));
	gmr1_conv_tch9_96.len = 480;
	gmr1_puncturer_generate(
		&gmr1_conv_tch9_96,
		&gmr1_punct12_P25, &gmr1_punct12_P23, &gmr1_punct12_Ps25, 158
	);
}


/*! \brief GMR-1 TCH9 channel coder
 *  \param[out] bits_e 662 encoded bits of one NT9 burst
 *  \param[in] l2 L2 packet data
 *  \param[in] mode Channel encoding mode
 *  \param[in] bits_sacch 10 saach bits to be multiplexed
 *  \param[in] bits_status 4 status bits to be multiplexed
 *  \param[in] ciph 658 bits of cipher stream (can be NULL)
 *  \param[inout] il Inter-burst interleaver state
 *
 *  L2 data size depends on the mode (18 bytes for 2k4, 30 bytes for 4k8,
 *  60 bytes for 9k6).
 */
void
gmr1_tch9_encode(ubit_t *bits_e, const uint8_t *l2, enum gmr1_tch9_mode mode,
                 const ubit_t *bits_sacch, const ubit_t *bits_status,
                 const ubit_t *ciph, struct gmr1_interleaver *il)
{
	const struct osmo_conv_code *cc = gmr1_conv_tch9[mode];
	ubit_t bits_u[480];
	ubit_t bits_c[648];
	ubit_t bits_ep_epp_x[648];
	ubit_t bits_my[658];
	int i;

	osmo_pbit2ubit_ext(bits_u, 0, l2, 0, cc->len, 1);
	osmo_conv_encode(cc, bits_u, bits_c);
	gmr1_interleave_intra(bits_ep_epp_x, bits_c, 81);
	gmr1_interleave_inter(il, bits_ep_epp_x, bits_ep_epp_x);
	gmr1_scramble_ubit(bits_ep_epp_x, bits_ep_epp_x, 648);

	memcpy(bits_my,    bits_ep_epp_x,     52);
	memcpy(bits_my+52, bits_sacch,        10);
	memcpy(bits_my+62, bits_ep_epp_x+52, 596);

	if (ciph) {
		for (i=0; i<658; i++)
			bits_my[i] ^= ciph[i];
	}

	memcpy(bits_e,    bits_my,      52);
	memcpy(bits_e+52, bits_status,   4);
	memcpy(bits_e+56, bits_my+52,  606);
}

/*! \brief GMR-1 TCH9 channel decoder
 *  \param[out] l2 L2 packet data
 *  \param[out] bits_sacch 10 saach bits demultiplexed
 *  \param[out] bits_status 4 status bits demultiplexed
 *  \param[in] bits_e 662 encoded bits of one NT9 burst
 *  \param[in] mode Channel encoding mode
 *  \param[in] ciph 658 bits of cipher stream (can be NULL)
 *  \param[inout] il Inter-burst interleaver state
 *  \param[out] conv_rv Return of the convolutional decode (can be NULL)
 *
 *  L2 data size depends on the mode (18 bytes for 2k4, 30 bytes for 4k8,
 *  60 bytes for 9k6).
 */
void
gmr1_tch9_decode(uint8_t *l2, sbit_t *bits_sacch, sbit_t *bits_status,
                 const sbit_t *bits_e, enum gmr1_tch9_mode mode,
		 const ubit_t *ciph, struct gmr1_interleaver *il,
                 int *conv_rv)
{
	const struct osmo_conv_code *cc = gmr1_conv_tch9[mode];
	sbit_t bits_my[658];
	sbit_t bits_ep_epp_x[648];
	sbit_t bits_c[648];
	ubit_t bits_u[480];
	int i, rv;

	memcpy(bits_my,     bits_e,     52);
	memcpy(bits_status, bits_e+52,   4);
	memcpy(bits_my+52,  bits_e+56, 606);

	if (ciph) {
		for (i=0; i<658; i++)
			if (ciph[i])
				bits_my[i] *= -1;
	}

	memcpy(bits_ep_epp_x,    bits_my,     52);
	memcpy(bits_sacch,       bits_my+52,  10);
	memcpy(bits_ep_epp_x+52, bits_my+62, 596);

	gmr1_scramble_sbit(bits_ep_epp_x, bits_ep_epp_x, 648);
	gmr1_deinterleave_inter(il, bits_ep_epp_x, bits_ep_epp_x);
	gmr1_deinterleave_intra(bits_c, bits_ep_epp_x, 81);

	rv = osmo_conv_decode(cc, bits_c, bits_u);
	if (conv_rv)
		*conv_rv = rv;

	osmo_ubit2pbit_ext(l2, 0, bits_u, 0, cc->len, 1);
}

/*! @} */
