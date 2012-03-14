/* GMR-1 GSMtap helpers */

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

/*! \addtogroup gsmtap
 *  @{
 */

/*! \file gsmtap.c
 *  \brief Osmocom GMR-1 GSMtap helpers header
 */

#include <stdint.h>
#include <string.h>

#include <arpa/inet.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/gsmtap.h>
#include <osmocom/gmr1/gsmtap.h>


/*! \brief Helper to build GSM tap message with GMR-1 payload
 *  \param[in] chan_type Type of channel (one of GSMTAP_GMR1_xxx)
 *  \param[in] arfcn ARFCN
 *  \param[in] fn Frame number
 *  \param[in] tn Timeslot number
 *  \param[in] l2 Packet of L2 data to encapsulate
 *  \param[in] len Length of the l2 data in bytes
 */
struct msgb *
gmr1_gsmtap_makemsg(uint8_t chan_type, uint16_t arfcn, uint32_t fn, uint8_t tn,
                    const uint8_t *l2, int len)
{
	struct msgb *msg;
	struct gsmtap_hdr *gh;
	uint8_t *dst;

	msg = msgb_alloc(sizeof(*gh) + len, "gmr1_gsmtap_tx");
	if (!msg)
		return NULL;

	gh = (struct gsmtap_hdr *) msgb_put(msg, sizeof(*gh));
	gh->version = GSMTAP_VERSION;
	gh->hdr_len = sizeof(*gh)/4;
	gh->type = GSMTAP_TYPE_GMR1_UM;
	gh->arfcn = htons(arfcn);
	gh->timeslot = tn;
	gh->sub_slot = 0;
	gh->snr_db = 0;
	gh->signal_dbm = 0;
	gh->frame_number = htonl(fn);
	gh->sub_type = chan_type;
	gh->antenna_nr = 0;

	dst = msgb_put(msg, len);
	memcpy(dst, l2, len);

	return msg;
}

/*! @} */
