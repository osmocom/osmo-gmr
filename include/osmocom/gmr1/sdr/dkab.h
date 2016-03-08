/* GMR-1 SDR - DKABs bursts */
/* See GMR-1 05.004 (ETSI TS 101 376-5-4 V1.2.1) - Section 6.1
 *     GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) - Section 7.4.6 */

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

#ifndef __OSMO_GMR1_SDR_DKAB_H__
#define __OSMO_GMR1_SDR_DKAB_H__

/*! \defgroup dkab DKAB bursts
 *  \ingroup sdr
 *  @{
 */

/*! \file sdr/dkab.h
 *  \brief Osmocom GMR-1 DKABs bursts header
 */

#include <stdint.h>
#include <osmocom/core/bits.h>
#include <osmocom/dsp/cxvec.h>

#define GMR1_DKAB_SYMS (39*3)

int
gmr1_dkab_demod(struct osmo_cxvec *burst_in, int sps, float freq_shift, int p,
                sbit_t *ebits, float *toa_p);


/*! @} */

#endif /* __OSMO_GMR1_SDR_DKAB_H__ */
