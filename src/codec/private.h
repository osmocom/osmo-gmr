/* GMR-1 AMBE vocoder private header */

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

#ifndef __OSMO_GMR1_CODEC_PRIVATE_H__
#define __OSMO_GMR1_CODEC_PRIVATE_H__

/*! \defgroup codec/private AMBE vocoder - internal API
 *  \ingroup codec/private
 *  @{
 */

/*! \file codec/private.h
 *  \brief Osmocom GMR-1 AMBE vocoder private header
 */

#include <stdint.h>

#include "mbelib.h"


#define AMBE_RATE 8000		/*< \brief AMBE sample rate */


/*! \brief AMBE possible frame types */
enum ambe_frame_type
{
	AMBE_SPEECH,		/*< \brief Speed frame */
	AMBE_SILENCE,		/*< \brief Silence indication frame */
	AMBE_TONE,		/*< \brief Tone frame  */
};

/*! \brief AMBE encoded frame raw parameters */
struct ambe_raw_params
{
	uint8_t pitch;		/*!< \brief Pitch */
	uint8_t pitch_interp;	/*!< \brief Pitch interpolation selection */
	uint8_t gain;		/*!< \brief Gain VQ */
	uint8_t v_uv;		/*!< \brief V/UV decision VQ */

	uint8_t sf1_prba12;	/*!< \brief sf1 PRBA[1,2] VQ   */
	uint8_t sf1_prba34;	/*!< \brief sf1 PRBA[3,4] VQ   */
	uint8_t sf1_prba57;	/*!< \brief sf1 PRBA[5,6,7] VQ */
	uint8_t sf1_hoc[4];	/*!< \brief sf1 HOCs VQ        */

	uint8_t sf0_mag_interp;	/*!< \brief sf0 mag interpolation selection   */
	uint8_t sf0_perr_14;	/*!< \brief sf0 mag prediction error VQ [1,4] */
	uint8_t sf0_perr_58;	/*!< \brief sf0 mag prediction error VQ [5,8] */
};

/*! \brief AMBE subframe parameters */
struct ambe_subframe
{
	float f0;               /*!< \brief fundamental normalized frequency */
	int L;                  /*!< \brief Number of harmonics */
	int Lb[4];              /*!< \brief Harmonics per block */
	int v_uv[8];            /*!< \brief Voicing state */
	float gain;		/*!< \brief Gain */
	float Mlog[56];         /*!< \brief log spectral magnitudes */
};

/*! \brief AMBE decoder state */
struct ambe_decoder
{
	float tone_phase_f1;	/*!< \brief Phase frequency 1 for tone frames */
	float tone_phase_f2;	/*!< \brief Phase frequency 2 for tone frames */

	struct ambe_subframe sf_prev;	/*!< \brief Previous subframe */

	mbe_parms mp_cur;	/*!< \brief mbelib current frame */
	mbe_parms mp_prev;	/*!< \brief mbelib previous frame */
	mbe_parms mp_prev_enh;	/*!< \brief mbelib previous frame (enhanced) */
};

/* From ambe.c */
void ambe_decode_init(struct ambe_decoder *dec);
void ambe_decode_fini(struct ambe_decoder *dec);

int ambe_decode_frame(struct ambe_decoder *dec,
                      int16_t *audio, int N,
                      const uint8_t *frame, int bad);
int ambe_decode_dtx(struct ambe_decoder *dec,
                    int16_t *audio, int N);

/* From frame.c */
void ambe_frame_unpack_raw(struct ambe_raw_params *rp, const uint8_t *frame);
int  ambe_frame_decode_params(struct ambe_subframe *sf,
                              struct ambe_subframe *sf_prev,
                              struct ambe_raw_params *rp);

/* From math.c */
float cosf_fast(float angle);
void ambe_fdct(float *out, float *in, int N, int M);
void ambe_idct(float *out, float *in, int N, int M);

/* From tables.c */
extern const uint8_t ambe_hpg_tbl[48][4];
extern const float ambe_gain_tbl[256][2];
extern const uint16_t ambe_v_uv_tbl[64];
extern const float ambe_prba12_tbl[128][2];
extern const float ambe_prba34_tbl[64][2];
extern const float ambe_prba57_tbl[128][3];
extern const float ambe_hoc0_tbl[128][4];
extern const float ambe_hoc1_tbl[64][4];
extern const float ambe_hoc2_tbl[64][4];
extern const float ambe_hoc3_tbl[64][4];
extern const float ambe_sf0_interp_tbl[4][2];
extern const float ambe_sf0_perr14_tbl[64][4];
extern const float ambe_sf0_perr58_tbl[32][4];

/* From tone.c */
int ambe_decode_tone(struct ambe_decoder *dec,
                     int16_t *audio, int N, const uint8_t *frame);


/*! @} */

#endif /* __OSMO_GMR1_CODEC_PRIVATE_H__ */
