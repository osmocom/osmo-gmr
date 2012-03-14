/* GMR-1 RT framework: TCH9 task */

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

#ifndef __RTFWK_SA_TCH9_H__
#define __RTFWK_SA_TCH9_H__

#include <stdint.h>

struct app_state;

struct tch9_sink_params {
	struct app_state *as;
	int chan_id;
	uint32_t fn;
	uint64_t align;
	float freq_err;
	int tn;
	int ciph;
	uint8_t kc[8];
};

extern const struct sample_actor_desc tch9_sink;

#endif /* __RTFWK_SA_TCH9_H__ */
