/* GMR-1 TCH3 channel coding */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 5.1 */

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

/*! \addtogroup tch3
 *  @{
 */

/*! \file l1/tch3.c
 *  \brief Osmocom GMR-1 TCH3 channel coding implementation
 */

#include <stdint.h>
#include <string.h>

#include <osmocom/core/bits.h>
#include <osmocom/core/conv.h>

#include <osmocom/gmr1/l1/conv.h>
#include <osmocom/gmr1/l1/punct.h>
#include <osmocom/gmr1/l1/scramb.h>


static struct osmo_conv_code gmr1_conv_tch3_speech;

static void __attribute__ ((constructor))
gmr1_tch3_init(void)
{
	/* Init convolutional coder */
	memcpy(&gmr1_conv_tch3_speech, &gmr1_conv_tch3, sizeof(struct osmo_conv_code));
	gmr1_conv_tch3_speech.len = 48;
	gmr1_puncturer_generate(&gmr1_conv_tch3_speech, &gmr1_punct12_P12);
}


/*! \brief Stateless GMR-1 TCH3 channel coder
 *  \param[out] bits_e 212 encoded bits to be mapped on a burst
 *  \param[in] frame0 1st speech frame (10 byte / 80 bits, msb first)
 *  \param[in] frame1 2nd speech frame (10 byte / 80 bits, msb first)
 *  \param[in] bits_s 4 status bits to be multiplexed
 *  \param[in] ciph 208 bits of cipher stream (can be NULL)
 *  \param[in] m Multiplexing mode (0 or 1)
 */
void
gmr1_tch3_encode(ubit_t *bits_e,
                 const uint8_t *frame0, const uint8_t *frame1,
                 const ubit_t *bits_s, const ubit_t *ciph, int m)
{
	ubit_t bits_epp[208];
	ubit_t bits_xmy[208];
	int i, j;

	for (i=0; i<2; i++)
	{
		const uint8_t *frame = i ? frame1 : frame0;

		ubit_t bits_d[80];
		ubit_t bits_c[104];
		ubit_t bits_ep[104];

		int kc;

		osmo_pbit2ubit(bits_d, frame, 80);

		osmo_conv_encode(&gmr1_conv_tch3_speech, bits_c, bits_d);
		memcpy(bits_c+72, bits_d+48, 32);

		for (kc=0; kc<104; kc++) {
			int ii, ij, kep;
			ii = kc % 24;
			ij = kc / 24;
			kep = (ii < 8) ? (ij + 5*ii) : (ij + 4*ii + 8);
			bits_ep[kep] = bits_c[kc];
		}

		if (m) {
			for (j=0; j<104; j++)
				bits_epp[(104*i)+j] = bits_ep[j];
		} else {
			for (j=0; j<104; j++)
				bits_epp[(j<<1)+i] = bits_ep[j];
		}
	}

	gmr1_scramble_ubit(bits_xmy, bits_epp, 208);

	if (ciph) {
		for (i=0; i<208; i++)
			bits_xmy[i] ^= ciph[i];
	}

	memcpy(bits_e,    bits_xmy,     52);
	memcpy(bits_e+52, bits_s,        4);
	memcpy(bits_e+56, bits_xmy+52, 156);
}

/*! \brief Stateless GMR-1 TCH3 channel decoder
 *  \param[out] frame0 1st speech frame (10 byte / 80 bits, msb first)
 *  \param[out] frame1 2nd speech frame (10 byte / 80 bits, msb first)
 *  \param[out] bits_s 4 status bits that were demultiplexed
 *  \param[in] bits_e 212 softbits demodulated from a burst
 *  \param[in] ciph 208 bits of cipher stream (can be NULL)
 *  \param[in] m Multiplexing mode (0 or 1)
 *  \param[out] conv0_rv Return of the conv. decode of frame 0 (can be NULL)
 *  \param[out] conv1_rv Return of the conv. decode of frame 1 (can be NULL)
 *  \return 0 if CRC check pass, any other value for fail.
 */
void
gmr1_tch3_decode(uint8_t *frame0, uint8_t *frame1, ubit_t *bits_s,
                 const sbit_t *bits_e, const ubit_t *ciph, int m,
                 int *conv0_rv, int *conv1_rv)
{
	sbit_t bits_xmy[208];
	sbit_t bits_epp[208];
	int rv, i, j;

	for (i=0; i<4; i++)
		bits_s[i] = bits_e[52+i] < 0;

	memcpy(bits_xmy,    bits_e,     52);
	memcpy(bits_xmy+52, bits_e+56, 156);

	if (ciph) {
		for (i=0; i<208; i++)
			if (ciph[i])
				bits_xmy[i] *= -1;
	}

	gmr1_scramble_sbit(bits_epp, bits_xmy, 208);

	for (i=0; i<2; i++)
	{
		int *conv_rv = i ? conv1_rv : conv0_rv;
		uint8_t *frame = i ? frame1 : frame0;

		sbit_t bits_ep[104];
		sbit_t bits_c[104];
		ubit_t bits_d[80];

		int kc;

		if (m) {
			for (j=0; j<104; j++)
				bits_ep[j] = bits_epp[(104*i)+j];
		} else {
			for (j=0; j<104; j++)
				bits_ep[j] = bits_epp[(j<<1)+i];
		}

		for (kc=0; kc<104; kc++) {
			int ii, ij, kep;
			ii = kc % 24;
			ij = kc / 24;
			kep = (ii < 8) ? (ij + 5*ii) : (ij + 4*ii + 8);
			bits_c[kc] = bits_ep[kep];
		}

		rv = osmo_conv_decode(&gmr1_conv_tch3_speech, bits_c, bits_d);
		if (conv_rv)
			*conv_rv = rv;

		for (j=48; j<80; j++)
			bits_d[j] = bits_c[j+24] < 0;

		osmo_ubit2pbit(frame, bits_d, 80);
	}
}

/*! @} */
