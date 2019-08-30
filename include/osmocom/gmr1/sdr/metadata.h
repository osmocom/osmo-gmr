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

#ifndef __OSMO_GMR1_SDR_METADATA__
#define __OSMO_GMR1_SDR_METADATA__

/*! \defgroup meta SDR metadata helpers
 *  \ingroup sdr
 *  @{
 */

/*! \file sdr/metadata.h
 *  \brief Osmocom GMR-1 SDR metadata helpers
 */

#include <stdint.h>
#include <stdio.h>

struct gmr1_md_annotation;
struct gmr1_metadata;

struct gmr1_metadata *gmr1_md_open(const char *meta_filename, const char *data_filename, int samplerate);
void gmr1_md_close(struct gmr1_metadata *md);

struct gmr1_md_annotation *gmr1_md_get_annotation(struct gmr1_metadata *md);
void gmr1_md_put_annotation(struct gmr1_metadata *md, struct gmr1_md_annotation *mda, uint64_t sample);
void gmr1_md_flush(struct gmr1_metadata *md, uint64_t sample);

struct gmr1_md_annotation *gmr1_mda_alloc(void);
void gmr1_mda_free(struct gmr1_md_annotation *mda);
void gmr1_mda_clear(struct gmr1_md_annotation *mda);
void gmr1_mda_add_field(struct gmr1_md_annotation *mda, const char *field, const char *format, ...);
void gmr1_mda_write(struct gmr1_md_annotation *mda, FILE *fh, int first);


/*! @} */

#endif /* __OSMO_GMR1_SDR_METADATA__ */
