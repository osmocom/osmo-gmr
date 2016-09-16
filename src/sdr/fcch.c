/* GMR-1 SDR - FCCH bursts */
/* See GMR-1 05.004 (ETSI TS 101 376-5-4 V3.1.1) - Section 8 */

/* (C) 2011-2019 by Sylvain Munaut <tnt@246tNt.com>
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

#include <osmocom/dsp/cxvec.h>
#include <osmocom/dsp/cxvec_math.h>

#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/fcch.h>


/* ------------------------------------------------------------------------ */
/* Burst format data                                                        */
/* ------------------------------------------------------------------------ */

/*! \brief FCCH burst (GMR-1 version)
 *  See GMR-1 05.004 (ETSI TS 101 376-5-2 V3.1.1) - Section 8.1
 */
const struct gmr1_fcch_burst gmr1_fcch_burst = {
	.freq = 0.32f,
	.len  = 3 * 39,
};


/*! \brief FCCH3 L-band burst (GMR-1 3G version for L-band)
 *  See GMR-1 05.004 (ETSI TS 101 376-5-2 V3.1.1) - Section 8.2.1
 */
const struct gmr1_fcch_burst gmr1_fcch3_lband_burst = {
	.freq = 0.32f,
	.len  = 12 * 39,
};

/*! \brief FCCH3 S-band burst (GMR-1 3G version for S-band)
 *  See GMR-1 05.004 (ETSI TS 101 376-5-2 V3.1.1) - Section 8.2.2
 */
const struct gmr1_fcch_burst gmr1_fcch3_sband_burst = {
	.freq = 0.16f,
	.len  = 12 * 39,
};


/* ------------------------------------------------------------------------ */
/* Reference waveform generation                                            */
/* ------------------------------------------------------------------------ */

/*! \brief Generate FCCH reference up or down chirp at a given oversampling
 *  \param[in] burst_type FCCH burst format description
 *  \param[in] sps Oversampling rate
 *  \param[in] up_down Selects chirp direction (0=down 1=up)
 *  \returns A newly allocated complex vector containing the chirp
 *
 * Up-Chirp: \f$\frac{\sqrt{2}}{2}\cdot e^{j\left(
 * 2\pi f\cdot\left(t-\frac{T}{2}\right)^2/T^2\right)}\f$
 *
 * Down-Chirp: \f$\frac{\sqrt{2}}{2}\cdot e^{-j\left(
 * 2\pi f\cdot\left(t-\frac{T}{2}\right)^2/T^2\right)}\f$
 *
 * The resulting length will either be 117 * sps or 468 * sps depending on the
 * type of FCCH burst
 */
static struct osmo_cxvec *
gmr1_sdr_fcch_gen_up_down_chirp(const struct gmr1_fcch_burst *burst_type,
                                int sps, int up_down)
{
	struct osmo_cxvec *cv;
	int i, l;
	float sq2d2, phase_base, pos, halfpos;

	l = burst_type->len * sps;

	cv = osmo_cxvec_alloc(l);
	if (!cv)
		return NULL;

	cv->len = l;

	sq2d2 = sqrtf(2.0f) / 2.0f;
	phase_base = burst_type->freq * 2.0f * M_PIf / (float)(burst_type->len);
	halfpos = ((float)(burst_type->len)) / 2.0f;

	if (up_down)
		phase_base *= -1.0f;

	for (i=0; i<l; i++) {
		pos = ((float)i / (float)sps) - halfpos;
		cv->data[i] = sq2d2 * cexpf( I * phase_base * (pos * pos) );
	}

	return cv;
}

/*! \brief Generate FCCH reference up chirp at a given oversampling
 *  \param[in] burst_type FCCH burst format description
 *  \param[in] sps Oversampling rate
 *  \returns A newly allocated complex vector containing the chirp
 *
 * \f$\frac{\sqrt{2}}{2}\cdot e^{j\left(
 * 2\pi f\cdot\left(t-\frac{T}{2}\right)^2/T^2\right)}\f$
 *
 * The resulting length will either be 117 * sps or 468 * sps depending on the
 * type of FCCH burst
 */
static struct osmo_cxvec *
gmr1_sdr_fcch_gen_up_chirp(const struct gmr1_fcch_burst *burst_type, int sps)
{
	return gmr1_sdr_fcch_gen_up_down_chirp(burst_type, sps, 0);
}

/*! \brief Generate FCCH reference down chirp at a given oversampling
 *  \param[in] burst_type FCCH burst format description
 *  \param[in] sps Oversampling rate
 *  \returns A newly allocated complex vector containing the chirp
 *
 * \f$\frac{\sqrt{2}}{2}\cdot e^{-j\left(
 * 2\pi f\cdot\left(t-\frac{T}{2}\right)^2/T^2\right)}\f$
 *
 * The resulting length will either be 117 * sps or 468 * sps depending on the
 * type of FCCH burst
 */
static struct osmo_cxvec *
gmr1_sdr_fcch_gen_down_chirp(const struct gmr1_fcch_burst *burst_type, int sps)
{
	return gmr1_sdr_fcch_gen_up_down_chirp(burst_type, sps, 1);
}

/*! \brief Generate FCCH reference dual chirp at a given oversampling
 *  \param[in] burst_type FCCH burst format description
 *  \param[in] sps Oversampling rate
 *  \returns A newly allocated complex vector containing the dual chirp
 *
 * \f$\sqrt{2}\cdot\cos\left(2\pi f\cdot\left(t-\frac{T}{2}\right)^2/\;T^2\right)\f$
 *
 * The resulting length will either be 117 * sps or 468 * sps depending on the
 * type of FCCH burst. The vector is also 'real only'.
 */
static struct osmo_cxvec *
gmr1_sdr_fcch_gen_dual_chirp(const struct gmr1_fcch_burst *burst_type, int sps)
{
	struct osmo_cxvec *cv;
	int i, l;
	float sq2, phase_base, pos, halfpos;

	l = burst_type->len * sps;

	cv = osmo_cxvec_alloc(l);
	if (!cv)
		return NULL;

	cv->len = l;
	cv->flags |= CXVEC_FLG_REAL_ONLY;

	sq2 = sqrtf(2.0f);
	phase_base = burst_type->freq * 2.0f * M_PIf / (float)(burst_type->len);
	halfpos = ((float)(burst_type->len)) / 2.0f;

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
 *  \param[in] burst_type FCCH burst format description
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
gmr1_fcch_rough(const struct gmr1_fcch_burst *burst_type,
                struct osmo_cxvec *search_win_in, int sps, float freq_shift,
                int *toa)
{
	struct osmo_cxvec *ref = NULL;
	struct osmo_cxvec *search_win = NULL;
	struct osmo_cxvec *corr = NULL;
	float pos;
	int rv = 0;

	/* Generate reference dual chirp */
	ref = gmr1_sdr_fcch_gen_dual_chirp(burst_type, 1);
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
 *  \param[in] burst_type FCCH burst format description
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
_peak_record(const struct gmr1_fcch_burst *burst_type,
             int *toa, float *pwr, int *n, int N, int Lp, int sps,
             int peak_toa, float peak_pwr)
{
	int i,j;
	int has_dupe = 0;

	/* Make sure it's not a duplicate at Lp interval */
	/* (with a window of half burst length) */
	for (i=0; i<*n; i++) {
		/* Compute params */
		int th = (burst_type->len * sps) >> 1;	/* threshold */
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
 *  \param[in] burst_type FCCH burst format description
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
gmr1_fcch_rough_multi(const struct gmr1_fcch_burst *burst_type,
                      struct osmo_cxvec *search_win_in, int sps, float freq_shift,
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
	ref = gmr1_sdr_fcch_gen_dual_chirp(burst_type, 1);
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
	Lw += burst_type->len;

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
			_peak_record(burst_type,
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
 *  \param[in] burst_type FCCH burst format description
 *  \param[in] burst_in Complex signal of the FCCH burst
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] freq_shift Frequency shift to pre-apply to burst_in (rad/sym)
 *  \param[out] toa Pointer to the toa return variable
 *  \param[out] freq_error Pointer to the frequency error return variable (rad/sym)
 *  \returns 0 in case of success. -errno for errors.
 *
 * The input vector must be burst_type->len * sps samples long.
 * The frequency error is doesn't include any correction done with
 * freq_shift.
 */
int
gmr1_fcch_fine(const struct gmr1_fcch_burst *burst_type,
               struct osmo_cxvec *burst_in, int sps, float freq_shift,
               int *toa, float *freq_error)
{
	struct osmo_cxvec *ref_up = NULL, *ref_down = NULL;
	struct osmo_cxvec *mix_up = NULL, *mix_down = NULL;
	struct osmo_cxvec *burst = NULL;
	fftwf_plan fft_plan;
	float bin_hz, peak_up, peak_down;
	float freq_err_hz, freq_err_rps;
	float chirp_rate, toa_ms, toa_samples;
	int len, mid, i;
	int rv = 0;

	/* Generate reference up & down chirp */
	ref_up   = gmr1_sdr_fcch_gen_up_chirp(burst_type, 1);
	ref_down = gmr1_sdr_fcch_gen_down_chirp(burst_type, 1);

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
	len = burst_type->len;

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
		/* Shift the frequency to center the FFT on x[len >> 1] */
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
	peak_up   = osmo_cxvec_peak_energy_find(mix_up,   5, PEAK_WEIGH_WIN, NULL);
	peak_down = osmo_cxvec_peak_energy_find(mix_down, 5, PEAK_WEIGH_WIN, NULL);

	/* Convert to frequencies */
	bin_hz = (float)GMR1_SYM_RATE / (float)len;
	peak_up   = (peak_up   - mid) * bin_hz;
	peak_down = (peak_down - mid) * bin_hz;

	/* Frequency error */
	freq_err_hz = (peak_up + peak_down) / 2.0f;
	freq_err_rps = (2.0f * M_PIf * freq_err_hz) / GMR1_SYM_RATE;

	*freq_error = freq_err_rps;

	/* Time of arrival */
	chirp_rate = (2.0f * burst_type->freq * GMR1_SYM_RATE * GMR1_SYM_RATE) / (float)(burst_type->len * 1000);
	toa_ms = ((peak_up - peak_down) / 2.0f) / chirp_rate;
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
 *  \param[in] burst_type FCCH burst format description
 *  \param[in] burst_in Complex signal of the FCCH burst
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] freq_shift Frequency shift to pre-apply to burst_in (rad/sym)
 *  \param[out] snr Pointer to the SNR return variable
 *  \returns 0 in case of success. -errno for errors.
 *
 * The input vector must be burst_type->len * sps samples long.
 * This method estimated the FFT peak energy over the FFT average energy
 * to estimate SNR.
 */
int
gmr1_fcch_snr(const struct gmr1_fcch_burst *burst_type,
              struct osmo_cxvec *burst_in, int sps, float freq_shift,
              float *snr)
{
	struct osmo_cxvec *ref = NULL;
	struct osmo_cxvec *burst = NULL;
	fftwf_plan fft_plan;
	int peaks[6], len, i;
	int rv = 0;

	/* Generate reference dual chirp */
	ref = gmr1_sdr_fcch_gen_dual_chirp(burst_type, 1);
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
	len = burst_type->len;

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
		/* Find 6 strongest peaks */
		/* With time/freq error, there will be 2 peaks after the mix,
		 * and they might not be bin-aligned and so the 4 first peaks
		 * are "signal". We then use the two next as an upper limit
		 * for "noise" */
	osmo_cxvec_peaks_scan(burst, peaks, 6);

		/* Among the top 6, top 2 over bottom 2 */
	*snr =	(osmo_normsqf(burst->data[peaks[0]]) + osmo_normsqf(burst->data[peaks[1]])) /
		(osmo_normsqf(burst->data[peaks[4]]) + osmo_normsqf(burst->data[peaks[5]]));

	/* Cleanup */
err:
	osmo_cxvec_free(burst);
	osmo_cxvec_free(ref);

	return rv;
}

/*! @} */
