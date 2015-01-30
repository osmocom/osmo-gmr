/* -*- c++ -*- */
/*
 * Copyright 2015 Sylvain Munaut <tnt@246tNt.com>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <gnuradio/io_signature.h>

#include "burst_to_tagged_stream_impl.h"


namespace gr {
  namespace gmr1 {

	  /* FIXME: Those should be in a common include */
static const pmt::pmt_t SOB_KEY = pmt::string_to_symbol("sob");
static const pmt::pmt_t EOB_KEY = pmt::string_to_symbol("eob");


burst_to_tagged_stream::sptr
burst_to_tagged_stream::make(int max_length, const std::string& len_tag_key)
{
	return gnuradio::get_initial_sptr(
		new burst_to_tagged_stream_impl(max_length, len_tag_key)
	);
}

burst_to_tagged_stream_impl::burst_to_tagged_stream_impl(int max_length, const std::string& len_tag_key)
    : gr::block("burst_to_tagged_stream",
                io_signature::make(1, 1, sizeof(gr_complex)),
                io_signature::make(1, 1, sizeof(gr_complex))),
      d_active(false), d_offset(0),
      d_len_tag_key(pmt::string_to_symbol(len_tag_key))
{
	/* Ensure our output buffer size */
	set_min_output_buffer(max_length * sizeof(gr_complex));

	/* We'll take care of tags ourselves */
	set_tag_propagation_policy(TPP_DONT);
}

burst_to_tagged_stream_impl::~burst_to_tagged_stream_impl()
{
	/* Nothing to do */
}


void
burst_to_tagged_stream_impl::forecast(
	int noutput_items,
	gr_vector_int &ninput_items_required)
{
	ninput_items_required[0] = 1;
}

int
burst_to_tagged_stream_impl::general_work(
		int noutput_items,
		gr_vector_int &ninput_items,
		gr_vector_const_void_star &input_items,
		gr_vector_void_star &output_items)
{
	const gr_complex *in  = reinterpret_cast<const gr_complex *>(input_items[0]);
	gr_complex       *out = reinterpret_cast<      gr_complex *>(output_items[0]);
	std::vector<tag_t> tags, tags_to_copy;
	std::vector<tag_t>::iterator it;
	int avail, n;

	/* If we're not active, drop samples until we are */
	if (!this->d_active)
	{
		get_tags_in_range(tags, 0,
			nitems_read(0),
			nitems_read(0) + ninput_items[0],
			SOB_KEY
		);

		if (tags.empty()) {
			consume_each(ninput_items[0]);
		} else {
			this->d_active = true;
			this->d_offset = 0;

			consume_each(tags[0].offset - nitems_read(0));
		}

		return 0;
	}

	/* We're active ! Look for EOB */
	get_tags_in_range(tags, 0,
		nitems_read(0) + ((this->d_offset == 0) ? 1 : 0),
		nitems_read(0) + ninput_items[0],
		EOB_KEY
	);

	/* Decide how much to copy */
	avail = noutput_items - this->d_offset;

	if (tags.empty())
		/* Copy all we can */
		n = ninput_items[0];
	else
		/* Only until the end of burst */
		n = tags[0].offset - nitems_read(0) + 1;

	/* Are we screwed */
	if (avail < n)
	{
		fprintf(stderr, "SOB without EOB in time ...\n");
		return WORK_DONE;
	}

	/* Copy tags */
		/* If we have the EOB, we exclude tags on it */
	get_tags_in_range(tags_to_copy, 0,
		nitems_read(0),
		nitems_read(0) + n - (tags.empty() ? 0 : 1)
	);

	for (it=tags_to_copy.begin(); it!=tags_to_copy.end(); it++)
	{
		tag_t new_tag = *it;

		/* Exclude some tags */
		if (pmt::eqv(it->key, SOB_KEY) ||
		    pmt::eqv(it->key, EOB_KEY) ||
		    pmt::eqv(it->key, this->d_len_tag_key))
			continue;

		new_tag.offset =
			nitems_written(0) +
			this->d_offset +
			(new_tag.offset - nitems_read(0));

		add_item_tag(0, new_tag);
	}

	/* Copy data */
	memcpy(out + this->d_offset, in, n * sizeof(gr_complex));
	this->d_offset += n;

	/* Was it the end */
	if (tags.empty())
	{
		/* No, don't produce yet */
		consume_each(n);
		return 0;
	}
	else
	{
		/* Yes !, we need to add the length tag */
		add_item_tag(
			0,
			this->nitems_written(0),
			this->d_len_tag_key,
			pmt::from_long(this->d_offset)
		);

		/* Note: Because we may have EOB and next SOB on the same sample, we
		 * leave the last sample in the input buffer */
		consume_each(n - 1);

		/* Prepare for next */
		this->d_active = false;

		return this->d_offset;
	}
}

  } // namespace gmr1
} // namespace gr
