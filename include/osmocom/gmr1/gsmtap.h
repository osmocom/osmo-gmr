/* GMR-1 GSMtap helpers */

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

#ifndef __OSMO_GMR1_GSMTAP_H__
#define __OSMO_GMR1_GSMTAP_H__

/*! \defgroup gsmtap GMR-1 GSMtap helpers
 *  @{
 */

/*! \file gsmtap.h
 *  \brief Osmocom GMR-1 GSMtap helpers header
 */

#include <stdint.h>

struct msgb;


#define GSMTAP_TYPE_GMR1_UM		0x0a    /* GMR-1 L2 packets */
#define GSMTAP_GMR1_BCCH		0x01

struct msgb *gmr1_gsmtap_makemsg(
	uint8_t chan_type, uint32_t fn, uint8_t tn,
	const uint8_t *l2, int len);


/*! @} */

#endif /* __OSMO_GMR1_GSMTAP_H__ */
