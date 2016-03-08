/* GMR-1 AMBE vocoder - Tone frames */

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

/*! \file codec/tone.c
 *  \brief Osmocom GMR-1 AMBE vocoder tone frames handling
 */

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "private.h"


/*! \brief Structure describing a dual-frequency tone */
struct tone_desc {
	char *name;	/*!< \brief Tone description */
	int f1;		/*!< \brief Frequency 1 (Hz) */
	int f2;		/*!< \brief Frequency 2 (Hz) */
};

/*! \brief DTMF tones descriptions */
static const struct tone_desc dtmf_tones[] = {
	{ "1", 1209, 697 },
	{ "4", 1209, 770 },
	{ "7", 1209, 852 },
	{ "*", 1209, 941 },
	{ "2", 1336, 697 },
	{ "5", 1336, 770 },
	{ "8", 1336, 852 },
	{ "0", 1336, 941 },
	{ "3", 1477, 697 },
	{ "6", 1477, 770 },
	{ "9", 1477, 852 },
	{ "#", 1477, 941 },
	{ "A", 1633, 697 },
	{ "B", 1633, 770 },
	{ "C", 1633, 852 },
	{ "D", 1633, 941 },
};

/*! \brief KNOX tones descriptions */
static const struct tone_desc knox_tones[] = {
	{ "1", 1052, 606 },
	{ "4", 1052, 672 },
	{ "7", 1052, 743 },
	{ "*", 1052, 820 },
	{ "2", 1162, 606 },
	{ "5", 1162, 672 },
	{ "8", 1162, 743 },
	{ "0", 1162, 820 },
	{ "3", 1297, 606 },
	{ "6", 1297, 672 },
	{ "9", 1297, 743 },
	{ "#", 1297, 820 },
	{ "A", 1430, 606 },
	{ "B", 1430, 672 },
	{ "C", 1430, 743 },
	{ "D", 1430, 820 },
};

/*! \brief Call progress tones descriptions */
static const struct tone_desc call_progress_tones[] = {
	{ "Dial", 440, 350 },
	{ "Ring", 480, 440 },
	{ "Busy", 630, 480 },
	{ "????", 490, 350 },
};


/*! \brief Synthesize and add a tone to a given audio buffer
 *  \param[out] audio Audio buffer to mix the tone into
 *  \param[in] N number of audio samples to generate
 *  \param[in] ampl Tone amplitude
 *  \param[in] freq_hz Tone frequency in Hertz
 *  \param[inout] phase_p Pointer to phase variable to use
 */
static void
tone_gen(int16_t *audio, int N, int ampl, int freq_hz, float *phase_p)
{
	float phase, phase_step;
	int i;

	phase = *phase_p;
	phase_step = (2.0f * M_PIf * freq_hz) / AMBE_RATE;

	for (i=0; i<N; i++)
	{
		audio[i] += (int16_t)(ampl * cosf(phase));
		phase += phase_step;
	}

	*phase_p = phase;
}


/*! \brief Decodes an AMBE tone frame
 *  \param[in] dec AMBE decoder state
 *  \param[out] audio Output audio buffer
 *  \param[in] N number of audio samples to produce (152..168)
 *  \param[in] frame Frame data (10 bytes = 80 bits). Must be tone frame !
 *  \returns 0 for success. -EINVAL if frame was invalid.
 */
int
ambe_decode_tone(struct ambe_decoder *dec,
                 int16_t *audio, int N, const uint8_t *frame)
{
	int p_sf_sel, p_log_ampl, p_freq;
	int i, j, cnt;
	int start, stop;
	int amplitude;

	/* Decode parameters */
	p_sf_sel   = frame[0] & 3;
	p_log_ampl = frame[1];

	p_freq = 0;
	for (i=0; i<8; i++) {
		cnt = 0;
		for (j=0; j<8; j++)
			cnt += (frame[j] >> (7-i)) & 1;
		p_freq = (p_freq << 1) | (cnt >= 4);
	}

	/* Clear audio */
	memset(audio, 0x00, sizeof(int16_t) * N);

	/* Audio start / stop */
	start = (p_sf_sel & 2) ? 0 : N >> 1;
	stop  = (p_sf_sel & 1) ? (N-1) : ((N >> 1) - 1);

	if (start >= stop)
		return 0;

	/* Compute amplitude */
	amplitude = (int)(32767.0f * exp2f(((float)p_log_ampl-255.0f)/17.0f));

	/* Interpret frequency code */
	if (p_freq == 0xff)
	{
		/* Inactive, nothing to do */
	}
	else if ((p_freq >= 0xa0) && (p_freq <= 0xa3))
	{
		/* Call progress tone */
		int cpi = p_freq & 0xf;

		tone_gen(&audio[start], stop-start+1, amplitude >> 1,
		         call_progress_tones[cpi].f1, &dec->tone_phase_f1);
		tone_gen(&audio[start], stop-start+1, amplitude >> 1,
		         call_progress_tones[cpi].f2, &dec->tone_phase_f2);
	}
	else if ((p_freq >= 0x90) && (p_freq <= 0x9f))
	{
		/* Knox tone */
		int ki = p_freq & 0xf;

		tone_gen(&audio[start], stop-start+1, amplitude >> 1,
		         knox_tones[ki].f1, &dec->tone_phase_f1);
		tone_gen(&audio[start], stop-start+1, amplitude >> 1,
		         knox_tones[ki].f2, &dec->tone_phase_f2);
	}
	else if ((p_freq >= 0x80) && (p_freq <= 0x8f))
	{
		/* DTMF tone */
		int di = p_freq & 0xf;

		tone_gen(&audio[start], stop-start+1, amplitude >> 1,
		         dtmf_tones[di].f1, &dec->tone_phase_f1);
		tone_gen(&audio[start], stop-start+1, amplitude >> 1,
		         dtmf_tones[di].f2, &dec->tone_phase_f2);
	}
	else if (p_freq < 0x7f)
	{
		int freq_hz = (p_freq * 125) >> 2; /* 31.25 Hz increments */

		tone_gen(&audio[start], stop-start+1, amplitude,
		         freq_hz, &dec->tone_phase_f1);
	}
	else
	{
		/* Invalid */
		return -EINVAL;
	}

	return 0;
}

/*! @} */
