/* GMR-1 puncturing */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V3.3.1) - Section 4.5 */

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

#ifndef __OSMO_GMR1_L1_PUNCT_H__
#define __OSMO_GMR1_L1_PUNCT_H__

/*! \defgroup punct Puncturing
 *  \ingroup l1_prim
 *  @{
 */

/*! \file l1/punct.h
 *  \brief Osmocom GMR-1 puncturing header
 */

#include <stdint.h>


/*! \brief structure describing a puncturing code */
struct gmr1_puncturer {
	int r; /*!< \brief Number of punctured bits */
	int L; /*!< \brief Length of the puncturing mask */
	int N; /*!< \brief Rate of the punctured convolutional code (1/N) */
	const uint8_t mask[]; /*!< \brief Puncturing mask */
};


struct osmo_conv_code;

int gmr1_puncturer_generate(struct osmo_conv_code *code,
                            const struct gmr1_puncturer *punct_pre,
                            const struct gmr1_puncturer *punct_main,
                            const struct gmr1_puncturer *punct_post,
                            int repeat);


/* Various puncturing codes used in GMR-1 */

extern const struct gmr1_puncturer gmr1_punct_k5_12_P23;
extern const struct gmr1_puncturer gmr1_punct_k5_12_P25;
extern const struct gmr1_puncturer gmr1_punct_k5_12_Ps25;
extern const struct gmr1_puncturer gmr1_punct_k5_12_P311;
extern const struct gmr1_puncturer gmr1_punct_k5_12_P412;
extern const struct gmr1_puncturer gmr1_punct_k5_12_Ps412;
extern const struct gmr1_puncturer gmr1_punct_k5_12_P12;
extern const struct gmr1_puncturer gmr1_punct_k5_12_Ps12;
extern const struct gmr1_puncturer gmr1_punct_k5_12_A;
extern const struct gmr1_puncturer gmr1_punct_k5_12_B;
extern const struct gmr1_puncturer gmr1_punct_k5_12_C;
extern const struct gmr1_puncturer gmr1_punct_k5_12_D;
extern const struct gmr1_puncturer gmr1_punct_k5_12_E;
extern const struct gmr1_puncturer gmr1_punct_k5_12_P38;
extern const struct gmr1_puncturer gmr1_punct_k5_12_P26;
extern const struct gmr1_puncturer gmr1_punct_k5_12_P37;
extern const struct gmr1_puncturer gmr1_punct_k5_13_P16;
extern const struct gmr1_puncturer gmr1_punct_k5_13_P25;
extern const struct gmr1_puncturer gmr1_punct_k5_13_P15;
extern const struct gmr1_puncturer gmr1_punct_k5_13_Ps15;
extern const struct gmr1_puncturer gmr1_punct_k5_13_P78;
extern const struct gmr1_puncturer gmr1_punct_k5_15_P23;
extern const struct gmr1_puncturer gmr1_punct_k5_15_P53;
extern const struct gmr1_puncturer gmr1_punct_k5_15_Ps53;
extern const struct gmr1_puncturer gmr1_punct_k7_12_P23;
extern const struct gmr1_puncturer gmr1_punct_k7_12_P410;
extern const struct gmr1_puncturer gmr1_punct_k7_12_P512;
extern const struct gmr1_puncturer gmr1_punct_k7_12_P116;
extern const struct gmr1_puncturer gmr1_punct_k7_12_P148;
extern const struct gmr1_puncturer gmr1_punct_k7_12_P184;
extern const struct gmr1_puncturer gmr1_punct_k7_12_P1152;
extern const struct gmr1_puncturer gmr1_punct_k7_12_P45;
extern const struct gmr1_puncturer gmr1_punct_k7_12_P245;
extern const struct gmr1_puncturer gmr1_punct_k9_12_P13;
extern const struct gmr1_puncturer gmr1_punct_k9_12_P47;
extern const struct gmr1_puncturer gmr1_punct_k9_12_P34;
extern const struct gmr1_puncturer gmr1_punct_k9_12_P17;
extern const struct gmr1_puncturer gmr1_punct_k9_12_P19;
extern const struct gmr1_puncturer gmr1_punct_k9_12_P26;
extern const struct gmr1_puncturer gmr1_punct_k9_12_P110;
extern const struct gmr1_puncturer gmr1_punct_k9_12_P14;
extern const struct gmr1_puncturer gmr1_punct_k9_12_P45;
extern const struct gmr1_puncturer gmr1_punct_k9_12_P234;
extern const struct gmr1_puncturer gmr1_punct_k6_14_P45;
extern const struct gmr1_puncturer gmr1_punct_k9_14_P148;
extern const struct gmr1_puncturer gmr1_punct_k9_14_P65;
extern const struct gmr1_puncturer gmr1_punct_k9_13_P12;
extern const struct gmr1_puncturer gmr1_punct_k9_13_P1213;
extern const struct gmr1_puncturer gmr1_punct_k9_13_P44;
extern const struct gmr1_puncturer gmr1_punct_k9_13_P33;
extern const struct gmr1_puncturer gmr1_punct_k9_13_P65;


/*! @} */

#endif /* __OSMO_GMR1_L1_PUNCT_H__ */
