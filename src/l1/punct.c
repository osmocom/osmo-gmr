/* GMR-1 puncturing */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V3.3.1) - Section 4.5 */

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


/*! \brief GMR-1 P(2;3) puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_P23 = {
	.r = 2,
	.L = 3,
	.N = 2,
	.mask = {
		0, 1,
		1, 0,
		1, 1,
	},
};

/*! \brief GMR-1 P(2;5) puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_P25 = {
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

/*! \brief GMR-1 Ps(2;5) puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_Ps25 = {
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

/*! \brief GMR-1 P(3;11) puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_P311 = {
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

/*! \brief GMR-1 P(4;12) puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_P412 = {
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

/*! \brief GMR-1 Ps(4;12) puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_Ps412 = {
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

/*! \brief GMR-1 P(1;2) puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_P12 = {
	.r = 1,
	.L = 2,
	.N = 2,
	.mask = {
		1, 1,
		1, 0,
	},
};

/*! \brief GMR-1 Ps(1;2) puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_Ps12 = {
	.r = 1,
	.L = 2,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
	},
};

/*! \brief GMR-1 A puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_A = {
	.r = 0,
	.L = 4,
	.N = 2,
	.mask = {
		1, 1,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 B puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_B = {
	.r = 1,
	.L = 4,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 C puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_C = {
	.r = 2,
	.L = 4,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 0,
		1, 1,
	},
};

/*! \brief GMR-1 D puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_D = {
	.r = 3,
	.L = 4,
	.N = 2,
	.mask = {
		0, 1,
		1, 0,
		0, 1,
		1, 1,
	},
};

/*! \brief GMR-1 E puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_E = {
	.r = 1,
	.L = 4,
	.N = 2,
	.mask = {
		1, 2,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(3;8) puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_P38 = {
	.r = 3,
	.L = 8,
	.N = 2,
	.mask = {
		0, 1,
		1, 1,
		0, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 0,
		1, 1,
	},
};

/*! \brief GMR-1 P(2;6) puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_P26 = {
	.r = 2,
	.L = 6,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 1,
		1, 0,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(3;7) puncturing code for the K=5 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_12_P37 = {
	.r = 3,
	.L = 7,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(1;6) puncturing code for the K=5 rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_13_P16 = {
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

/*! \brief GMR-1 P(2;5) puncturing code for the K=5 rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_13_P25 = {
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

/*! \brief GMR-1 P(1;5) puncturing code for the K=5 rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_13_P15 = {
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

/*! \brief GMR-1 Ps(1;5) puncturing code for the K=5 rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_13_Ps15 = {
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

/*! \brief GMR-1 P(7;8) puncturing code for the K=5 rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_13_P78 = {
	.r = 7,
	.L = 8,
	.N = 3,
	.mask = {
		0, 0, 1,
		1, 1, 0,
		1, 1, 1,
		0, 1, 1,
		1, 1, 1,
		1, 1, 0,
		1, 0, 1,
		1, 0, 1,
	},
};

/*! \brief GMR-1 P(2;3) puncturing code for the K=5 rate 1/5 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_15_P23 = {
	.r = 2,
	.L = 3,
	.N = 5,
	.mask = {
		1, 1, 1, 1, 1,
		1, 1, 0, 1, 1,
		1, 1, 1, 1, 0,
	},
};

/*! \brief GMR-1 P(5;3) puncturing code for the K=5 rate 1/5 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_15_P53 = {
	.r = 5,
	.L = 3,
	.N = 5,
	.mask = {
		1, 1, 1, 0, 1,
		1, 0, 0, 1, 1,
		1, 1, 1, 0, 0,
	},
};

/*! \brief GMR-1 Ps(5;3) puncturing code for the K=5 rate 1/5 conv coder */
const struct gmr1_puncturer gmr1_punct_k5_15_Ps53 = {
	.r = 5,
	.L = 3,
	.N = 5,
	.mask = {
		1, 1, 1, 0, 0,
		1, 0, 0, 1, 1,
		1, 1, 1, 0, 1,
	},
};

/*! \brief GMR-1 P(2;3) puncturing code for the K=7 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k7_12_P23 = {
	.r = 2,
	.L = 3,
	.N = 2,
	.mask = {
		1, 1,
		1, 0,
		0, 1,
	},
};

/*! \brief GMR-1 P(4;10) puncturing code for the K=7 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k7_12_P410 = {
	.r = 4,
	.L = 10,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 1,
		1, 1,
		1, 0,
		1, 1,
	},
};

/*! \brief GMR-1 P(5;12) puncturing code for the K=7 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k7_12_P512 = {
	.r = 5,
	.L = 12,
	.N = 2,
	.mask = {
		1, 1,
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 1,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
	},
};

/*! \brief GMR-1 P(1;16) puncturing code for the K=7 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k7_12_P116 = {
	.r = 1,
	.L = 16,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(1;48) puncturing code for the K=7 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k7_12_P148 = {
	.r = 1,
	.L = 48,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(1;84) puncturing code for the K=7 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k7_12_P184 = {
	.r = 1,
	.L = 84,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(1;152) puncturing code for the K=7 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k7_12_P1152 = {
	.r = 1,
	.L = 152,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(4;5) puncturing code for the K=7 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k7_12_P45 = {
	.r = 4,
	.L = 5,
	.N = 2,
	.mask = {
		0, 1,
		1, 1,
		1, 0,
		0, 1,
		1, 0,
	},
};

/*! \brief GMR-1 P2(4;5) puncturing code for the K=7 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k7_12_P245 = {
	.r = 4,
	.L = 5,
	.N = 2,
	.mask = {
		1, 0,
		0, 1,
		1, 0,
		0, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(1;3) puncturing code for the K=9 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_12_P13 = {
	.r = 1,
	.L = 3,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(4;7) puncturing code for the K=9 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_12_P47 = {
	.r = 4,
	.L = 7,
	.N = 2,
	.mask = {
		0, 1,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
		1, 1,
		1, 0,
	},
};

/*! \brief GMR-1 P(3;4) puncturing code for the K=9 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_12_P34 = {
	.r = 3,
	.L = 4,
	.N = 2,
	.mask = {
		1, 1,
		1, 0,
		0, 1,
		1, 0,
	},
};

/*! \brief GMR-1 P(1;7) puncturing code for the K=9 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_12_P17 = {
	.r = 1,
	.L = 7,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(1;9) puncturing code for the K=9 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_12_P19 = {
	.r = 1,
	.L = 9,
	.N = 2,
	.mask = {
		0, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(2;6) puncturing code for the K=9 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_12_P26 = {
	.r = 2,
	.L = 6,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 1,
		1, 0,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(1;10) puncturing code for the K=9 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_12_P110 = {
	.r = 1,
	.L = 10,
	.N = 2,
	.mask = {
		0, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(1;4) puncturing code for the K=9 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_12_P14 = {
	.r = 1,
	.L = 4,
	.N = 2,
	.mask = {
		1, 0,
		1, 1,
		1, 1,
		1, 1,
	},
};

/*! \brief GMR-1 P(4;5) puncturing code for the K=9 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_12_P45 = {
	.r = 4,
	.L = 5,
	.N = 2,
	.mask = {
		0, 1,
		1, 1,
		0, 1,
		1, 0,
		1, 0,
	},
};

/*! \brief GMR-1 P2(3;4) puncturing code for the K=9 rate 1/2 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_12_P234 = {
	.r = 3,
	.L = 4,
	.N = 2,
	.mask = {
		1, 0,
		0, 1,
		1, 0,
		1, 1,
	},
};

/*! \brief GMR-1 P(4;5) puncturing code for the K=6 rate 1/4 conv coder */
const struct gmr1_puncturer gmr1_punct_k6_14_P45 = {
	.r = 4,
	.L = 5,
	.N = 4,
	.mask = {
		1, 0, 1, 1,
		1, 0, 1, 1,
		1, 1, 1, 0,
		1, 1, 1, 1,
		1, 1, 1, 0,
	},
};

/*! \brief GMR-1 P(14;8) puncturing code for the K=9 rate 1/4 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_14_P148 = {
	.r = 14,
	.L = 8,
	.N = 4,
	.mask = {
		1, 0, 0, 1,
		1, 0, 1, 0,
		1, 0, 1, 0,
		1, 0, 0, 1,
		1, 1, 0, 1,
		1, 0, 0, 1,
		1, 0, 0, 1,
		1, 1, 0, 1,
	},
};

/*! \brief GMR-1 P(6;5) puncturing code for the K=9 rate 1/4 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_14_P65 = {
	.r = 6,
	.L = 5,
	.N = 4,
	.mask = {
		0, 1, 1, 1,
		1, 0, 1, 1,
		1, 1, 1, 1,
		0, 0, 1, 0,
		1, 0, 1, 1,
	},
};

/*! \brief GMR-1 P(1;2) puncturing code for the K=9 rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_13_P12 = {
	.r = 1,
	.L = 2,
	.N = 3,
	.mask = {
		1, 1, 1,
		0, 1, 1,
	},
};

/*! \brief GMR-1 P(12;13) puncturing code for the K=9 rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_13_P1213 = {
	.r = 12,
	.L = 13,
	.N = 3,
	.mask = {
		1, 1, 0,
		1, 0, 1,
		0, 1, 1,
		1, 1, 0,
		1, 0, 1,
		0, 1, 1,
		1, 1, 0,
		1, 0, 1,
		0, 1, 1,
		1, 1, 0,
		1, 0, 1,
		0, 1, 1,
		1, 1, 1,
	},
};

/*! \brief GMR-1 P(4;4) puncturing code for the K=9 rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_13_P44 = {
	.r = 4,
	.L = 4,
	.N = 3,
	.mask = {
		1, 1, 0,
		0, 1, 1,
		1, 0, 1,
		1, 1, 0,
	},
};

/*! \brief GMR-1 P(3;3) puncturing code for the K=9 rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_13_P33 = {
	.r = 3,
	.L = 3,
	.N = 3,
	.mask = {
		0, 1, 1,
		1, 0, 1,
		1, 1, 0,
	},
};

/*! \brief GMR-1 P(6;5) puncturing code for the K=9 rate 1/3 conv coder */
const struct gmr1_puncturer gmr1_punct_k9_13_P65 = {
	.r = 6,
	.L = 5,
	.N = 3,
	.mask = {
		1, 0, 1,
		0, 1, 1,
		1, 0, 0,
		0, 1, 1,
		1, 1, 0,
	},
};


/*! @} */
