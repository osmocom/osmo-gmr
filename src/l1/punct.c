/* GMR-1 puncturing */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 4.5 */

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

/*! \addtogroup punct
 *  @{
 */

/*! \file l1/punct.c
 *  \brief Osmocom GMR-1 puncturing implementation
 */

#include <osmocom/core/conv.h>

#include <osmocom/gmr1/l1/punct.h>

#include <errno.h>
#include <stdlib.h>


/*! \brief Generate convolutional code puncturing array for a osmo_conv_code
 *  \param[inout] code The code for which to generate the puncturing array
 *  \param[in] punct_pre The puncturing scheme for first block (can be NULL)
 *  \param[in] punct_main The puncturing scheme
 *  \param[in] punct_post The puncturing scheme for last block (can be NULL)
 *  \param[in] repeat How many time to apply main punctured (0 = auto)
 *  \return 0 for success, <0 for error codes.
 *
 * The array is allocated with malloc and must be free'd by the caller
 * when no longer required.
 */
int
gmr1_puncturer_generate(struct osmo_conv_code *code,
                        const struct gmr1_puncturer *punct_pre,
                        const struct gmr1_puncturer *punct_main,
                        const struct gmr1_puncturer *punct_post,
			int repeat)
{
	int N, d, cl, pl;
	int ip, ii, io, i;
	int *p;

	/* Safety checks */
	N = code->N;

	if (punct_pre && (punct_pre->N != N))
		return -EINVAL;

	if (punct_main->N != N)
		return -EINVAL;

	if (punct_post && (punct_post->N != N))
		return -EINVAL;

	/* Upper bound for length */
	cl = osmo_conv_get_output_length(code, 0);
	pl = 0;

	if (punct_pre) {
		cl -= punct_pre->L * N;
		pl += punct_pre->r;
	}

	if (punct_post) {
		cl -= punct_post->L * N;
		pl += punct_post->r;
	}

	d = punct_main->L * N;

	if (!repeat)
		repeat = ((cl + d - 1) / d);

	pl += repeat * punct_main->r + 1;

	/* Alloc array */
	p = malloc(pl * sizeof(int));
	if (!p)
		return -ENOMEM;

	/* Fill */
	cl = osmo_conv_get_output_length(code, 0);
	ii = io = 0;

	if (punct_pre) {
		d = punct_pre->L * N;
		for (ip=0; ii<cl && ip<d ; ii++,ip++)
			if (punct_pre->mask[ip] == 0)
				p[io++] = ii;
	}

	if (punct_post) {
		cl -= punct_post->L * N;
	}

	for (i=0; i<repeat; i++) {
		d = punct_main->L * N;
		for (ip=0; ii<cl && ip<d; ii++,ip++) {
			if (punct_main->mask[ip] == 0)
				p[io++] = ii;
		}
	}

	if (punct_post) {
		d = punct_post->L * N;
		ii = cl;
		for (ip=0; ii>0 && ip<d; ii++,ip++)
			if (punct_post->mask[ip] == 0)
				p[io++] = ii;
	}

	p[io] = -1;

	code->puncture = p;

	return 0;
}


/*! \brief GMR-1 P(1;2) puncturing code for the rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct12_P12 = {
	.r = 1,
	.L = 2,
	.N = 2,
	.mask = {
		1, 1,
		1, 0,
	},
};

/*! \brief GMR-1 P(2;3) puncturing code for the rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct12_P23 = {
	.r = 2,
	.L = 3,
	.N = 2,
	.mask = {
		0, 1,
		1, 0,
		1, 1,
	},
};

/*! \brief GMR-1 P(2;5) puncturing code for the rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct12_P25 = {
	.r = 2,
	.L = 5,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P*(2;5) puncturing code for the rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct12_Ps25 = {
	.r = 2,
	.L = 5,
	.N = 2,
	.mask = {
		1, 1,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
	},
};

/*! \brief GMR-1 P(3;11) puncturing code for the rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct12_P311 = {
	.r = 3,
	.L = 11,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 1,
		1, 0,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(4;12) puncturing code for the rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct12_P412 = {
	.r = 4,
	.L = 12,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P*(4;12) puncturing code for the rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct12_Ps412 = {
	.r = 4,
	.L = 12,
	.N = 2,
	.mask = {
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
	},
};

/*! \brief GMR-1 P(2;5) puncturing code for the rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct13_P25 = {
	.r = 2,
	.L = 5,
	.N = 3,
	.mask = {
		1, 1, 1,
		1, 1, 1,
		1, 0, 1,
		1, 1, 1,
		1, 0, 1,
	},
};

/*! \brief GMR-1 P(1;5) puncturing code for the rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct13_P15 = {
	.r = 1,
	.L = 5,
	.N = 3,
	.mask = {
		1, 0, 1,
		1, 1, 1,
		1, 1, 1,
		1, 1, 1,
		1, 1, 1,
	},
};

/*! \brief GMR-1 P*(1;5) puncturing code for the rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct13_Ps15 = {
	.r = 1,
	.L = 5,
	.N = 3,
	.mask = {
		1, 1, 1,
		1, 1, 1,
		1, 1, 1,
		1, 1, 1,
		1, 0, 1,
	},
};

/*! \brief GMR-1 P(1;6) puncturing code for the rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct13_P16 = {
	.r = 1,
	.L = 6,
	.N = 3,
	.mask = {
		1, 1, 0,
		1, 1, 1,
		1, 1, 1,
		1, 1, 1,
		1, 1, 1,
		1, 1, 1,
	},
};

/*! \brief GMR-1 P(2;3) puncturing code for the rate 1/5 conv coder */
const struct gmr1_puncturer gmr1_punct15_P23 = {
	.r = 2,
	.L = 3,
	.N = 5,
	.mask = {
		1, 1, 1, 1, 1,
		1, 1, 0, 1, 1,
		1, 1, 1, 1, 0,
	},
};

/*! \brief GMR-1 P(5;3) puncturing code for the rate 1/5 conv coder */
const struct gmr1_puncturer gmr1_punct15_P53 = {
	.r = 5,
	.L = 3,
	.N = 5,
	.mask = {
		1, 1, 1, 0, 1,
		1, 0, 0, 1, 1,
		1, 1, 1, 0, 0,
	},
};

/*! \brief GMR-1 P*(5;3) puncturing code for the rate 1/5 conv coder */
const struct gmr1_puncturer gmr1_punct15_Ps53 = {
	.r = 5,
	.L = 3,
	.N = 5,
	.mask = {
		1, 1, 1, 0, 0,
		1, 0, 0, 1, 1,
		1, 1, 1, 0, 1,
	},
};

/*! @} */
