/* GMR-1 puncturing */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 4.5 */

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

extern const struct gmr1_puncturer gmr1_punct12_P12;
extern const struct gmr1_puncturer gmr1_punct12_P23;
extern const struct gmr1_puncturer gmr1_punct12_P25;
extern const struct gmr1_puncturer gmr1_punct12_Ps25;
extern const struct gmr1_puncturer gmr1_punct12_P311;
extern const struct gmr1_puncturer gmr1_punct12_P412;
extern const struct gmr1_puncturer gmr1_punct12_Ps412;
extern const struct gmr1_puncturer gmr1_punct13_P25;
extern const struct gmr1_puncturer gmr1_punct13_P15;
extern const struct gmr1_puncturer gmr1_punct13_Ps15;
extern const struct gmr1_puncturer gmr1_punct13_P16;
extern const struct gmr1_puncturer gmr1_punct15_P23;
extern const struct gmr1_puncturer gmr1_punct15_P53;
extern const struct gmr1_puncturer gmr1_punct15_Ps53;


/*! @} */

#endif /* __OSMO_GMR1_L1_PUNCT_H__ */
