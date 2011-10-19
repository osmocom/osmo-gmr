/* GMR-1 SDR - FCCH bursts */
/* See GMR-1 05.003 (ETSI TS 101 376-5-4 V1.2.1) - Section 8.1 */

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

/*! \addtogroup fcch
 *  @{
 */

/*! \file sdr/fcch.c
 *  \brief Osmocom GMR-1 FCCH bursts implementation
 */

#include <complex.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>

#include <fftw3.h>

#include <osmocom/sdr/cxvec.h>
#include <osmocom/sdr/cxvec_math.h>

#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/fcch.h>


/* ------------------------------------------------------------------------ */
/* Reference waveform generation                                            */
/* ------------------------------------------------------------------------ */

/*! \brief Generate FCCH reference up or down chirp at a given oversampling
 *  \param[in] sps Oversampling rate
 *  \param[in] up_down Selects chirp direction (0=down 1=up)
 *  \returns A newly allocated complex vector containing the chirp
 *
 * Up-Chirp: \f$\frac{\sqrt{2}}{2}\cdot e^{j\left(
 * 0.64\pi\left(t-\frac{T}{2}\right)^2/T^2\right)}\f$
 *
 * Down-Chirp: \f$\frac{\sqrt{2}}{2}\cdot e^{-j\left(
 * 0.64\pi\left(t-\frac{T}{2}\right)^2/T^2\right)}\f$
 *
 * The length will be 117 * sps
 */
static struct osmo_cxvec *
gmr1_sdr_fcch_gen_up_down_chirp(int sps, int up_down)
{
	struct osmo_cxvec *cv;
	int i, l;
	float sq2d2, phase_base, pos, halfpos;

	l = GMR1_FCCH_SYMS * sps;

	cv = osmo_cxvec_alloc(l);
	if (!cv)
		return NULL;

	cv->len = l;

	sq2d2 = sqrtf(2.0f) / 2.0f;
	phase_base = 0.64f * M_PIf / (float)(GMR1_FCCH_SYMS);
	halfpos = ((float)(GMR1_FCCH_SYMS)) / 2.0f;

	if (up_down)
		phase_base *= -1.0f;

	for (i=0; i<l; i++) {
		pos = ((float)i / (float)sps) - halfpos;
		cv->data[i] = sq2d2 * cexpf( I * phase_base * (pos * pos) );
	}

	return cv;
}

/*! \brief Generate FCCH reference up chirp at a given oversampling
 *  \param[in] sps Oversampling rate
 *  \returns A newly allocated complex vector containing the chirp
 *
 * \f$\frac{\sqrt{2}}{2}\cdot e^{j\left(
 * 0.64\pi\left(t-\frac{T}{2}\right)^2/T^2\right)}\f$
 *
 * The length will be 117 * sps
 */
static struct osmo_cxvec *
gmr1_sdr_fcch_gen_up_chirp(int sps)
{
	return gmr1_sdr_fcch_gen_up_down_chirp(sps, 0);
}

/*! \brief Generate FCCH reference down chirp at a given oversampling
 *  \param[in] sps Oversampling rate
 *  \returns A newly allocated complex vector containing the chirp
 *
 * \f$\frac{\sqrt{2}}{2}\cdot e^{-j\left(
 * 0.64\pi\left(t-\frac{T}{2}\right)^2/T^2\right)}\f$
 *
 * The length will be 117 * sps
 */
static struct osmo_cxvec *
gmr1_sdr_fcch_gen_down_chirp(int sps)
{
	return gmr1_sdr_fcch_gen_up_down_chirp(sps, 1);
}

/*! \brief Generate FCCH reference dual chirp at a given oversampling
 *  \param[in] sps Oversampling rate
 *  \returns A newly allocated complex vector containing the dual chirp
 *
 * \f$\sqrt{2}\cdot\cos\left(0.64\pi\left(t-\frac{T}{2}\right)^2/\;T^2\right)\f$
 *
 * The length will be 117 * sps. The vector is also 'real only'.
 */
static struct osmo_cxvec *
gmr1_sdr_fcch_gen_dual_chirp(int sps)
{
	struct osmo_cxvec *cv;
	int i, l;
	float sq2, phase_base, pos, halfpos;

	l = GMR1_FCCH_SYMS * sps;

	cv = osmo_cxvec_alloc(l);
	if (!cv)
		return NULL;

	cv->len = l;
	cv->flags |= CXVEC_FLG_REAL_ONLY;

	sq2 = sqrtf(2.0f);
	phase_base = 0.64f * M_PIf / (float)(GMR1_FCCH_SYMS);
	halfpos = ((float)(GMR1_FCCH_SYMS)) / 2.0f;

	for (i=0; i<l; i++) {
		pos = ((float)i / (float)sps) - halfpos;
		cv->data[i] = sq2 * cosf( phase_base * (pos * pos) );
	}

	return cv;
}


/* ------------------------------------------------------------------------ */
/* Raw FCCH detection functions                                             */
/* ------------------------------------------------------------------------ */

/*! \brief Rough FCCH timing acquisition
 *  \param[in] search_win_in Complex signal where to search for FCCH
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] freq_shift Frequency shift to pre-apply to search_win_in (rad/sym)
 *  \param[out] toa Pointer to the toa return variable
 *  \returns 0 in case of success. -errno for errors.
 *
 * To be sure to acquire the signal, you need more than a single BCCH period.
 * (so more than 320 ms of signal, plus the fcch length itself)
 */
int
gmr1_fcch_rough(struct osmo_cxvec *search_win_in, int sps, float freq_shift,
                int *toa)
{
	struct osmo_cxvec *ref = NULL;
	struct osmo_cxvec *search_win = NULL;
	struct osmo_cxvec *corr = NULL;
	float pos;
	int rv = 0;

	/* Generate reference dual chirp */
	ref = gmr1_sdr_fcch_gen_dual_chirp(1);
	if (!ref) {
		rv = -ENOMEM;
		goto err;
	}

	/* Normalize and decimate the search window */
	search_win = osmo_cxvec_sig_normalize(search_win_in, sps, freq_shift, NULL);

	/* Correlate with the reference */
	corr = osmo_cxvec_correlate(ref, search_win, 1, NULL);

	DEBUG_SIGNAL("fcch_rough", corr);

	/* Find the highest energy window */
	pos = osmo_cxvec_peak_energy_find(corr, 5, PEAK_WEIGH_WIN, NULL);

	/* Return TOA */
	*toa = (int)round(pos * sps);

	/* Cleanup */
err:
	osmo_cxvec_free(corr);
	osmo_cxvec_free(search_win);
	osmo_cxvec_free(ref);

	return rv;
}


/*! \brief Internal method to record peaks in order or power and remove dupes
 *  \param[in,out] toa Array of TOA already recorded
 *  \param[in,out] pwr Array of pwr already recorded
 *  \param[in,out] n Number of peaks already recorded
 *  \param[in] N size of the above arrays
 *  \param[in] Lp Periodicity length
 *  \param[in] sps Oversampling ratio
 *  \param[in] peak_toa New peak TOA
 *  \param[in] peak_pwr New peak power
 */
static inline void
_peak_record(int *toa, float *pwr, int *n, int N, int Lp, int sps,
             int peak_toa, float peak_pwr)
{
	int i,j;
	int has_dupe = 0;

	/* Make sure it's not a duplicate at Lp interval */
	/* (with a GMR1_FCCH_SYMS / 2 window) */
	for (i=0; i<*n; i++) {
		/* Compute params */
		int th = (GMR1_FCCH_SYMS * sps) >> 1;	/* threshold */
		int d = (toa[i] % Lp) - (peak_toa % Lp);/* wrapped toa diff */

		/* If not a dupe, continue */
		if (abs(d) > th)
			continue;

		/* If the new peak has less power, continue */
		if (pwr[i] > peak_pwr) {
			if (!has_dupe)
				has_dupe = 1;
			continue;
		}

		/* Remove the old peak */
		for (j=i; j<(*n)-1; j++) {
			toa[j] = toa[j+1];
			pwr[j] = pwr[j+1];
		}
		*n = *n - 1;

		/* We'll insert it no mather what */
		has_dupe = -1;
	}

	if (has_dupe > 0)
		return;

	/* Find insert point */
	for (i=0; i<*n; i++)
		if (peak_pwr > pwr[i])
			break;

	/* If it's at the end, nothing to do */
	if (i == N)
		return;

	/* Shift everything down */
	for (j=N-1; j>i; j--) {
		toa[j] = toa[j-1];
		pwr[j] = pwr[j-1];
	}

	/* Insert */
	toa[i] = peak_toa;
	pwr[i] = peak_pwr;

	/* One more entry ? */
	if (*n != N)
		*n = *n + 1;
}

/*! \brief Rough FCCH timing acquisition w/ multiple FCCH detection
 *  \param[in] search_win_in Complex signal where to search for FCCH
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] freq_shift Frequency shift to pre-apply to search_win_in (rad/sym)
 *  \param[out] peaks_toa Array of floats to store the returned alignements
 *  \param[in] N Maximum number of alignements to returns
 *  \returns A positive value of the number of FCCH returned. -errno for errors
 *
 * This method can detect multiple overlapping FCCH and returns alignements for
 * all of them. To do so it needs at least 650 ms worth of data (two SI cycles
 * plus some margin).
 */
int
gmr1_fcch_rough_multi(struct osmo_cxvec *search_win_in, int sps, float freq_shift,
                      int *peaks_toa, int N)
{
	struct osmo_cxvec *ref = NULL;
	struct osmo_cxvec *search_win = NULL;
	struct osmo_cxvec *corr = NULL;
	float *corr_pwr = NULL;
	float pwr_max, pwrs[2], peaks[2], avg, stddev, th, peaks_pwr[N];
	int Lw, Lp, nLp, i, pwr_max_idx, a, peaks_cnt;
	int rv;

	/* Safety : need 650 ms of signal */
	if (search_win_in->len < ((650 * GMR1_SYM_RATE * sps) / 1000))
		return -EINVAL;

	/* Generate reference dual chirp */
	ref = gmr1_sdr_fcch_gen_dual_chirp(1);
	if (!ref) {
		rv = -ENOMEM;
		goto err;
	}

	/* Normalize and decimate the search window */
	search_win = osmo_cxvec_sig_normalize(search_win_in, sps, freq_shift, NULL);

	/* Correlate with the reference */
	corr = osmo_cxvec_correlate(ref, search_win, 1, NULL);

	DEBUG_SIGNAL("fcch_rough_multi", corr);

	/* Convert to power + find peak within first 330 ms */
	corr_pwr = malloc(sizeof(float) * corr->len);
	if (!corr_pwr) {
		rv = -ENOMEM;
		goto err;
	}

	Lw = (320 * GMR1_SYM_RATE) / 1000;	/* Len window */
	Lw += GMR1_FCCH_SYMS;

	Lp = (320 * GMR1_SYM_RATE) / 1000;	/* Len period */

	pwr_max_idx = 0;
	pwr_max = 0.0f;

	for (i=0; i<corr->len; i++) {
		float e = osmo_normsqf(corr->data[i]);
		corr_pwr[i] = e;
		if ((e > pwr_max) && (i < Lw)) {
			pwr_max = e;
			pwr_max_idx = i;
		}
	}

	/* Whatever peak we found, there should be a matching one Lp
	 * samples later. Find it ! */
	pwrs[0]  = pwrs[1]  = 0.0f;
	peaks[0] = peaks[1] = 0.0f;

	for (i=-10; i<=10; i++)
	{
		int j = pwr_max_idx + i;

		if ((j > 0) && (j < corr->len)) {
			pwrs[0]  += corr_pwr[j];
			peaks[0] += corr_pwr[j] * j;
		}

		j += Lp;

		if ((j > 0) && (j < corr->len)) {
			pwrs[1]  += corr_pwr[j];
			peaks[1] += corr_pwr[j] * j;
		}
	}

	peaks[0] /= pwrs[0];
	peaks[1] /= pwrs[1];

	nLp = (int)round(peaks[1] - peaks[0]);

	/* Safety */
	if (abs(nLp - Lp) > 10) {
		rv = -EINVAL;
		goto err;
	}

	Lp = nLp;

	/* 'Mix' the two cycles to improve signal. Compute avg at the same time */
	avg = 0.0f;

	for (i=0; i<Lw; i++) {
		float v = sqrtf(corr_pwr[i] * corr_pwr[i+Lp]);
		corr_pwr[i] = v;
		avg += v;
	}

	avg /= Lw;

	/* Compute std dev */
	stddev = 0.0f;

	for (i=0; i<Lw; i++) {
		float v = corr_pwr[i] - avg;
		stddev += v * v;
	}

	stddev = sqrtf(stddev / Lw);

	/* Threshold is avg + (3 * stddev) */
	th = avg + 3.0f * stddev;

	/* Scan to find peaks */
	peaks_cnt = 0;

	for (i=1, a=0; i<Lw-1; i++) {
		if (corr_pwr[i] > th) {
			float p_pwr, p_fpos;
			int p_pos;

			/* Prevent doubles */
			if (a)
				continue;
			a = 1;

			/* Precise peak power and position */
			p_pwr = corr_pwr[i-1] + corr_pwr[i] + corr_pwr[i+1];
			p_fpos = (-corr_pwr[i-1] + corr_pwr[i+1]) / p_pwr;
			p_pos = (int)round((i + p_fpos) * sps);

			/* Record the peak */
			_peak_record(
				peaks_toa, peaks_pwr, &peaks_cnt, N, Lp, sps,
				p_pos, p_pwr
			);
		} else {
			/* Not in the peak */
			a = 0;
		}
	}

	rv = peaks_cnt;

	/* Cleanup */
err:
	free(corr_pwr);

	osmo_cxvec_free(corr);
	osmo_cxvec_free(search_win);
	osmo_cxvec_free(ref);

	return rv;
}


/*! \brief Fine FCCH timing & frequency acquisition
 *  \param[in] burst_in Complex signal of the FCCH burst
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] freq_shift Frequency shift to pre-apply to burst_in (rad/sym)
 *  \param[out] toa Pointer to the toa return variable
 *  \param[out] freq_error Pointer to the frequency error return variable (rad/sym)
 *  \returns 0 in case of success. -errno for errors.
 *
 * The input vector must be GMR1_FCCH_SYMS * sps samples long.
 * The frequency error is doesn't include any correction done with
 * freq_shift.
 */
int
gmr1_fcch_fine(struct osmo_cxvec *burst_in, int sps, float freq_shift,
               int *toa, float *freq_error)
{
	struct osmo_cxvec *ref_up = NULL, *ref_down = NULL;
	struct osmo_cxvec *mix_up = NULL, *mix_down = NULL;
	struct osmo_cxvec *burst = NULL;
	fftwf_plan fft_plan;
	float peak_up, peak_down;
	float freq_err_hz, freq_err_rps, toa_ms, toa_samples;
	int len, mid, i;
	int rv = 0;

	/* Generate reference up & down chirp */
	ref_up   = gmr1_sdr_fcch_gen_up_chirp(1);
	ref_down = gmr1_sdr_fcch_gen_down_chirp(1);

	if (!ref_up || !ref_down) {
		rv = -ENOMEM;
		goto err;
	}

	/* Normalize and decimate the burst to 1 sps */
	burst = osmo_cxvec_sig_normalize(burst_in, sps, freq_shift, NULL);
	if (!burst) {
		rv = -ENOMEM;
		goto err;
	}

	/* Sanity check */
	len = GMR1_FCCH_SYMS;

	if ((len != burst->len) ||
	    (len != ref_up->len) ||
	    (len != ref_down->len)) {
	    	rv = -EINVAL;
		goto err;
	}

	/* Multiply burst with the ref */
	mix_up   = osmo_cxvec_alloc(len);
	mix_down = osmo_cxvec_alloc(len);

	if (!mix_up || !mix_down) {
		rv = -ENOMEM;
		goto err;
	}

	for (i=0; i<len; i++) {
		mix_up->data[i]   = burst->data[i] * ref_up->data[i];
		mix_down->data[i] = burst->data[i] * ref_down->data[i];
	}

	mix_up->len = mix_down->len = len;

	/* Debug */
	DEBUG_SIGNAL("fcch_mix_up", mix_up);
	DEBUG_SIGNAL("fcch_mix_down", mix_down);

	/* Compute the FFT */
		/* Shift the frequency to center the FFT on x[58] */
	mid = (float)(len >> 1);
	for (i=0; i<len; i++) {
		float complex fs = cexp(I * 2.0f * M_PIf * mid / (float)(len) * i);
		mix_up->data[i] *= fs;
		mix_down->data[i] *= fs;
	}

		/* Do the fft */
	fft_plan = fftwf_plan_dft_1d(len, mix_up->data, mix_up->data, FFTW_FORWARD, FFTW_ESTIMATE);
	fftwf_execute(fft_plan);
	fftwf_destroy_plan(fft_plan);

	fft_plan = fftwf_plan_dft_1d(len, mix_down->data, mix_down->data, FFTW_FORWARD, FFTW_ESTIMATE);
	fftwf_execute(fft_plan);
	fftwf_destroy_plan(fft_plan);

	/* Debug */
	DEBUG_SIGNAL("fcch_fft_up", mix_up);
	DEBUG_SIGNAL("fcch_fft_down", mix_down);

	/* Find peaks position */
	peak_up   = osmo_cxvec_peak_energy_find(mix_up, 5, PEAK_WEIGH_WIN, NULL);
	peak_down = osmo_cxvec_peak_energy_find(mix_down, 5, PEAK_WEIGH_WIN, NULL);

	/* Convert to frequencies */
	peak_up   = (peak_up   - mid) * 200.0f;	/* 200 Hz bin size */
	peak_down = (peak_down - mid) * 200.0f;

	/* Frequency error */
	freq_err_hz = (peak_up + peak_down) / 2;
	freq_err_rps = (2 * M_PIf * freq_err_hz) / GMR1_SYM_RATE;

	*freq_error = freq_err_rps;

	/* Time of arrival */
		/* 3 kHz / ms chrip ramp rate */
	toa_ms = ((peak_up - peak_down) / 2) / 3000.0f;
	toa_samples = (toa_ms * GMR1_SYM_RATE * sps) / 1000.0f;

	*toa = (int)round(toa_samples);

	/* Cleanup */
err:
	osmo_cxvec_free(mix_down);
	osmo_cxvec_free(mix_up);

	osmo_cxvec_free(burst);

	osmo_cxvec_free(ref_down);
	osmo_cxvec_free(ref_up);

	return rv;
}


/*! \brief SNR estimation on a FCCH burst
 *  \param[in] burst_in Complex signal of the FCCH burst
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] freq_shift Frequency shift to pre-apply to burst_in (rad/sym)
 *  \param[out] snr Pointer to the SNR return variable
 *  \returns 0 in case of success. -errno for errors.
 *
 * The input vector must be GMR1_FCCH_SYMS * sps samples long.
 * This method estimated the FFT peak energy over the FFT average energy
 * to estimate SNR.
 */
int
gmr1_fcch_snr(struct osmo_cxvec *burst_in, int sps, float freq_shift, float *snr)
{
	struct osmo_cxvec *ref = NULL;
	struct osmo_cxvec *burst = NULL;
	fftwf_plan fft_plan;
	float avg;
	int len, i;
	int rv = 0;

	/* Generate reference dual chirp */
	ref = gmr1_sdr_fcch_gen_dual_chirp(1);
	if (!ref) {
		rv = -ENOMEM;
		goto err;
	}

	/* Normalize and decimate the burst to 1 sps */
	burst = osmo_cxvec_sig_normalize(burst_in, sps, freq_shift, NULL);
	if (!burst) {
		rv = -ENOMEM;
		goto err;
	}

	/* Sanity check */
	len = GMR1_FCCH_SYMS;

	if ((len != burst->len) ||
	    (len != ref->len)) {
	    	rv = -EINVAL;
		goto err;
	}

	/* Multiply burst with the ref (we know it's real only) */
	for (i=0; i<len; i++)
		burst->data[i] *= crealf(ref->data[i]);

	DEBUG_SIGNAL("fcch_snr_mix", burst);

	/* Compute the FFT */
	fft_plan = fftwf_plan_dft_1d(len, burst->data, burst->data, FFTW_FORWARD, FFTW_ESTIMATE);
	fftwf_execute(fft_plan);
	fftwf_destroy_plan(fft_plan);

	DEBUG_SIGNAL("fcch_snr_fft", burst);

	/* Evaluate SNR */
	avg = 0.0f;

	for (i=0; i<burst->len-3; i++)
		avg += osmo_normsqf(burst->data[2+i]);
	avg /= (burst->len-3);

	*snr = osmo_normsqf(burst->data[0]) / avg;

	/* Cleanup */
err:
	osmo_cxvec_free(burst);
	osmo_cxvec_free(ref);

	return rv;
}

/*! }@ */
