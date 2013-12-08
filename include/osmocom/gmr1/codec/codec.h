/* GMR-1 AMBE vocoder */

/* (C) 2013 by Sylvain Munaut <tnt@246tNt.com>
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

#ifndef __OSMO_GMR1_CODEC_H__
#define __OSMO_GMR1_CODEC_H__

/*! \defgroup codec AMBE vocoder
 *  \ingroup codec
 *  @{
 */

/*! \file codec/codec.h
 *  \brief Osmocom GMR-1 AMBE vocoder header
 */

#include <stdint.h>


struct gmr1_codec;

struct gmr1_codec *gmr1_codec_alloc(void);
void               gmr1_codec_release(struct gmr1_codec *codec);

int gmr1_codec_decode_frame(struct gmr1_codec *codec,
                            int16_t *audio, int N,
                            const uint8_t *frame, int bad);

int gmr1_codec_decode_dtx(struct gmr1_codec *codec,
                          int16_t *audio, int N);


/*! @} */

#endif /* __OSMO_GMR1_CODEC_H__ */
