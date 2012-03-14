/* GMR-1 Demo RX live application */

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
#include <stdlib.h>
#include <string.h>

#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>

#include <osmocom/gmr1/sdr/fcch.h>

#include "rtfwk/common.h"
#include "rtfwk/sampbuf.h"
#include "rtfwk/sa_fcch.h"
#include "rtfwk/sa_file.h"


/* Main ------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	struct app_state _as, *as = &_as;
	int rv = 0, i;

	/* Init app state */
	memset(as, 0x00, sizeof(struct app_state));

	/* Args */
	if (argc < 3) {
		fprintf(stderr, "Usage: %s sps arfcn1:file1 [arfcn2:file2] ...\n", argv[0]);
		return -EINVAL;
	}

	as->n_chans = argc - 2;

	as->sps = atoi(argv[1]);
	if (as->sps < 1 || as->sps > 16) {
		fprintf(stderr, "[!] sps must be withing [1,16]\n");
		return -EINVAL;
	}

	/* Init GSMTap */
	as->gti = gsmtap_source_init("127.0.0.1", GSMTAP_UDP_PORT, 0);
	gsmtap_source_add_sink(as->gti);

	/* Buffer */
	as->buf = sbuf_alloc(as->n_chans);
	if (!as->buf) {
		rv = -ENOMEM;
		goto err;
	}

	/* Parse arguments */
	for (i=0; i<as->n_chans; i++)
	{
		char *d;

		d = strchr(argv[i+2], ':');
		if (!d) {
			fprintf(stderr, "[!] Arguments must be of the form arfcn:filename\n");
			rv = -EINVAL;
			goto err;
		}

		*d = '\0';

		as->arfcn[i] = atoi(argv[i+2]);
		as->filename[i] = d+1;
	}

	/* Create all the sources */
	for (i=0; i<as->n_chans; i++) {
		struct sample_actor *sa;
		sa = sbuf_set_producer(as->buf, i, &sa_file_src, as->filename[i]);
		if (!sa) {
			fprintf(stderr, "[!] Failed to create source for stream #%d\n", i);
			rv = -EIO;
			goto err;
		}
	}

	/* Attribute single 'FCCH detect' sink to each channel */
	for (i=0; i<as->n_chans; i++) {
		struct sample_actor *sa;
		struct fcch_sink_params p = {
			.as = as,
			.chan_id = i,
			.start_discard = 5000,
			.burst_type = &gmr1_fcch_burst,
		};
		sa = sbuf_add_consumer(as->buf, i, &fcch_sink, &p);
		if (!sa) {
			fprintf(stderr, "[!] Failed to create FCCH sink for stream #%d\n", i);
			rv = -ENOMEM;
			goto err;
		}
	}

	/* Go forth and process ! */
	sbuf_work(as->buf);

	/* Done ! */
	rv = 0;

	/* Clean up */
err:
	sbuf_free(as->buf);

	return rv;
}
