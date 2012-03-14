/* File source/sink for use with Sample Buffer */

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
#include <stdio.h>

#include "sampbuf.h"


#define MAX_CHUNK_SIZE (1 << 14)	/* 128k samples = 1 Mo */


struct sa_file_priv {
	FILE *fh;
};

static int
sa_file_src_init(struct sample_actor *sc, void *params)
{
	struct sa_file_priv *p = sc->priv;
	const char *filename = params;

	p->fh = fopen(filename, "rb");
	if (!p->fh) {
		fprintf(stderr, "[!] File Source: Failed to open input file '%s'\n", filename);
		perror("fopen");
		return -errno;
	}

	return 0;
}

static int
sa_file_sink_init(struct sample_actor *sc, void *params)
{
	struct sa_file_priv *p = sc->priv;
	const char *filename = params;

	p->fh = fopen(filename, "wb");
	if (!p->fh) {
		fprintf(stderr, "[!] File Sink: Failed to open output file '%s'\n", filename);
		perror("fopen");
		return -errno;
	}

	return 0;
}

static void
sa_file_fini(struct sample_actor *sc)
{
	struct sa_file_priv *p = sc->priv;
	fclose(p->fh);
}

static int
sa_file_src_work(struct sample_actor *sc,
                 float complex *data, unsigned int len)
{
	struct sa_file_priv *p = sc->priv;
	size_t rv;

	if (len > MAX_CHUNK_SIZE)
		len = MAX_CHUNK_SIZE;

	rv = fread(data, sizeof(float complex), len, p->fh);

	return rv > 0 ? rv : -1;
}

static int
sa_file_sink_work(struct sample_actor *sc,
                  float complex *data, unsigned int len)
{
	struct sa_file_priv *p = sc->priv;
	size_t rv;

	if (len > MAX_CHUNK_SIZE)
		len = MAX_CHUNK_SIZE;

	rv = fwrite(data, sizeof(float complex), len, p->fh);

	return rv > 0 ? rv : -1;
}


const struct sample_actor_desc sa_file_src = {
	.init = sa_file_src_init,
	.fini = sa_file_fini,
	.work = sa_file_src_work,
	.priv_size = sizeof(struct sa_file_priv),
};

const struct sample_actor_desc sa_file_sink = {
	.init = sa_file_sink_init,
	.fini = sa_file_fini,
	.work = sa_file_sink_work,
	.priv_size = sizeof(struct sa_file_priv),
};
