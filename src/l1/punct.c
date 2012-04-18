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
 *  \param[in] punct The puncturing scheme description
 *  \return 0 for success, <0 for error codes.
 *
 * The array is allocated with malloc and must be free'd by the caller
 * when no longer required.
 */
int
gmr1_puncturer_generate(struct osmo_conv_code *code,
                        const struct gmr1_puncturer *punct)
{
	int i, j, cl, pl;
	int *p;

	/* Safety checks */
	if (code->N != punct->N)
		return -EINVAL;

	/* Upper bound for length */
	cl = osmo_conv_get_output_length(code, 0);
	pl = ((cl + punct->L - 1) / punct->L) * punct->r + 1;

	/* Alloc array */
	p = malloc(pl * sizeof(int));
	if (!p)
		return -ENOMEM;

	code->puncture = p;

	/* Fill */
	for (i=0, j=0; i<cl; i++) {
		if (punct->mask[i % (punct->N * punct->L)] == 0)
			p[j++] = i;
	}
	p[j] = -1;

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
