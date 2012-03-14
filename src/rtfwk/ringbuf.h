/* Ring Buffer implementation */

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

#ifndef __OSMO_RINGBUF_H__
#define __OSMO_RINGBUF_H__


struct osmo_ringbuf
{
	/* Memory zone */
	void *base;		/*!< Base pointer */
	unsigned int len;	/*!< Length in bytes */

	/* Pointers */
	unsigned int ri;	/*!< Read index */
	unsigned int wi;	/*!< Write index */
};


int  osmo_rb_init(struct osmo_ringbuf *rb, unsigned int len);
void osmo_rb_deinit(struct osmo_ringbuf *rb);
struct osmo_ringbuf *osmo_rb_alloc(unsigned int len);
void osmo_rb_free(struct osmo_ringbuf *rb);


static inline void
osmo_rb_clear(struct osmo_ringbuf *rb)
{
	rb->wi = 0;
	rb->ri = 0;
}

static inline unsigned int
osmo_rb_used_bytes(struct osmo_ringbuf *rb)
{
	return (rb->wi - rb->ri + rb->len) % rb->len;
}

static inline unsigned int
osmo_rb_free_bytes(struct osmo_ringbuf *rb)
{
	return rb->len - osmo_rb_used_bytes(rb) - 1;
}

static inline void *
osmo_rb_write_ptr(struct osmo_ringbuf *rb)
{
	return rb->base + rb->wi;
}

static inline void *
osmo_rb_read_ptr(struct osmo_ringbuf *rb)
{
	return rb->base + rb->ri;
}

static inline void
osmo_rb_write_advance(struct osmo_ringbuf *rb, unsigned int bytes)
{
	rb->wi = (rb->wi + bytes) % rb->len;
}

static inline void
osmo_rb_read_advance(struct osmo_ringbuf *rb, unsigned int bytes)
{
	rb->ri = (rb->ri + bytes) % rb->len;
}


#endif /* __OSMO_RINGBUF_H__ */
