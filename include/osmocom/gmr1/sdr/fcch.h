/* GMR-1 SDR - FCCH burst */
/* See GMR-1 05.004 (ETSI TS 101 376-5-4 V1.2.1) - Section 8.1 */

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

#ifndef __OSMO_GMR1_SDR_FCCH_H__
#define __OSMO_GMR1_SDR_FCCH_H__

/*! \defgroup fcch FCCH bursts
 *  \ingroup sdr
 *  @{
 */

/*! \file sdr/fcch.h
 *  \brief Osmocom GMR-1 FCCH bursts header
 */

#include <osmocom/sdr/cxvec.h>


#define GMR1_FCCH_SYMS	(39*3)	/*!< \brief FCCH burst duration in symbols */


int gmr1_fcch_rough(struct osmo_cxvec *search_win_in, int sps, float freq_shift,
                    int *toa);

int gmr1_fcch_rough_multi(struct osmo_cxvec *search_win_in, int sps, float freq_shift,
                          int *toa, int N);

int gmr1_fcch_fine(struct osmo_cxvec *burst_in, int sps, float freq_shift,
                   int *toa, float *freq_error);

int gmr1_fcch_snr(struct osmo_cxvec *burst_in, int sps, float freq_shift,
                  float *snr);


/*! }@ */

#endif /* __OSMO_GMR1_SDR_FCCH_H__ */
