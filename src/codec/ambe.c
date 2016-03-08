/* GMR-1 AMBE vocoder - internal API */

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

/*! \addtogroup codec_private
 *  @{
 */

/*! \file codec/ambe.c
 *  \brief Osmocom GMR-1 AMBE internal API
 */

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "private.h"


/*! \brief Initializes decoder state
 *  \param[in] dec Decoder state structure
 */
void
ambe_decode_init(struct ambe_decoder *dec)
{
	memset(dec, 0x00, sizeof(struct ambe_decoder));

	ambe_synth_init(&dec->synth);

	dec->sf_prev.w0 = 0.09378f;
	dec->sf_prev.f0 = dec->sf_prev.w0 / (2 * M_PIf);
	dec->sf_prev.L  = 30;
}

/*! \brief Release all resources associated with a decoder
 *  \param[in] dec Decoder state structure
 */
void
ambe_decode_fini(struct ambe_decoder *dec)
{
	/* nothing to do */
}


/*! \brief Identify the type of frame
 *  \param[in] frame Frame data (10 bytes = 80 bits)
 *  \returns See \ref ambe_frame_type for possible return values
 */
static enum ambe_frame_type
ambe_classify_frame(const uint8_t *frame)
{
	switch (frame[0] & 0xfc) {
	case 0xfc:
		return AMBE_TONE;

	case 0xf8:
		return AMBE_SILENCE;

	default:
		return AMBE_SPEECH;
	};
}

/*! \brief Decodes an AMBE speech frame to audio
 *  \param[in] dec Decoder state structure
 *  \param[out] audio Output audio buffer
 *  \param[in] N number of audio samples to produce (152..168)
 *  \param[in] frame Frame data (10 bytes = 80 bits)
 *  \param[in] bad Bad Frame Indicator. Set to 1 if frame is corrupt
 *  \returns 0 for success. Negative error code otherwise.
 */
static int
ambe_decode_speech(struct ambe_decoder *dec,
                   int16_t *audio, int N,
                   const uint8_t *frame, int bad)
{
	struct ambe_raw_params rp;
	struct ambe_subframe sf[2];

	/* Unpack frame */
	ambe_frame_unpack_raw(&rp, frame);

	/* Decode subframe parameters */
	ambe_frame_decode_params(sf, &dec->sf_prev, &rp);

	/* Expand parameters for synthesis */
	ambe_subframe_expand(&sf[0]);
	ambe_subframe_expand(&sf[1]);

	/* Perform the actual audio synthesis */
	ambe_synth_enhance(&dec->synth, &sf[0]);
	ambe_synth_audio(&dec->synth, audio,      &sf[0], &dec->sf_prev);

	ambe_synth_enhance(&dec->synth, &sf[1]);
	ambe_synth_audio(&dec->synth, audio + 80, &sf[1], &sf[0]);

	/* Save subframe */
	memcpy(&dec->sf_prev, &sf[1], sizeof(struct ambe_subframe));

	/* Done */
	return 0;
}

/*! \brief Decodes an AMBE frame to audio
 *  \param[in] dec Decoder state structure
 *  \param[out] audio Output audio buffer
 *  \param[in] N number of audio samples to produce (152..168)
 *  \param[in] frame Frame data (10 bytes = 80 bits)
 *  \param[in] bad Bad Frame Indicator. Set to 1 if frame is corrupt
 *  \returns 0 for success. Negative error code otherwise.
 */
int
ambe_decode_frame(struct ambe_decoder *dec,
                  int16_t *audio, int N,
                  const uint8_t *frame, int bad)
{
	switch(ambe_classify_frame(frame)) {
		case AMBE_SPEECH:
			return ambe_decode_speech(dec, audio, N, frame, bad);

		case AMBE_SILENCE:
			/* FIXME: Comfort noise */
			memset(audio, 0, 160*sizeof(int16_t));
			return 0;

		case AMBE_TONE:
			return ambe_decode_tone(dec, audio, N, frame);
	}

	return -EINVAL;
}

/*! \brief Generates audio for DTX period
 *  \param[in] dec Decoder state structure
 *  \param[out] audio Output audio buffer
 *  \param[in] N number of audio samples to produce (152..168)
 */
int
ambe_decode_dtx(struct ambe_decoder *dec,
                int16_t *audio, int N)
{
	/* FIXME: Comfort noise */
	memset(audio, 0x00, sizeof(int16_t) * N);
	return 0;
}

/*! @} */
