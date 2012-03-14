/* Ring Buffer implementation */

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

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "ringbuf.h"


static int
_osmo_rb_create_file(off_t len)
{
	char path[] = "/dev/shm/osmo-rb-XXXXXX";
	mode_t oum;
	int fd, rv;

	/* Make sure only user can use temp file */
	oum = umask(0777);

	/* Create temporary file */
	fd = mkstemp(path);
	if (fd < 0)
		return -errno;

	/* Restore umask */
	umask(oum);

	/* Remove file */
	rv = unlink(path);
	if (rv) {
		close(fd);
		return -errno;
	}

	/* Adjust size */
	rv = ftruncate(fd, len);
	if (rv < 0) {
		close(fd);
		return -errno;
	}

	return fd;
}

static void *
_osmo_rb_mmap_file(int fd, off_t len)
{
	void *base, *p;

	base = mmap(NULL, len << 1, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (base == MAP_FAILED)
		return NULL;

	p = mmap(base, len, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
	if (p != base)
		return NULL;

	p = mmap(base + len, len, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
	if (p != (base + len))
		return NULL;

	return base;
}

int
osmo_rb_init(struct osmo_ringbuf *rb, unsigned int len)
{
	int fd, rv = 0;

	memset(rb, 0x00, sizeof(struct osmo_ringbuf));

	rb->len = len;

	fd = _osmo_rb_create_file(len);
	if (fd < 0)
		return fd;

	rb->base = _osmo_rb_mmap_file(fd, len);
	if (!rb->base)
		rv = -errno;

	close(fd);

	return rv;
}

void
osmo_rb_deinit(struct osmo_ringbuf *rb)
{
	munmap(rb->base, rb->len);
	munmap(rb->base + rb->len, rb->len);
	munmap(rb->base, rb->len << 1);
}

struct osmo_ringbuf *
osmo_rb_alloc(unsigned int len)
{
	struct osmo_ringbuf *rb;

	rb = malloc(sizeof(struct osmo_ringbuf));
	if (!rb)
		return NULL;

	if (osmo_rb_init(rb, len)) {
		free(rb);
		return NULL;
	}

	return rb;
}

void
osmo_rb_free(struct osmo_ringbuf *rb)
{
	if (rb) {
		osmo_rb_deinit(rb);
		free(rb);
	}
}
