/* GMR-1 AMBE vocoder - Speech synthesis */

/* (C) 2014 by Sylvain Munaut <tnt@246tNt.com>
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

/*! \file codec/synth.c
 *  \brief Osmocom GMR-1 AMBE vocoder speech synthesis
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "private.h"

/*! \brief Synthesis window (39 samples overlap) */
static const float ws[] = {
	0.000f, 0.025f, 0.050f, 0.075f, 0.100f, 0.125f, 0.150f, 0.175f,
	0.200f, 0.225f, 0.250f, 0.275f, 0.300f, 0.325f, 0.350f, 0.375f,
	0.400f, 0.425f, 0.450f, 0.475f, 0.500f, 0.525f, 0.550f, 0.575f,
	0.600f, 0.625f, 0.650f, 0.675f, 0.700f, 0.725f, 0.750f, 0.775f,
	0.800f, 0.825f, 0.850f, 0.875f, 0.900f, 0.925f, 0.950f, 0.975f,
	1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f,
	1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f,
	1.000f, 1.000f, 1.000f, 1.000f,
	1.000f,
	1.000f, 1.000f, 1.000f, 1.000f,
	1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f,
	1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f, 1.000f,
	0.975f, 0.950f, 0.925f, 0.900f, 0.875f, 0.850f, 0.825f, 0.800f,
	0.775f, 0.750f, 0.725f, 0.700f, 0.675f, 0.650f, 0.625f, 0.600f,
	0.575f, 0.550f, 0.525f, 0.500f, 0.475f, 0.450f, 0.425f, 0.400f,
	0.375f, 0.350f, 0.325f, 0.300f, 0.275f, 0.250f, 0.225f, 0.200f,
	0.175f, 0.150f, 0.125f, 0.100f, 0.075f, 0.050f, 0.025f, 0.000f,
};

/*! \brief Pitch refinement window */
static const float wr[] = {
	0.014873f, 0.019270f, 0.024182f, 0.029633f, 0.035694f, 0.042353f,
	0.049618f, 0.057539f, 0.066135f, 0.075386f, 0.085322f, 0.095993f,
	0.107350f, 0.119398f, 0.132219f, 0.145736f, 0.159947f, 0.174903f,
	0.190559f, 0.206884f, 0.223901f, 0.241592f, 0.259901f, 0.278822f,
	0.298359f, 0.318434f, 0.339018f, 0.360127f, 0.381668f, 0.403604f,
	0.425920f, 0.448547f, 0.471431f, 0.494535f, 0.517800f, 0.541163f,
	0.564575f, 0.587971f, 0.611295f, 0.634490f, 0.657474f, 0.680203f,
	0.702622f, 0.724637f, 0.746204f, 0.767276f, 0.787766f, 0.807607f,
	0.826778f, 0.845205f, 0.862786f, 0.879533f, 0.895400f, 0.910234f,
	0.924090f, 0.936931f, 0.948628f, 0.959200f, 0.968647f, 0.976876f,
	0.983859f, 0.989640f, 0.994168f, 0.997358f, 0.999304f,
	1.000000f,
	0.999304f, 0.997358f, 0.994168f, 0.989640f, 0.983859f,
	0.976876f, 0.968647f, 0.959200f, 0.948628f, 0.936931f, 0.924090f,
	0.910234f, 0.895400f, 0.879533f, 0.862786f, 0.845205f, 0.826778f,
	0.807607f, 0.787766f, 0.767276f, 0.746204f, 0.724637f, 0.702622f,
	0.680203f, 0.657474f, 0.634490f, 0.611295f, 0.587971f, 0.564575f,
	0.541163f, 0.517800f, 0.494535f, 0.471431f, 0.448547f, 0.425920f,
	0.403604f, 0.381668f, 0.360127f, 0.339018f, 0.318434f, 0.298359f,
	0.278822f, 0.259901f, 0.241592f, 0.223901f, 0.206884f, 0.190559f,
	0.174903f, 0.159947f, 0.145736f, 0.132219f, 0.119398f, 0.107350f,
	0.095993f, 0.085322f, 0.075386f, 0.066135f, 0.057539f, 0.049618f,
	0.042353f, 0.035694f, 0.029633f, 0.024182f, 0.019270f, 0.014873f,
};

/*! \brief Random phase increment (precomputed) */
static const float rho[] = {
	 3.002978f, -0.385743f, -1.804058f,  0.708389f,  3.080091f,  0.234237f,
	-2.601564f,  2.564900f,  0.101063f, -0.241570f, -2.283176f,  0.460491f,
	-1.611275f,  2.258339f, -2.055267f,  1.733923f,  2.517236f, -1.766211f,
	 0.897032f, -2.360999f, -0.280836f, -2.714514f,  2.100092f,  2.300326f,
	-1.158767f, -2.044268f, -2.668387f, -2.578737f,  0.185036f,  1.551429f,
	 2.726814f,  2.655614f,  3.046857f,  0.834348f, -0.513595f,  1.466037f,
	 0.691121f,  0.127319f, -2.034924f, -1.070655f,  0.456588f, -2.278682f,
	 1.229021f, -2.139595f, -0.119750f, -0.301534f,  0.029391f,  0.068775f,
	 0.520336f,  2.339119f, -0.808328f,  1.332154f,  2.929768f, -0.338316f,
	 0.022767f, -1.063795f,
};


/*! \brief Generates random sequence of uint16_t according to spec
 *  \param[out] u_seq Result buffer
 *  \param[in] u_prev Last 'u' value of where to resume from
 *  \param[in] n Number of items to generate
 */
static void
ambe_gen_random(uint16_t *u_seq, uint16_t u_prev, int n)
{
	uint32_t u = u_prev;
	int i;

	for (i=0; i<n; i++) {
		u = (u * 171 + 11213) % 53125;
		u_seq[i] = u;
	}
}

/*! \brief Perform unvoiced synthesis
 *  \param[in] synth Synthesizer state structure
 *  \param[out] suv Result buffer (80 samples)
 *  \param[in] sf Expanded subframe data
 */
static void
ambe_synth_unvoiced(struct ambe_synth *synth, float *suv, struct ambe_subframe *sf)
{
	uint16_t u[121];
	float uw[121];
	float Uwi[65], Uwq[65];
	int i, al, bl, l;

	/* Generate the white noise sequence and window it with ws */
	ambe_gen_random(u, synth->u_prev, 121);
	synth->u_prev = u[79];

	for (i=0; i<121; i++)
		uw[i] = (float)u[i] * ws[i];

	/* Compute the DFT */
	ambe_fdft_fc(Uwi, Uwq, uw, 128, 121);

	/* Apply the spectral magnitude */
	bl = ceilf(128.0f / (2 * M_PIf) * (.5f) * sf->w0);

	for (i=0; i<bl; i++) {
		Uwi[i] = 0.0f;
		Uwq[i] = 0.0f;
	}

	for (l=0; l<sf->L; l++)
	{
		float ampl;

		/* Edges */
		al = bl;
		bl = ceilf(128.0f / (2 * M_PIf) * (l + 1.5f) * sf->w0);

		/* Compute factor */
		ampl = 0.0f;

		for (i=al; i<bl; i++) {
			ampl += Uwi[i] * Uwi[i] + Uwq[i] * Uwq[i];
		}

		ampl = 76.89f * sf->Ml[l] / sqrtf(ampl / (bl - al));

		/* Set magnitude */
		for (i=al; i<bl; i++) {
			if (sf->Vl[l]) {
				Uwi[i] = 0.0f;
				Uwq[i] = 0.0f;
			} else {
				Uwi[i] *= ampl;
				Uwq[i] *= ampl;
			}
		}
	}

	for (i=bl; i<=64; i++) {
		Uwi[i] = 0.0f;
		Uwq[i] = 0.0f;
	}

	/* Get time-domain samples via iDFT */
	ambe_idft_cf(uw, Uwi, Uwq, 128, 121);

	/* Weighted Overlap And Add */
	for (i=0; i<21; i++) {
		suv[i] = synth->uw_prev[i + 60];
	}

	for (i=21; i<60; i++) {
		suv[i] = (ws[i + 60] * synth->uw_prev[i + 60] + ws[i - 20] * uw[i - 20])
		       / (ws[i + 60] * ws[i + 60] + ws[i - 20] * ws[i - 20]);
	}

	for (i=60; i<80; i++) {
		suv[i] = uw[i - 20];
	}

	memcpy(synth->uw_prev, uw, sizeof(float) * 121);
}

/*! \brief Perform voiced synthesis
 *  \param[in] synth Synthesizer state structure
 *  \param[out] sv Result buffer (80 samples)
 *  \param[in] sf Expanded subframe data for current subframe
 *  \param[in] sf_prev Expanded subframe data for prevous subframe
 */
static void
ambe_synth_voiced(struct ambe_synth *synth, float *sv,
                  struct ambe_subframe *sf, struct ambe_subframe *sf_prev)
{
	int i, l, L_max, L_uv;

	/* Pre-clear */
	memset(sv, 0x00, sizeof(float) * 80);

	/* How many subband to process */
	L_max = sf_prev->L > sf->L ? sf_prev->L : sf->L;

	/* psi update */
	L_uv = 0;
	for (l=0; l<L_max; l++)
		L_uv += sf->Vl[l] ? 0 : 1;

	synth->psi1 = remainderf(synth->psi1 + (sf->w0 + sf_prev->w0) * 40.0f, 2 * M_PIf);

	/* Scan each band */
	for (l=0; l<L_max; l++)
	{
		int   Vl_cur,  Vl_prev;
		float Ml_cur,  Ml_prev;
		float phi_cur, phi_prev;
		float w_cur,   w_prev;
		int fine;

		/* Handle out-of-bound for Vl and Ml */
		Vl_cur  = l >= sf->L      ? 0 : sf->Vl[l];
		Vl_prev = l >= sf_prev->L ? 0 : sf_prev->Vl[l];

		Ml_cur  = l >= sf->L      ? 0.0f : sf->Ml[l];
		Ml_prev = l >= sf_prev->L ? 0.0f : sf_prev->Ml[l];

		/* Phase and Angular speed */
		w_cur   = (l+1) * sf->w0;
		w_prev  = (l+1) * sf_prev->w0;

		phi_prev = synth->phi[l];
		phi_cur  = synth->psi1 * (l+1);

		if (l >= (sf->L / 4))
			phi_cur += ((float)L_uv / (float)sf->L) * rho[l];

		synth->phi[l] = phi_cur;

		/* Actual synthesis */
			/* Can we do a fine transistion ? */
		fine = Vl_cur && Vl_prev && (l < 7) && (fabsf(w_cur - w_prev) < (.1f * w_cur));

			/* Fine transition */
		if (fine)
		{
			float Ml_step = (Ml_cur - Ml_prev) / 80.0f;
			float Dpl = phi_cur - phi_prev - (w_cur + w_prev) * 40.0f;
			float Dwl = (Dpl - 2 * M_PIf * floorf((Dpl + M_PIf) / (2 * M_PIf))) / 80.0f;
			float THa = w_prev + Dwl;
			float THb = (w_cur - w_prev) / 160.0f;

			for (i=0; i<80; i++)
				sv[i] += (Ml_prev + i * Ml_step) * cosf_fast(
					phi_prev + (THa + THb * i) * i
				);
		}

			/* Coarse transition: Current frame (if voiced) */
		if (!fine && Vl_cur)
		{
			for (i=21; i<80; i++)
				sv[i] += ws[i-20] * Ml_cur * cosf_fast(phi_cur + w_cur * (i - 80));
		}

			/* Coarse transition: Previous frame (if voiced) */
		if (!fine && Vl_prev)
		{
			for (i=0; i<60; i++)
				sv[i] += ws[i+60] * Ml_prev * cosf_fast(phi_prev + w_prev * i);
		}
	}

	/* Still need to update phi for the rest of the bands */
	for (l=L_max; l<56; l++)
		synth->phi[l] = (synth->psi1 * (l+1)) + (((float)L_uv / (float)sf->L) * rho[l]);
}


/*! \brief Initialized Synthesizer state
 *  \param[out] synth The structure to reset
 */
void
ambe_synth_init(struct ambe_synth *synth)
{
	memset(synth, 0x00, sizeof(struct ambe_synth));
	synth->u_prev = 3147;
}

/*! \brief Apply the spectral magnitude enhancement on the subframe
 *  \param[in] synth Synthesizer state structure
 *  \param[in] sf Expanded subframe data for subframe to enhance
 */
void
ambe_synth_enhance(struct ambe_synth *synth, struct ambe_subframe *sf)
{
	float rm0, rm1;
	float k1, k2, k3;
	float gamma;
	int l;

	/* Compute RM0 and RM1 */
	rm0 = 0.0f;
	rm1 = 0.0f;

	for (l=0; l<sf->L; l++)
	{
		float sq = sf->Ml[l] * sf->Ml[l];
		rm0 += sq;
		rm1 += sq * cosf_fast(sf->w0 * (l+1));
	}

	/* Pre compute some constants */
	k1 = 0.96f * M_PIf / (sf->w0 * rm0 * (rm0 * rm0 - rm1 * rm1));
	k2 = rm0 * rm0 + rm1 * rm1;
	k3 = 2.0f * rm0 * rm1;

	/* Apply to the amplitudes */
	gamma = 0.0f;

	for (l=0; l<sf->L; l++)
	{
		float w;

		if ( (l+1)*8 <= sf->L ) {
			w = 1.0f;
		} else {
			w = sqrtf(sf->Ml[l]) * powf(
				k1 * (k2 - k3 * cosf_fast(sf->w0 * (l+1))),
				0.25f
			);

			if (w > 1.2f)
				w = 1.2f;
			else if (w < 0.5f)
				w = 0.5f;
		}

		sf->Ml[l] *= w;

		gamma += sf->Ml[l] * sf->Ml[l];
	}

	/* Compute final gamma and apply it */
	gamma = sqrtf(rm0 / gamma);

	for (l=0; l<sf->L; l++)
	{
		sf->Ml[l] *= gamma;
	}

	/* Update SE */
	synth->SE = 0.95f * synth->SE + 0.05f * rm0;
	if (synth->SE < 1e4f)
		synth->SE = 1e4f;
}

/*! \brief Generate audio for a given subframe
 *  \param[in] synth Synthesizer state structure
 *  \param[out] audio Result buffer (80 samples)
 *  \param[in] sf Expanded subframe data for current subframe
 *  \param[in] sf_prev Expanded subframe data for prevous subframe
 */
void
ambe_synth_audio(struct ambe_synth *synth, int16_t *audio,
                 struct ambe_subframe *sf,
                 struct ambe_subframe *sf_prev)
{
	float suv[80], sv[80];
	int i;

	ambe_synth_unvoiced(synth, suv, sf);
	ambe_synth_voiced(synth, sv, sf, sf_prev);
	for (i=0; i<80; i++)
		audio[i] = (int16_t)((suv[i] + 2.0f * sv[i]) * 4.0f);
}

/*! @} */
