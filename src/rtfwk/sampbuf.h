/* GMR-1 RT framework: Sample Buffer with producer/consumer model */

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

#ifndef __RTFWK_SAMPBUF_H__
#define __RTFWK_SAMPBUF_H__


#include <complex.h>
#include <stdint.h>

#include <osmocom/core/linuxlist.h>

#include "ringbuf.h"


/* Sample Actor */
/* ------------ */

struct sample_actor;

struct sample_actor_desc {
	int  (*init)(struct sample_actor *sc, void *params);
	void (*fini)(struct sample_actor *sc);
	int  (*work)(struct sample_actor *sc,
	             float complex *data, unsigned int len);
	int priv_size;
};

struct sample_actor {
	/* List */
	struct llist_head list;

	/* Desc */
	const struct sample_actor_desc *desc;

	/* State tracking */
	uint64_t time;

	/* Private data */
	void *priv;
};


struct sample_actor *
sact_alloc(const struct sample_actor_desc *desc, void *params);

void
sact_free(struct sample_actor *sact);


/* Sample Buffer */
/* ------------- */

#define RB_LEN		(1 << 24)	/* 16 Mb */
#define MAX_CHANS	10

struct sample_buf {
	/* Channels */
	int n_chans;

	/* Underlying storage */
	struct osmo_ringbuf *rb[MAX_CHANS];

	/* Time/Sample tracking */
	uint64_t chan_wtime[MAX_CHANS];
	uint64_t chan_rtime[MAX_CHANS];

	/* Producer */
	struct sample_actor *producer[MAX_CHANS];

	/* Consumers */
	struct llist_head consumers[MAX_CHANS];
};


struct sample_buf *
sbuf_alloc(int n_chans);

void
sbuf_free(struct sample_buf *sbuf);


struct sample_actor *
sbuf_set_producer(struct sample_buf *sbuf, int chan_id,
                  const struct sample_actor_desc *desc, void *params);

struct sample_actor *
sbuf_add_consumer(struct sample_buf *sbuf, int chan_id,
                  const struct sample_actor_desc *desc, void *params);


void sbuf_work(struct sample_buf *sbuf);


#endif /* __RTFWK_SAMPBUF_H__ */
