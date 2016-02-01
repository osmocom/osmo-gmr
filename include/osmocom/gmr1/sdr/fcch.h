/* GMR-1 SDR - FCCH bursts */
/* See GMR-1 05.004 (ETSI TS 101 376-5-4 V3.1.1) - Section 8 */

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

#ifndef __OSMO_GMR1_SDR_FCCH_H__
#define __OSMO_GMR1_SDR_FCCH_H__

/*! \defgroup fcch FCCH bursts
 *  \ingroup sdr
 *  @{
 */

/*! \file sdr/fcch.h
 *  \brief Osmocom GMR-1 FCCH bursts header
 */

#include <osmocom/dsp/cxvec.h>


struct gmr1_fcch_burst
{
	float freq;	/*!< \brief Frequency sweep range */
	int len;	/*!< \brief Burst duration in symbols */
};

extern const struct gmr1_fcch_burst gmr1_fcch_burst;
extern const struct gmr1_fcch_burst gmr1_fcch3_lband_burst;
extern const struct gmr1_fcch_burst gmr1_fcch3_sband_burst;


int gmr1_fcch_rough(const struct gmr1_fcch_burst *burst_type,
                    struct osmo_cxvec *search_win_in, int sps, float freq_shift,
                    int *toa);

int gmr1_fcch_rough_multi(const struct gmr1_fcch_burst *burst_type,
                          struct osmo_cxvec *search_win_in, int sps, float freq_shift,
                          int *toa, int N);

int gmr1_fcch_fine(const struct gmr1_fcch_burst *burst_type,
                   struct osmo_cxvec *burst_in, int sps, float freq_shift,
                   int *toa, float *freq_error);

int gmr1_fcch_snr(const struct gmr1_fcch_burst *burst_type,
                  struct osmo_cxvec *burst_in, int sps, float freq_shift,
                  float *snr);


/*! @} */

#endif /* __OSMO_GMR1_SDR_FCCH_H__ */
