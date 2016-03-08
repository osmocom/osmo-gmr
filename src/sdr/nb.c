/* GMR-1 SDR - Normal bursts */
/* See GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) */

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

/*! \addtogroup nb
 *  @{
 */

/*! \file sdr/nb.c
 *  \brief Osmocom GMR-1 Normal bursts descriptions
 */

#include <stdlib.h>

#include <osmocom/gmr1/sdr/pi4cxpsk.h>


/* BCCH ------------------------------------------------------------------- */

static struct gmr1_pi4cxpsk_sync _bcch_sync[] = {
	{  28, 11, { 0, 2, 2, 0, 0, 0, 2, 0, 2, 2, 2 } },
	{ 119,  3, { 2, 2, 0 } },
	{ 197,  3, { 2, 2, 0 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_data _bcch_data[] = {
	{   2, 26 },	/* e0   ... e51  */
	{  39, 80 },	/* e52  ... e211 */
	{ 122, 75 },	/* e212 ... e361 */
	{ 200, 31 },	/* e361 ... e423 */
	{ -1 },
};

/*! \brief BCCH bursts
 *  See GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) - Section 7.4.2
 */
struct gmr1_pi4cxpsk_burst gmr1_bcch_burst = {
	.mod = &gmr1_pi4cqpsk,
	.guard_pre = 2,
	.guard_post = 3,
	.len = 39 * 6,
	.ebits = 424,
	.sync = { _bcch_sync, NULL },
	.data = _bcch_data,
};


/* DC2 -------------------------------------------------------------------- */

static struct gmr1_pi4cxpsk_sync _dc2_sync[] = {
	{ 28, 7, { 0, 1, 2, 3, 0, 3, 0 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_data _dc2_data[] = {
	{  2, 26 },	/* e0  ... e51  */
	{ 35, 40 },	/* e52 ... e131 */
	{ -1 },
};

/*! \brief DC2 bursts
 *  See GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) - Section 7.4.4
 */
struct gmr1_pi4cxpsk_burst gmr1_dc2_burst = {
	.mod = &gmr1_pi4cqpsk,
	.guard_pre = 2,
	.guard_post = 3,
	.len = 39 * 2,
	.ebits = 132,
	.sync = { _dc2_sync, NULL },
	.data = _dc2_data,
};


/* DC6 -------------------------------------------------------------------- */

static struct gmr1_pi4cxpsk_sync _dc6_sync[] = {
	{  28, 7, { 0, 0, 0, 2, 2, 0, 2 } },
	{ 119, 3, { 0, 3, 0 } },
	{ 197, 3, { 3, 1, 1 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_data _dc6_data[] = {
	{   2, 26 },	/* e0   ... e51  */
	{  35, 84 },	/* e52  ... e219 */
	{ 122, 75 },	/* e220 ... e369 */
	{ 200, 31 },	/* e370 ... e431 */
	{ -1 },
};

/*! \brief DC6 bursts
 *  See GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) - Section 7.4.5
 */
struct gmr1_pi4cxpsk_burst gmr1_dc6_burst = {
	.mod = &gmr1_pi4cqpsk,
	.guard_pre = 2,
	.guard_post = 3,
	.len = 39 * 6,
	.ebits = 432,
	.sync = { _dc6_sync, NULL },
	.data = _dc6_data,
};


/* NT3 Speech ------------------------------------------------------------- */

static struct gmr1_pi4cxpsk_sync _nt3_speech_sync[] = {
	{ 28, 6, { 0, 3, 3, 1, 2, 3 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_data _nt3_speech_data[] = {
	{  2, 26 },	/* e0  ... e51  */
	{ 34, 80 },	/* e52 ... e211 */
	{ -1 },
};

/*! \brief NT3 bursts for encoded speech
 *  See GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) - Section 7.4.8.1
 */
struct gmr1_pi4cxpsk_burst gmr1_nt3_speech_burst = {
	.mod = &gmr1_pi4cqpsk,
	.guard_pre = 2,
	.guard_post = 3,
	.len = 39 * 3,
	.ebits = 212,
	.sync = { _nt3_speech_sync, NULL },
	.data = _nt3_speech_data,
};


/* NT3 FACCH -------------------------------------------------------------- */

static struct gmr1_pi4cxpsk_sync _nt3_facch_sync0[] = {
	{ 28, 8, { 1, 0, 1, 0, 1, 0, 1, 0 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_sync _nt3_facch_sync1[] = {
	{ 28, 8, { 1, 1, 0, 0, 1, 0, 0, 1 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_data _nt3_facch_data[] = {
	{  2, 26 },	/* e0  ... e25  */
	{ 36, 78 },	/* e26 ... e103 */
	{ -1 },
};

/*! \brief NT3 bursts for FACCH
 *  See GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) - Section 7.4.8.2
 */
struct gmr1_pi4cxpsk_burst gmr1_nt3_facch_burst = {
	.mod = &gmr1_pi4cbpsk,
	.guard_pre = 2,
	.guard_post = 3,
	.len = 39 * 3,
	.ebits = 104,
	.sync = { _nt3_facch_sync0, _nt3_facch_sync1, NULL },
	.data = _nt3_facch_data,
};


/* NT6 -------------------------------------------------------------------- */

static struct gmr1_pi4cxpsk_sync _nt6_facch_sync[] = {
	{  28, 6, { 0, 2, 2, 3, 2, 3 } },
	{ 119, 3, { 0, 1, 0 } },
	{ 197, 3, { 2, 3, 0 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_sync _nt6_data_sync[] = {
	{  28, 6, { 0, 0, 0, 2, 2, 0 } },
	{ 119, 3, { 1, 3, 0 } },
	{ 197, 3, { 2, 1, 3 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_data _nt6_data[] = {
	{   2, 26 },	/* e0   ... e51  */
	{  34, 85 },	/* e52  ... e221 */
	{ 122, 75 },	/* e222 ... e371 */
	{ 200, 31 },	/* e372 ... e433 */
	{ -1 },
};

/*! \brief NT6 bursts
 *  See GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) - Section 7.4.9
 */
struct gmr1_pi4cxpsk_burst gmr1_nt6_burst = {
	.mod = &gmr1_pi4cqpsk,
	.guard_pre = 2,
	.guard_post = 3,
	.len = 39 * 6,
	.ebits = 434,
	.sync = { _nt6_facch_sync, _nt6_data_sync, NULL },
	.data = _nt6_data,
};


/* NT9 -------------------------------------------------------------------- */

static struct gmr1_pi4cxpsk_sync _nt9_facch_sync[] = {
	{  28, 6, { 0, 2, 2, 3, 2, 3 } },
	{ 119, 3, { 1, 2, 2 } },
	{ 197, 3, { 0, 1, 0 } },
	{ 275, 3, { 2, 3, 0 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_sync _nt9_data_sync[] = {
	{  28, 6, { 0, 0, 0, 2, 2, 0 } },
	{ 119, 3, { 0, 2, 0 } },
	{ 197, 3, { 1, 3, 0 } },
	{ 275, 3, { 2, 1, 3 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_data _nt9_data[] = {
	{   2, 26 },	/* e0   ... e51  */
	{  34, 85 },	/* e52  ... e221 */
	{ 122, 75 },	/* e222 ... e371 */
	{ 200, 75 },	/* e372 ... e521 */
	{ 278, 70 },	/* e522 ... e661 */
	{ -1 },
};

/*! \brief NT9 bursts
 *  See GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) - Section 7.4.10
 */
struct gmr1_pi4cxpsk_burst gmr1_nt9_burst = {
	.mod = &gmr1_pi4cqpsk,
	.guard_pre = 2,
	.guard_post = 3,
	.len = 39 * 9,
	.ebits = 662,
	.sync = { _nt9_facch_sync, _nt9_data_sync, NULL },
	.data = _nt9_data,
};


/* RACH =------------------------------------------------------------------ */

static struct gmr1_pi4cxpsk_sync _rach_sync[] = {
	{  78, 17, { 0, 2, 2, 0, 0, 0, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0 } },
	{ 127, 32, { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	             2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 } },
	{ 191, 32, { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	             2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 } },
	{ 255, 17, { 0, 2, 2, 0, 0, 0, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0 } },
	{ 347,  1, { 0 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_data _rach_data[] = {
	{   2, 76 },	/* e0   ... e151 */
	{  95, 32 },	/* e152 ... e215 */
	{ 159, 32 },	/* e216 ... e279 */
	{ 223, 32 },	/* e280 ... e343 */
	{ 272, 75 },	/* e344 ... e493 */
	{ -1 },
};

/*! \brief RACH bursts
 *  See GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) - Section 7.4.11
 */
struct gmr1_pi4cxpsk_burst gmr1_rach_burst = {
	.mod = &gmr1_pi4cqpsk,
	.guard_pre = 2,
	.guard_post = 3,
	.len = 39 * 9,
	.ebits = 494,
	.sync = { _rach_sync, NULL },
	.data = _rach_data,
};


/* SDCCH ------------------------------------------------------------------ */

static struct gmr1_pi4cxpsk_sync _sdcch_sync0[] = {
	{  28, 7, { 0, 1, 0, 1, 0, 1, 0 } },
	{ 115, 7, { 1, 0, 1, 0, 1, 0, 1 } },
	{ 197, 7, { 0, 1, 0, 1, 0, 1, 1 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_sync _sdcch_sync1[] = {
	{  28, 7, { 0, 0, 1, 1, 0, 0, 1 } },
	{ 115, 7, { 1, 0, 0, 1, 1, 0, 0 } },
	{ 197, 7, { 1, 1, 0, 0, 1, 1, 1 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_sync _sdcch_sync2[] = {
	{  28, 7, { 0, 0, 0, 0, 1, 1, 1 } },
	{ 115, 7, { 1, 0, 0, 0, 0, 1, 1 } },
	{ 197, 7, { 1, 1, 0, 0, 0, 0, 1 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_sync _sdcch_sync3[] = {
	{  28, 7, { 0, 1, 1, 0, 1, 0, 0 } },
	{ 115, 7, { 1, 0, 1, 1, 0, 1, 0 } },
	{ 197, 7, { 0, 1, 0, 1, 1, 0, 1 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_data _sdcch_data[] = {
	{   2, 26 },	/* e0   ... e25  */
	{  35, 80 },	/* e26  ... e105 */
	{ 122, 75 },	/* e106 ... e180 */
	{ 204, 27 },	/* e181 ... e207 */
	{ -1 },
};

/*! \brief SDCCH bursts
 *  See GMR-1 05.002 (ETSI TS 101 376-5-2 V1.1.1) - Section 7.4.12
 */
struct gmr1_pi4cxpsk_burst gmr1_sdcch_burst = {
	.mod = &gmr1_pi4cbpsk,
	.guard_pre = 2,
	.guard_post = 3,
	.len = 39 * 6,
	.ebits = 208,
	.sync = { _sdcch_sync0, _sdcch_sync1, _sdcch_sync2, _sdcch_sync3 },
	.data = _sdcch_data,
};

/*! @} */
