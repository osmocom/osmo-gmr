/* GMR-1 interleaving */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 4.8 */

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

/*! \addtogroup interleave
 *  @{
 */

/*! \file l1/interleave.c
 *  \brief Osmocom GMR-1 interleaving implementation
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <osmocom/core/bits.h>

#include <osmocom/gmr1/l1/interleave.h>


/*! \brief GMR-1 intra burst inteleaver
 *  \param[out] out Interleaved bit array to write to
 *  \param[in] in Original bit array to read from
 *  \param[in] N Dimension of the interleaving matrix
 *
 * Both arrays need to have a length of (8*N).
 * This routine works for any type that has the same size as uint8_t like
 * sbit_t or ubit_t.
 */
void
gmr1_interleave_intra(void *out, const void *in, int N)
{
	const uint8_t *inb = (uint8_t *)in;
	uint8_t *outb = (uint8_t *)out;
	int len = N << 3;
	int kc, kep, i, j;

	for (kc=0; kc<len; kc++) {
		i = kc >> 3;
		j = (5 * kc) & 7;
		kep = N * j + i;
		outb[kep] = inb[kc];
	}
}

/*! \brief GMR-1 intra burst de-interleaver
 *  \param[out] out Deinterleaved bit array to write to
 *  \param[in] in Interleaved bit array to read from
 *  \param[in] N Dimension of the interleaving matrix
 *
 * Both arrays need to have a length of (8*N).
 * This routine works for any type that has the same size as uint8_t like
 * sbit_t or ubit_t.
 */
void
gmr1_deinterleave_intra(void *out, const void *in, int N)
{
	const uint8_t *inb = (uint8_t *)in;
	uint8_t *outb = (uint8_t *)out;
	int len = N << 3;
	int kc, kep, i, j;

	for (kc=0; kc<len; kc++) {
		i = kc >> 3;
		j = (5 * kc) & 7;
		kep = N * j + i;
		outb[kc] = inb[kep];
	}
}


/*! \brief GMR-1 inter burst interleaver initializer
 *  \param[in] il The interleaver object to init
 *  \param[in] N The interleaving depth
 *  \param[in] K The interleaving width
 */
int
gmr1_interleaver_init(struct gmr1_interleaver *il, int N, int K)
{
	int l;
	uint8_t *b;

	memset(il, 0x00, sizeof(struct gmr1_interleaver));

	l = N * K * sizeof(uint8_t);
	b = malloc(l);
	if (!b)
		return -ENOMEM;

	memset(b, 0x00, l);

	il->N = N;
	il->K = K;
	il->bits_cpp = b;

	return 0;
}

/*! \brief GMR-1 inter burst interleaver cleanup
 *  \param[in] il The interleaver object to release
 */
void
gmr1_interleaver_fini(struct gmr1_interleaver *il)
{
	free(il->bits_cpp);

	memset(il, 0x00, sizeof(struct gmr1_interleaver));
}

/*! \brief GMR-1 inter burst interleaver
 *  \param[in] il The interleaver object
 *  \param[out] bits_epp N bits output of interleaver
 *  \param[in] bits_ep N bits input to interleaver
 *
 *  bits_ep and bits_epp can be equal for inplace processing
 */
void
gmr1_interleave_inter(struct gmr1_interleaver *il,
                      void *bits_epp, void *bits_ep)
{
	int i, jk;
	uint8_t *d, *s;

	/* Copy ep to cpp */
	i = il->n % il->N;
	d = &il->bits_cpp[i * il->K];
	memcpy(d, bits_ep, il->K * sizeof(uint8_t));

	/* Copy cpp to epp */
	d = bits_epp;
	s = il->bits_cpp;

	for (jk=0; jk<il->K; jk++) {
		i = ((il->n % il->N) - (jk % il->N) + il->N) % il->N;
		d[jk] = s[(i * il->K) + jk];
	}

	/* Next burst */
	il->n++;
}

/*! \brief GMR-1 inter burst de-interleaver
 *  \param[in] il The interleaver object
 *  \param[out] bits_ep N bits output from de-interleaver
 *  \param[in] bits_epp N bits input to de-interleaver
 *
 *  bits_ep and bits_epp can be equal for inplace processing
 */
void
gmr1_deinterleave_inter(struct gmr1_interleaver *il,
                        void *bits_ep, void *bits_epp)
{
	int i, jk;
	uint8_t *d, *s;

	/* Copy epp to cpp */
	s = bits_epp;
	d = il->bits_cpp;

	for (jk=0; jk<il->K; jk++) {
		i = ((il->n % il->N) - (jk % il->N) + il->N) % il->N;
		d[(i * il->K) + jk] = s[jk];
	}

	/* Copy cpp to ep */
	i = (il->n + 1) % il->N;
	s = &il->bits_cpp[i * il->K];
	memcpy(bits_ep, s, il->K * sizeof(uint8_t));

	/* Next burst */
	il->n++;
}

/*! @} */
