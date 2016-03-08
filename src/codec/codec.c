/* GMR-1 AMBE vocoder */

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

/*! \addtogroup codec
 *  @{
 */

/*! \file codec/codec.c
 *  \brief Osmocom GMR-1 AMBE vocoder public API implementation
 */

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <osmocom/gmr1/codec/codec.h>

#include "private.h"


/*! \brief Structure for GMR1 codec state */
struct gmr1_codec
{
	struct ambe_decoder dec;	/*!< \brief Decoder state */
};


/*! \brief Allocates and inits a codec object
 *  \returns A newly allocated codec, to be freed with \ref gmr1_codec_release
 */
struct gmr1_codec *
gmr1_codec_alloc(void)
{
	struct gmr1_codec *codec;

	codec = calloc(1, sizeof(struct gmr1_codec));
	if (!codec)
		return NULL;

	ambe_decode_init(&codec->dec);

	return codec;
}

/*! \brief Release a codec object created by \ref gmr1_codec_alloc
 *  \param[in] codec The codec object to release
 */
void
gmr1_codec_release(struct gmr1_codec *codec)
{
	if (!codec)
		return;

	ambe_decode_fini(&codec->dec);

	free(codec);
}

/*! \brief Decodes an AMBE frame to audio
 *  \param[in] codec Codec object
 *  \param[out] audio Output audio buffer
 *  \param[in] N number of audio samples to produce (152..168)
 *  \param[in] frame Frame data (10 bytes = 80 bits)
 *  \param[in] bad Bad Frame Indicator. Set to 1 if frame is corrupt
 *  \returns 0 for success. Negative error code otherwise.
 */
int
gmr1_codec_decode_frame(struct gmr1_codec *codec,
                        int16_t *audio, int N,
                        const uint8_t *frame, int bad)
{
	return ambe_decode_frame(&codec->dec, audio, N, frame, bad);
}

/*! \brief Generates audio for DTX period
 *  \param[in] codec Codec object
 *  \param[out] audio Output audio buffer
 *  \param[in] N number of audio samples to produce (152..168)
 */
int
gmr1_codec_decode_dtx(struct gmr1_codec *codec,
                      int16_t *audio, int N)
{
	return ambe_decode_dtx(&codec->dec, audio, N);
}

/*! @} */
