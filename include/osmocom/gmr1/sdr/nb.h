/* GMR-1 SDR - Normal bursts */
/* See GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) */

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

#ifndef __OSMO_GMR1_SDR_NB_H__
#define __OSMO_GMR1_SDR_NB_H__

/*! \defgroup nb Normal bursts
 *  \ingroup sdr
 *  @{
 */

/*! \file sdr/nb.h
 *  \brief Osmocom GMR-1 Normal bursts header
 */

struct gmr1_pi4cxpsk_burst;


	/* Various normal bursts types */
extern struct gmr1_pi4cxpsk_burst gmr1_bcch_burst;
extern struct gmr1_pi4cxpsk_burst gmr1_dc2_burst;
extern struct gmr1_pi4cxpsk_burst gmr1_dc6_burst;
extern struct gmr1_pi4cxpsk_burst gmr1_nt3_speech_burst;
extern struct gmr1_pi4cxpsk_burst gmr1_nt3_facch_burst;
extern struct gmr1_pi4cxpsk_burst gmr1_nt6_burst;
extern struct gmr1_pi4cxpsk_burst gmr1_nt9_burst;
extern struct gmr1_pi4cxpsk_burst gmr1_rach_burst;
extern struct gmr1_pi4cxpsk_burst gmr1_sdcch_burst;


/*! @} */

#endif /* __OSMO_GMR1_SDR_NB_H__ */
