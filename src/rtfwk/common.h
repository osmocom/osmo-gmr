/* GMR-1 RT framework: Common definitions */

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

#ifndef __RTFWK_COMMON_H__
#define __RTFWK_COMMON_H__

#include <math.h>

#include <osmocom/dsp/cxvec.h>
#include <osmocom/dsp/cxvec_math.h>

#include <osmocom/gmr1/sdr/defs.h>

#include "sampbuf.h"


struct gsmtap_inst;
struct gmr1_pi4cxpsk_burst;


struct app_state
{
	/* Sample source */
	struct sample_buf *buf;

	/* Params */
	int n_chans;
	int sps;

	/* GSMTap */
	struct gsmtap_inst *gti;

	/* Per-Channel data */
	struct {
		int arfcn;
		char *filename;
	} chans[0];
};


int
win_map(struct osmo_cxvec *win,
        float complex *data, int data_len,
        int begin, int win_len);

int
burst_map(struct osmo_cxvec *burst,
          float complex *data, int data_len, int base_align, int sps,
          struct gmr1_pi4cxpsk_burst *burst_type, int tn, int win);

float
burst_energy(struct osmo_cxvec *burst);


static inline float
to_hz(float f_rps)
{
	return (GMR1_SYM_RATE * f_rps) / (2.0f * M_PIf);
}

static inline float
to_db(float v)
{
	return 10.0f * log10f(v);
}


#endif /* __RTFWK_COMMON_H__ */
