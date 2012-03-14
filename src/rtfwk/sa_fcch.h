/* GMR-1 RT framework: FCCH task */

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

#ifndef __RTFWK_SA_FCCH_H__
#define __RTFWK_SA_FCCH_H__

struct app_state;
struct gmr1_fcch_burst;

struct fcch_sink_params {
	struct app_state *as;
	int chan_id;

	int start_discard;
	const struct gmr1_fcch_burst *burst_type;
};

extern const struct sample_actor_desc fcch_sink;

#endif /* __RTFWK_SA_FCCH_H__ */
