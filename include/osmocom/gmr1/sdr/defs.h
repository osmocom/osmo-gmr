/* GMR-1 SDR - Global definitions */
/* See GMR-1 05.004 (ETSI TS 101 376-5-4 V1.2.1) */

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

#ifndef __OSMO_GMR1_SDR_DEFS_H__
#define __OSMO_GMR1_SDR_DEFS_H__

/*! \defgroup sdr GMR-1 Software Defined Radio library
 *  @{
 */

/*! \file sdr/defs.h
 *  \brief Osmocom GMR-1 SDR global definitions
 */


#define GMR1_SYM_RATE	23400	/*!< \brief Base GMR-1 symbol rate */

#if 0
#define DEBUG_SIGNAL(n,v) osmo_cxvec_dbg_dump(v, "/tmp/dbg_" n ".cfile");
#else
#define DEBUG_SIGNAL(n,v) do { } while (0)
#endif


/*! @} */

#endif /* __OSMO_GMR1_SDR_DEFS_H__ */
