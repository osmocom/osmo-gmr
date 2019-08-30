/* GMR-1 SDR - metadata handling */

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

/*! \addtogroup meta
 *  @{
 */

/*! \file sdr/metadata.c
 *  \brief Osmocom GMR-1 SDR metadata helpers implementation
 */


#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <osmocom/core/linuxlist.h>

#include <osmocom/gmr1/sdr/metadata.h>


#define MDA_FLUSH_DELAY		10000	/* Flush sample delay */
#define MDA_DEFAULT_DATA	1024	/* Default size of data buffer */
#define MDA_MIN_FREE_DATA	128	/* Minimum free size before trying to put new fields in */

/*! \brief Single 'annotation' entry (multiple fields) at a given position */
struct gmr1_md_annotation
{
	/* Linked list */
	struct llist_head head;
	uint64_t sample;

	/* Data buffer */
	int	data_len;
	int	data_used;
	char *	data_buf;
};

/*! \brief Meta-Data file object */
struct gmr1_metadata
{
	FILE *fh;

	int ann_first;
	struct llist_head ann_free;
	struct llist_head ann_pending;
};


/*! \brief Create and open a new metadata file for an existing sample file
 *  \param[in] meta_filename Filename of the meta file to create
 *  \param[in] data_filename Filename of associated sample file ('core::url')
 *  \param[in] samplerate Sample Rate of sample file ('core::sampling_rate')
 *  \returns The newly opened meta-data file object. NULL for errors.
 *
 *  Calling `gmr1_md_close` is required to make sure the file is properly closed
 *  and valid
 */
struct gmr1_metadata *
gmr1_md_open(const char *meta_filename, const char *data_filename, int samplerate)
{
	struct gmr1_metadata *md;

	/* Struct */
	md = calloc(1, sizeof(struct gmr1_metadata));
	if (!md)
		return NULL;

	/* Init heads & other fields */
	INIT_LLIST_HEAD(&md->ann_free);
	INIT_LLIST_HEAD(&md->ann_pending);

	md->ann_first = 1;

	/* File */
	md->fh = fopen(meta_filename, "w");
	if (!md->fh) {
		free(md);
		return NULL;
	}

	/* Global & Capture infos */
	fprintf(md->fh,
		"{\n"
		" \"global\": {\n"
		"  \"core:datatype\": \"fc32\",\n"
		"  \"core:url\": \"%s\",\n"		/* data_filename */
		"  \"core:version\": \"1.0.0\"\n"
		" },\n"
		" \"capture\": [\n"
		"  {\n"
		"   \"core:sample_start\": 0,\n"
		"   \"core:sampling_rate\": %d\n"	/* samplerate */
		"  }\n"
		" ],\n"
		" \"annotations\": [\n",
		data_filename,
		samplerate
	);

	return md;
}

/*! \brief Properly closes an open meta-data file object
 *  \param[in] md The meta-data file object to close
 *
 *  Calling this is required to make sure the file is fully flushed and valid.
 *
 *  All associated annotations objects from that file will be freed and
 *  will no longer be valid.
 */
void
gmr1_md_close(struct gmr1_metadata *md)
{
	if (!md)
		return;

	/* Flush all pending chunks */
	gmr1_md_flush(md, -1ULL);

	/* Release all the free list */
	struct gmr1_md_annotation *mda, *mda_tmp;

	llist_for_each_entry_safe(mda, mda_tmp, &md->ann_free, head) {
		llist_del(&mda->head);
		gmr1_mda_free(mda);
	}

	/* Terminate annotations & top-level */
	fprintf(md->fh, " ]\n}");

	/* Release everything */
	fclose(md->fh);
	free(md);
}


/*! \brief Allocates an annotation object associated with the given meta-data file object
 *  \param[in] md The meta-data file object
 *  \returns A newly allocated annotation object to be filled by the user
 */
struct gmr1_md_annotation *
gmr1_md_get_annotation(struct gmr1_metadata *md)
{
	/* From free list ? */
	if (!llist_empty(&md->ann_free)) {
		struct gmr1_md_annotation *mda;

		mda = llist_entry(md->ann_free.next, struct gmr1_md_annotation, head);
		llist_del_init(&mda->head);

		return mda;
	}

	/* Allocate a brand new one */
	return gmr1_mda_alloc();
}

/*! \brief Attach an annotation at a given sample position
 *  \param[in] md The meta-data file object to attach to
 *  \param[in] mda The annotation object to attach
 *  \param[in] sample The annotation position ( 'core:sample_start' )
 *
 *  Once attached, the annotation object should not be used by the caller
 *  anymore, it belongs to the meta-data file object and will be freed/recyled
 *  appropriately
 */
void
gmr1_md_put_annotation(struct gmr1_metadata *md, struct gmr1_md_annotation *mda, uint64_t sample)
{
	struct gmr1_md_annotation *mda_scan;

	/* Finish the entry */
	mda->sample = sample;

	/* Add to pending list at right place */
	llist_for_each_entry(mda_scan, &md->ann_pending, head) {
		if (sample <= mda_scan->sample)
			break;
	}

	llist_add_tail(&mda->head, &mda_scan->head);

	/* Scan for potential flushes */
	if (sample >= MDA_FLUSH_DELAY) {
		gmr1_md_flush(md, sample - MDA_FLUSH_DELAY);
	}
}

/*! \brief Flush all annotation up to a given sample to disk
 *  \param[in] md The meta-data file object
 *  \param[in] sample Sample number up to which to flush annotations
 */
void
gmr1_md_flush(struct gmr1_metadata *md, uint64_t sample)
{
	struct gmr1_md_annotation *mda, *mda_tmp;

	/* Scan the pending list */
	llist_for_each_entry_safe(mda, mda_tmp, &md->ann_pending, head) {
		/* Is it the end ? */
		if (mda->sample > sample)
			break;

		/* Write the entry to file */
		gmr1_mda_write(mda, md->fh, md->ann_first);
		md->ann_first = 0;

		/* Move the entry to the free list and reset it */
		gmr1_mda_clear(mda);
		llist_move(&mda->head, &md->ann_free);
	}
}

/*! \brief Allocates an empty annotation object
 *
 *  You should instead use gmr1_md_get_annotation
 */
struct gmr1_md_annotation *
gmr1_mda_alloc(void)
{
	struct gmr1_md_annotation *mda;

	mda = calloc(1, sizeof(struct gmr1_md_annotation));
	if (!mda)
		return NULL;

	mda->data_len  = MDA_DEFAULT_DATA;
	mda->data_used = 0;
	mda->data_buf  = malloc(mda->data_len);
	if (!mda->data_buf)
	{
		free(mda);
		return NULL;
	}

	INIT_LLIST_HEAD(&mda->head);

	return mda;
}

/*! \brief Frees up memory use by an annotation object
 *  \param[in] mda Annotation object to free
 */
void
gmr1_mda_free(struct gmr1_md_annotation *mda)
{
	if (!mda)
		return;

	free(mda->data_buf);
	free(mda);
}

/*! \brief Clear all fields from an annotation and reset it to its default state
 *  \param[in] mda Annotation object to reset
 */
void
gmr1_mda_clear(struct gmr1_md_annotation *mda)
{
	mda->sample = 0;
	mda->data_used = 0;
}

/*! \brief Adds a field to an annotation object
 *  \param[in] mda Annotation object
 *  \param[in] field Field name (fully qualified)
 *  \param[in] format printf-style format string
 */
void
gmr1_mda_add_field(struct gmr1_md_annotation *mda, const char *field, const char *format, ...)
{
	va_list args_org, args_try;
	int data_min_len = mda->data_used + MDA_MIN_FREE_DATA;
	int state = 0;

	va_start(args_org, format);

	while (state < 2)
	{
		int avail, len;

		/* Adjust size if needed */
		if (mda->data_len < data_min_len) {
			mda->data_buf = realloc(mda->data_buf, data_min_len);
			mda->data_len = data_min_len;
		}

		avail = mda->data_len - mda->data_used;

		/* Field name and prefix */
		if (state < 1) {
			len = snprintf(mda->data_buf + mda->data_used, avail, ", \"%s\": ", field);
		}

		/* Data */
		else if (state < 2) {
			va_copy(args_try, args_org);
			len = vsnprintf(mda->data_buf + mda->data_used, avail, format, args_try);
			va_end(args_try);
		}

		/* Process length */
		if (avail > len) {
			/* Enough space, move to next step */
			state++;
			mda->data_used += len;
		} else
			/* Not enough space, resize buffer and retry */
			data_min_len = mda->data_len * 2;
	}

	va_end(args_org);
}

/*! \brief Writes JSON blob of an annotation to a file descriptor
 *  \param[in] mda Annotation object
 *  \param[in] fh File descriptor to write to
 *  \param[in] first Is this the first annotation to be written
 */
void
gmr1_mda_write(struct gmr1_md_annotation *mda, FILE *fh, int first)
{
	if (!mda->data_used)
		return;

	fprintf(fh, "%s   { \"core:sample_start\": %" PRIu64 "%s }",
		first ? "" : ",\n",
		mda->sample,
		mda->data_buf
	);
}

/*! @} */
