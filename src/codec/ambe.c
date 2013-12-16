/* GMR-1 AMBE vocoder - internal API */

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

/*! \addtogroup codec/private
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


void
ambe_decode_init(struct ambe_decoder *dec)
{
	memset(dec, 0x00, sizeof(struct ambe_decoder));
	mbe_initMbeParms(&dec->mp_prev);
}

void
ambe_decode_fini(struct ambe_decoder *dec)
{
	/* nothing to do */
}


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

static void
ambe_subframe_to_mbelib(mbe_parms *mp, struct ambe_subframe *sf)
{
	float unvc;
	int i;

	mp->w0 = sf->f0 * (2.0f * (float)M_PI);
	mp->L  = sf->L;

	unvc = 0.2046f / sqrtf(mp->w0);	/* ??? */

	for (i=1; i<=mp->L; i++) {
		int j = (int)((i-1) * 16.0f * sf->f0);
		mp->Vl[i] = sf->v_uv[j];
		mp->Ml[i] = powf(2.0, sf->Mlog[i-1]) / 6.0f;
		if (!mp->Vl[i])
			mp->Ml[i] *= unvc;
	}
}

static int
ambe_decode_speech(struct ambe_decoder *dec,
                   int16_t *audio, int N,
                   const uint8_t *frame, int bad)
{
	struct ambe_raw_params rp;
	struct ambe_subframe sf[2];
	mbe_parms mp[2];

	/* Unpack frame */
	ambe_frame_unpack_raw(&rp, frame);

	/* Decode subframe parameters */
	ambe_frame_decode_params(sf, &dec->sf_prev, &rp);

	/* Convert both subframes to mbelib's format */
	ambe_subframe_to_mbelib(&mp[0], &sf[0]);
	ambe_subframe_to_mbelib(&mp[1], &sf[1]);

	/* Synthesize speech (using mbelib for now) */
	mbe_spectralAmpEnhance(&mp[0]);
	mbe_spectralAmpEnhance(&mp[1]);

	mbe_synthesizeSpeech(audio,    &mp[0], &dec->mp_prev, 2);
	mbe_synthesizeSpeech(audio+80, &mp[1], &mp[0],        2);

	mbe_moveMbeParms(&mp[1], &dec->mp_prev);

	/* Save subframe */
	memcpy(&dec->sf_prev, &sf[1], sizeof(struct ambe_subframe));

	/* Done */
	return 0;
}

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
			/* FIXME: Tone gen */
			memset(audio, 0, 160*sizeof(int16_t));
			return 0;
	}

	return -EINVAL;
}

int
ambe_decode_dtx(struct ambe_decoder *dec,
                int16_t *audio, int N)
{
	/* FIXME: Comfort noise */
	memset(audio, 0x00, sizeof(int16_t) * N);
	return 0;
}

/*! @} */
