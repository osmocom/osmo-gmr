/* GMR-1 convolutional coding */
/* See GMR-1 05.003 (ETSI TS 101 376-5-3 V1.2.1) - Section 4.4 */

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

/*! \addtogroup conv
 *  @{
 */

/*! \file l1/conv.c
 *  \brief Osmocom GMR-1 convolutional coding implementation
 */

#include <stdint.h>

#include <osmocom/core/conv.h>


/* All codes are K=5 and share the same next_state table */
static const uint8_t gmr1_conv_next_state[][2] = {
	{  0,  1 }, {  2,  3 }, {  4,  5 }, {  6,  7 },
	{  8,  9 }, { 10, 11 }, { 12, 13 }, { 14, 15 },
	{  0,  1 }, {  2,  3 }, {  4,  5 }, {  6,  7 },
	{  8,  9 }, { 10, 11 }, { 12, 13 }, { 14, 15 },
};


/*
 * GMR-1 Rate 1/2 convolutional code
 *
 * g0(D) = 1 +           D^3 + D^4
 * g1(D) = 1 + D + D^2 +       D^4
 */

static const uint8_t gmr1_conv_12_next_output[][2] = {
	{ 0, 3 }, { 1, 2 }, { 1, 2 }, { 0, 3 },
	{ 2, 1 }, { 3, 0 }, { 3, 0 }, { 2, 1 },
	{ 3, 0 }, { 2, 1 }, { 2, 1 }, { 3, 0 },
	{ 1, 2 }, { 0, 3 }, { 0, 3 }, { 1, 2 },
};

/*! \brief GMR-1 rate 1/2 convolutional code */
const struct osmo_conv_code gmr1_conv_12 = {
	.N = 2,
	.K = 5,
	.len = 0, /* to be filled during specialization */
	.next_output = gmr1_conv_12_next_output,
	.next_state = gmr1_conv_next_state,
};


/*
 * GMR-1 Rate 1/3 convolutional code
 *
 * g0(D) = 1 +     D^2 +       D^4
 * g1(D) = 1 + D +       D^3 + D^4
 * g2(D) = 1 + D + D^2 + D^3 + D^4
 */

static const uint8_t gmr1_conv_13_next_output[][2] = {
	{ 0, 7 }, { 3, 4 }, { 5, 2 }, { 6, 1 },
	{ 3, 4 }, { 0, 7 }, { 6, 1 }, { 5, 2 },
	{ 7, 0 }, { 4, 3 }, { 2, 5 }, { 1, 6 },
	{ 4, 3 }, { 7, 0 }, { 1, 6 }, { 2, 5 },
};

/*! \brief GMR-1 rate 1/3 convolutional code */
const struct osmo_conv_code gmr1_conv_13 = {
	.N = 3,
	.K = 5,
	.len = 0, /* to be filled during specialization */
	.next_output = gmr1_conv_13_next_output,
	.next_state = gmr1_conv_next_state,
};


/*
 * GMR-1 Rate 1/4 convolutional code
 *
 * g0(D) = 1 +           D^3 + D^4
 * g1(D) = 1 + D + D^2 +       D^4
 * g2(D) = 1 +     D^2 +       D^4
 * g3(D) = 1 + D + D^2 + D^3 + D^4
 */

static const uint8_t gmr1_conv_14_next_output[][2] = {
	{  0, 15 }, {  5, 10 }, {  7,  8 }, {  2, 13 },
	{  9,  6 }, { 12,  3 }, { 14,  1 }, { 11,  4 },
	{ 15,  0 }, { 10,  5 }, {  8,  7 }, { 13,  2 },
	{  6,  9 }, {  3, 12 }, {  1, 14 }, {  4, 11 },
};

/*! \brief GMR-1 rate 1/4 convolutional code */
const struct osmo_conv_code gmr1_conv_14 = {
	.N = 4,
	.K = 5,
	.len = 0, /* to be filled during specialization */
	.next_output = gmr1_conv_14_next_output,
	.next_state = gmr1_conv_next_state,
};


/*
 * GMR-1 Rate 1/5 convolutional code
 *
 * g0(D) = 1 +     D^2 +       D^4
 * g1(D) = 1 + D +       D^3 + D^4
 * g2(D) = 1 + D + D^2 + D^3 + D^4
 * g3(D) = 1 +     D^2 + D^3 + D^4
 * g4(D) = 1 + D + D^2 +       D^4
 */

static const uint8_t gmr1_conv_15_next_output[][2] = {
	{  0, 31 }, { 13, 18 }, { 23,  8 }, { 26,  5 },
	{ 14, 17 }, {  3, 28 }, { 25,  6 }, { 20, 11 },
	{ 31,  0 }, { 18, 13 }, {  8, 23 }, {  5, 26 },
	{ 17, 14 }, { 28,  3 }, {  6, 25 }, { 11, 20 },
};

/*! \brief GMR-1 rate 1/5 convolutional code */
const struct osmo_conv_code gmr1_conv_15 = {
	.N = 5,
	.K = 5,
	.len = 0, /* to be filled during specialization */
	.next_output = gmr1_conv_15_next_output,
	.next_state = gmr1_conv_next_state,
};

/*! }@ */
