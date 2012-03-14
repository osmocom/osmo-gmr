/* Sample Buffer with producer / consummer model */

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

#include <complex.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <osmocom/core/linuxlist.h>

#include "ringbuf.h"
#include "sampbuf.h"


/* ------------------------------------------------------------------------ */
/* Sample Actor                                                             */
/* ------------------------------------------------------------------------ */

struct sample_actor *
sact_alloc(const struct sample_actor_desc *desc, void *params)
{
	struct sample_actor *sact;
	int rv;

	sact = calloc(1, sizeof(struct sample_actor));
	if (!sact)
		return NULL;

	INIT_LLIST_HEAD(&sact->list);

	sact->desc = desc;

	if (desc->priv_size > 0) {
		sact->priv = calloc(1, desc->priv_size);
		if (!sact->priv)
			goto err;
	}

	rv = sact->desc->init(sact, params);
	if (rv)
		goto err;

	return sact;

err:
	free(sact->priv);
	free(sact);
	return NULL;
}

void
sact_free(struct sample_actor *sact)
{
	if (sact) {
		sact->desc->fini(sact);
		if (sact->desc->priv_size > 0)
			free(sact->priv);
		free(sact);
	}
}


/* ------------------------------------------------------------------------ */
/* Sample Buffer                                                            */
/* ------------------------------------------------------------------------ */

struct sample_buf *
sbuf_alloc(int n_chans)
{
	struct sample_buf *sbuf;
	int i;

	sbuf = calloc(1, sizeof(struct sample_buf));
	if (!sbuf)
		return NULL;

	sbuf->n_chans = n_chans;

	for (i=0; i<n_chans; i++)
	{
		INIT_LLIST_HEAD(&sbuf->consumers[i]);

		sbuf->rb[i] = osmo_rb_alloc(RB_LEN);
		if (!sbuf->rb[i]) {
			sbuf_free(sbuf);
			return NULL;
		}
	}

	return sbuf;
}

void
sbuf_free(struct sample_buf *sbuf)
{
	int i;

	if (!sbuf)
		return;

	for (i=0; i<sbuf->n_chans; i++)
	{
		sact_free(sbuf->producer[i]);

		/* FIXME release consumers */

		osmo_rb_free(sbuf->rb[i]);
	}

	free(sbuf);
}


struct sample_actor *
sbuf_set_producer(struct sample_buf *sbuf, int chan_id,
                  const struct sample_actor_desc *desc, void *params)
{
	struct sample_actor *sact = NULL;

	sact_free(sbuf->producer[chan_id]);

	if (desc) {
		sact = sact_alloc(desc, params);
		if (!sact)
			return NULL;

		sact->time = sbuf->chan_wtime[chan_id];
	}

	sbuf->producer[chan_id] = sact;

	return sact;
}

struct sample_actor *
sbuf_add_consumer(struct sample_buf *sbuf, int chan_id,
                  const struct sample_actor_desc *desc, void *params)
{
	struct sample_actor *sact;

	sact = sact_alloc(desc, params);
	if (!sact)
		return NULL;

	sact->time = sbuf->chan_rtime[chan_id];

	llist_add(&sact->list, &sbuf->consumers[chan_id]);

	return sact;
}


#define WORK_CHUNK	(1 << 17)	/* 128k samples */

static int
_sbuf_chan_produce(struct sample_buf *sbuf, int chan_id)
{
	struct sample_actor *sact;
	float complex *data;
	int rv, free;

	/* Check free space */
	free = osmo_rb_free_bytes(sbuf->rb[chan_id]) / sizeof(float complex);
	if (free < WORK_CHUNK)
		return 0;

	/* Get producer */
	sact = sbuf->producer[chan_id];
	if (!sact)
		return 0;

	/* Get where to write */
	data = osmo_rb_write_ptr(sbuf->rb[chan_id]);

	/* Do some work */
	rv = sact->desc->work(sact, data, WORK_CHUNK);

	/* If nothing was done, return directly */
	if (!rv)
		return 0;

	/* If < 0, then this producer is done */
	if (rv < 0) {
		sbuf_set_producer(sbuf, chan_id, NULL, NULL);
		return 0;
	}

	/* Update state */
	osmo_rb_write_advance(sbuf->rb[chan_id], sizeof(float complex) * rv);

	sact->time += rv;
	sbuf->chan_wtime[chan_id] += rv;

	return 1;
}

static int
_sbuf_produce(struct sample_buf *sbuf)
{
	int i;
	int work_done = 0;

	for (i=0; i<sbuf->n_chans; i++)
		work_done |= _sbuf_chan_produce(sbuf, i);

	return work_done;
}

static int
_sbuf_chan_consume(struct sample_buf *sbuf, int chan_id)
{
	struct sample_actor *sact, *tmp;
	float complex *data;
	uint64_t rtime;
	int used, rv;
	int work_done = 0;

	/* Check available data */
	used = osmo_rb_used_bytes(sbuf->rb[chan_id]) / sizeof(float complex);

	/* Get where to write & matchine timestamp */
	data = osmo_rb_read_ptr(sbuf->rb[chan_id]);
	rtime = sbuf->chan_rtime[chan_id];

	/* Scan all consumers */
	llist_for_each_entry_safe(sact, tmp, &sbuf->consumers[chan_id], list)
	{
		int adv = sact->time - rtime;

		/* Can we do anything ? */
		if (used == adv)
			continue;

		/* Do some work */
		rv = sact->desc->work(sact, &data[adv], used - adv);

		/* If nothing was done, ... next */
		if (!rv)
			continue;

		/* If < 0, then this consumer is done */
		if (rv < 0) {
			llist_del(&sact->list);
			sact_free(sact);
			continue;
		}

		/* Update state */
		sact->time += rv;

		work_done = 1;
	}

	/* If we did no work and no producer left, we remove all consumers */
	if (!work_done && !sbuf->producer[chan_id]) {
		llist_for_each_entry_safe(sact, tmp, &sbuf->consumers[chan_id], list)
		{
			llist_del(&sact->list);
			sact_free(sact);
		}
	}

	return work_done;
}

static int
_sbuf_consume(struct sample_buf *sbuf)
{
	int i, found;
	uint64_t rtime;
	int work_done = 0;

	/* Consume data */
	for (i=0; i<sbuf->n_chans; i++)
		work_done |= _sbuf_chan_consume(sbuf, i);

	/* Find time up to where we can discard */
	found = 0;

	for (i=0; i<sbuf->n_chans; i++)
	{
		struct sample_actor *sact, *tmp;

		llist_for_each_entry_safe(sact, tmp, &sbuf->consumers[i], list)
		{
			if (!found || (rtime > sact->time)) {
				rtime = sact->time;
				found = 1;
			}
		}
	}

	if (!found)
		return 0;

	/* Actually discard */
	for (i=0; i<sbuf->n_chans; i++) {
		int discard_bytes = (rtime - sbuf->chan_rtime[i]) * sizeof(float complex);
		if (osmo_rb_used_bytes(sbuf->rb[i]) >= discard_bytes)
			osmo_rb_read_advance(sbuf->rb[i], discard_bytes);
		else
			osmo_rb_clear(sbuf->rb[i]);
		sbuf->chan_rtime[i] = rtime;
	}

	return work_done;
}

void
sbuf_work(struct sample_buf *sbuf)
{
	int i, rv;
	int has_produced, has_consumed;
	int has_producers, has_consumers;

	while (1) {
		/* Produce / Consume */
		has_produced = _sbuf_produce(sbuf);

		has_consumed = 0;
		do {
			rv = _sbuf_consume(sbuf);
			has_consumed |= rv;
		} while (rv);

		/* Check if there is any producers left */
		has_producers = 0;
		for (i=0; i<sbuf->n_chans; i++)
			has_producers |= (sbuf->producer[i] != NULL);

		/* Check if there is any consumer left */
		for (i=0; i<sbuf->n_chans; i++)
			if (!llist_empty(&sbuf->consumers[i]))
				break;
		has_consumers = (i != sbuf->n_chans);

		/* Check exit conditions */
		if (!has_consumers)
			break;

		if (!has_consumed && !has_producers)
			break;
	}
}
