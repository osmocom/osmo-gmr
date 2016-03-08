/* GMR-1 RACH generation utility */

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

#include <stdio.h>
#include <stdlib.h>

#include <osmocom/core/bits.h>
#include <osmocom/core/utils.h>

#include <osmocom/dsp/cxvec.h>

#include <osmocom/gmr1/l1/rach.h>
#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/pi4cxpsk.h>
#include <osmocom/gmr1/sdr/nb.h>

int main(int argc, char *argv[])
{
	const char *filename;
	uint8_t sb_mask, rach[18];
	ubit_t ebits[494];
	struct osmo_cxvec *burst;
	int rv;

	/* Check and parse args */
	if (argc != 4) {
		fprintf(stderr, "Usage: %s out.cfile sb_mask payload\n", argv[0]);
		return -1;
	}

	filename = argv[1];
	sb_mask = strtoul(argv[2], NULL, 0);

	rv = osmo_hexparse(argv[3], rach, 18);
	if (rv != 18) {
		fprintf(stderr, "Invalid payload string\n");
		return -1;
	}

	/* Alloc, encode, modulate, save, release */
	burst = osmo_cxvec_alloc(gmr1_rach_burst.len);
	gmr1_rach_encode(ebits, rach, sb_mask);
	gmr1_pi4cxpsk_mod(&gmr1_rach_burst, ebits, 0, burst);
	osmo_cxvec_dbg_dump(burst, filename);
	osmo_cxvec_free(burst);

	/* Done */
	return 0;
}
