/* GMR-1 A5 Ciphering algorithm */

/*
 * Full reimplementation of GMR-1 A5/1
 *
 * The logic behind the algorithm has been reverse engineered from a
 * Thuraya phone DSP. Thanks to Benedikt Driessen, Ralf Hund,
 * Carsten Willems, Christof Paar, and Thorsten Holz for their work
 * on this. See their paper for more details on how it was done.
 */

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

/*! \addtogroup a5
 *  @{
 */

/*! \file l1/a5.c
 *  \brief Osmocom GMR-1 A5 ciphering algorithm implementation
 */

#include <string.h>
#include <stdint.h>

#include <osmocom/core/bits.h>

#include <osmocom/gmr1/l1/a5.h>


/*! \brief Main method to generate a A5/x cipher stream
 *  \param[in] n Which A5/x method to use
 *  \param[in] key 8 byte array for the key (as received from the SIM)
 *  \param[in] fn Frame number
 *  \param[in] nbits How many bits to generate
 *  \param[out] dl Pointer to array of ubits to return Downlink cipher stream
 *  \param[out] ul Pointer to array of ubits to return Uplink cipher stream
 *
 * Currently only A5/0 and A5/1.
 * Either (or both) of dl/ul can be NULL if not needed.
 */
void
gmr1_a5(int n, uint8_t *key, uint32_t fn, int nbits,
        ubit_t *dl, ubit_t *ul)
{
	switch (n)
	{
	case 0:
		if (dl)
			memset(dl, 0x00, nbits);
		if (ul)
			memset(ul, 0x00, nbits);
		break;

	case 1:
		gmr1_a5_1(key, fn, nbits, dl, ul);
		break;

	default:
		/* a5/[2...7] not supported/existent */
		break;
	};
}

/*! \brief Computes parity of a 32-bit word
 *  \param[in] x 32 bit word
 *  \return Parity bit (xor of all bits) as 0 or 1
 */
static inline uint32_t
_a5_parity(uint32_t x)
{
	x ^= x >> 16;
	x ^= x >> 8;
	x ^= x >> 4;
	x &= 0xf;
	return (0x6996 >> x) & 1;
}

/*! \brief Compute majority bit from 3 taps
 *  \param[in] v1 LFSR state ANDed with tap-bit
 *  \param[in] v2 LFSR state ANDed with tap-bit
 *  \param[in] v3 LFSR state ANDed with tap-bit
 *  \return The majority bit (0 or 1)
 */
static inline uint32_t
_a5_majority(uint32_t v1, uint32_t v2, uint32_t v3)
{
	return (!!v1 + !!v2 + !!v3) >= 2;
}

/*! \brief Compute the next LFSR state
 *  \param[in] r Current state
 *  \param[in] mask LFSR mask
 *  \param[in] taps LFSR taps
 *  \return Next state
 */
static inline uint32_t
_a5_clock(uint32_t r, uint32_t mask, uint32_t taps)
{
	return ((r << 1) & mask) | _a5_parity(r & taps);
}



#define A51_R1_LEN	19
#define A51_R2_LEN	22
#define A51_R3_LEN	23
#define A51_R4_LEN	17

#define A51_R1_MASK	((1<<A51_R1_LEN)-1)
#define A51_R2_MASK	((1<<A51_R2_LEN)-1)
#define A51_R3_MASK	((1<<A51_R3_LEN)-1)
#define A51_R4_MASK	((1<<A51_R4_LEN)-1)

#define A51_R1_TAPS	0x072000	/* x^19 + x^18 + x^17 + x^14 + 1 */
#define A51_R2_TAPS	0x311000	/* x^22 + x^21 + x^17 + x^13 + 1 */
#define A51_R3_TAPS	0x660000	/* x^23 + x^22 + x^19 + x^18 + 1 */
#define A51_R4_TAPS	0x013100	/* x^17 + x^14 + x^13 + x^9 + 1 */

#define A51_BIT(r,n)	(1 << n)


/*! \brief GMR1-A5/1: Set the high bits of all register states
 *  \param[in] r Register states
 */
static inline void
_a5_1_set_bits(uint32_t *r)
{
	r[0] |= 1;
	r[1] |= 1;
	r[2] |= 1;
	r[3] |= 1;
}

/*! \brief GMR1-A5/1: Force clocking of all registers
 *  \param[in] r Register states
 */
static inline void
_a5_1_clock_force(uint32_t *r)
{
	r[0] = _a5_clock(r[0], A51_R1_MASK, A51_R1_TAPS);
	r[1] = _a5_clock(r[1], A51_R2_MASK, A51_R2_TAPS);
	r[2] = _a5_clock(r[2], A51_R3_MASK, A51_R3_TAPS);
	r[3] = _a5_clock(r[3], A51_R4_MASK, A51_R4_TAPS);
}

/*! \brief GMR1-A5/1: Clock all registers according to clocking rule
 *  \param[in] r Register states
 */
static inline void
_a5_1_clock(uint32_t *r)
{
	int cb[3], m;

	cb[0] = !!(r[3] & A51_BIT(R4, 15));
	cb[1] = !!(r[3] & A51_BIT(R4,  6));
	cb[2] = !!(r[3] & A51_BIT(R4,  1));

	m = (cb[0] + cb[1] + cb[2]) >= 2;

	if (cb[0] == m)
		r[0] = _a5_clock(r[0], A51_R1_MASK, A51_R1_TAPS);

	if (cb[1] == m)
		r[1] = _a5_clock(r[1], A51_R2_MASK, A51_R2_TAPS);

	if (cb[2] == m)
		r[2] = _a5_clock(r[2], A51_R3_MASK, A51_R3_TAPS);

	r[3] = _a5_clock(r[3], A51_R4_MASK, A51_R4_TAPS);
}

/*! \brief GMR1-A5/1: Generate the output bit from register states
 *  \param[in] r Register states
 *  \return The output bit
 */
static inline ubit_t
_a5_1_output(uint32_t *r)
{
	int m[3];

	#define MAJ(rnum, rname, a, b, c)		\
		m[rnum] = _a5_majority(			\
			r[rnum] & A51_BIT(rname, a),	\
			r[rnum] & A51_BIT(rname, b),	\
			r[rnum] & A51_BIT(rname, c)	\
		);

	MAJ(0, R1, 1,  6, 15);
	MAJ(1, R2, 3,  8, 14);
	MAJ(2, R3, 4, 15, 19);

	#undef MAJ

	m[0] ^= !!(r[0] & A51_BIT(R1, 11));
	m[1] ^= !!(r[1] & A51_BIT(R2,  1));
	m[2] ^= !!(r[2] & A51_BIT(R3,  0));

	return m[0] ^ m[1] ^ m[2];
}

/*! \brief Generate a GMR-1 A5/1 cipher stream
 *  \param[in] key 8 byte array for the key (as received from the SIM)
 *  \param[in] fn Frame number
 *  \param[in] nbits How many bits to generate
 *  \param[out] dl Pointer to array of ubits to return Downlink cipher stream
 *  \param[out] ul Pointer to array of ubits to return Uplink cipher stream
 *
 * Either (or both) of dl/ul can be NULL if not needed.
 */
void
gmr1_a5_1(uint8_t *key, uint32_t fn, int nbits, ubit_t *dl, ubit_t *ul)
{
	uint32_t r[4];
	uint8_t lkey[8];
	int i;

	/* Reorganize the key */
	for (i=0; i<8; i++)
		lkey[i] = key[i ^ 1];

	/* Mix-in frame number */
	lkey[6] ^= (fn & 0x0000f) <<  4; /* MFFN */
	lkey[3] ^= (fn & 0x00030) <<  2; /* MultiFrame Number */
	lkey[1] ^= (fn & 0x007c0) >>  3; /* SuperFrame Number */
	lkey[0] ^= (fn & 0x0f800) >> 11; /* ... */
	lkey[0] ^= (fn & 0x70000) >> 11; /* ... */

	/* Init Rx */
	r[0] = r[1] = r[2] = r[3] = 0;

	/* Key mixing */
	for (i=0; i<64; i++) {
		int byte_idx = (i >> 3);
		int bit_idx  = 7 - (i & 7);
		uint32_t b = (lkey[byte_idx] >> bit_idx) & 1;

		_a5_1_clock_force(r);

		r[0] ^= b;
		r[1] ^= b;
		r[2] ^= b;
		r[3] ^= b;
	}

	/* Set high bits */
	_a5_1_set_bits(r);

	/* Mixing */
	for (i=0; i<250; i++)
		_a5_1_clock(r);

	/* DL Output */
	for (i=0; i<nbits; i++) {
		_a5_1_clock(r);
		if (dl)
			dl[i] = _a5_1_output(r);
	}

	/* UL Output */
	if (!ul)
		return;

	for (i=0; i<nbits; i++) {
		_a5_1_clock(r);
		ul[i] = _a5_1_output(r);
	}
}

/*! @} */
