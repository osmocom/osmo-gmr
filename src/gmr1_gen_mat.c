/* GMR-1 G/g matrix geneation for FACCH3 */

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

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <osmocom/core/bits.h>
#include <osmocom/core/utils.h>

#include <osmocom/gmr1/l1/facch3.h>

static void
copy_bits(ubit_t *dst, int dst_col, int n_col, ubit_t *bits_e)
{
	int i, j;

	for (i=0; i<4; i++) {
		for (j=0; j<96; j++) {
			int si = (104 * i) + ((j < 22) ? j : (j+8));
			int di = dst_col + (((i*96)+j) * n_col);
			dst[di] = bits_e[si];
		}
	}
}

static int
pbm_save_bits(const char *filename, ubit_t *m, int x, int y)
{
	FILE *fh;
	int i, j;

	fh = fopen(filename, "w");
	if (!fh)
		return -EPERM;

	fprintf(fh, "P1\n%d %d\n", x, y);

	for (i=0; i<y; i++)
		for (j=0; j<x; j++)
			fprintf(fh, "%d%c", m[(i*x)+j], j==x-1 ? '\n' : ' ');

	fclose(fh);

	return 0;
}

int main(int argc, char *argv[])
{
	ubit_t mat_G[384*76];	/* 384 lines of 76 pixels */
	ubit_t mat_g[384];	/* 384 lines of 1 pixel */

	ubit_t bits_e[104*4];
	ubit_t bits_u[76];
	ubit_t bits_s[8*4];
	uint8_t l2[10];

	int i, j;

	memset(mat_G, 0x00, sizeof(mat_G));
	memset(mat_g, 0x00, sizeof(mat_g));

	memset(bits_s, 0x00, sizeof(bits_s));
	memset(l2, 0x00, sizeof(l2));

	gmr1_facch3_encode(bits_e, l2, bits_s, NULL);
	copy_bits(mat_g, 0, 1, bits_e);

	for (i=0; i<76; i++) {
		memset(bits_u, 0, sizeof(bits_u));
		bits_u[i] = 1;
		osmo_ubit2pbit_ext(l2, 0, bits_u, 0, 76, 1);
		gmr1_facch3_encode(bits_e, l2, bits_s, NULL);
		copy_bits(mat_G, i, 76, bits_e);
	}

	for (i=0; i<76; i++) {
		for (j=0; j<384; j++) {
			mat_G[(j*76)+i] ^= mat_g[j];
		}
	}

	pbm_save_bits("mat_G.pbm", mat_G, 76, 384);
	pbm_save_bits("mat_g.pbm", mat_g,  1, 384);

	return 0;
}
