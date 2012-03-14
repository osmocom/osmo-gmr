/* GMR-1 RT framework common functions */

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

#include <complex.h>
#include <errno.h>

#include <osmocom/dsp/cxvec.h>
#include <osmocom/dsp/cxvec_math.h>

#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/pi4cxpsk.h>

#include "common.h"


int
win_map(struct osmo_cxvec *win,
        float complex *data, int data_len,
        int begin, int win_len)
{
	if ((begin + win_len) > data_len)
		return -1;

	osmo_cxvec_init_from_data(win, &data[begin], win_len);

	return 0;
}

int
burst_map(struct osmo_cxvec *burst,
          float complex *data, int data_len, int base_align, int sps,
          struct gmr1_pi4cxpsk_burst *burst_type, int tn, int win)
{
	int begin, len;
	int etoa;

	etoa  = win >> 1;
	begin = base_align + (sps * tn * 39) - etoa;
	len   = (burst_type->len * sps) + win;

	if ((begin < 0) || ((begin + len) > data_len))
		return -EIO;

	osmo_cxvec_init_from_data(burst, &data[begin], len);

	return etoa;
}

float
burst_energy(struct osmo_cxvec *burst)
{
	int i;
	int b = (burst->len >> 5); /* exclude the borders */
	float e = 0.0f;
	for (i=b; i<burst->len-b; i++)
		e += osmo_normsqf(burst->data[i]);
	e /= burst->len;
	return e;
}
