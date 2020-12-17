///////////////////////////////////////////////////////////////////////////////
//
/// \file       file_info.c
/// \brief      Decode .xz file information into a lzma_index structure
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "index_decoder.h"


typedef struct {
	enum {
		SEQ_MAGIC_BYTES,
		SEQ_PADDING_SEEK,
		SEQ_PADDING_DECODE,
		SEQ_FOOTER,
		SEQ_INDEX_INIT,
		SEQ_INDEX_DECODE,
		SEQ_HEADER_DECODE,
		SEQ_HEADER_COMPARE,
	} sequence;

	/// Absolute position of in[*in_pos] in the file. All code that
	/// modifies *in_pos also updates this. seek_to_pos() needs this
	/// to determine if we need to request the application to seek for
	/// us or if we can do the seeking internally by adjusting *in_pos.
	uint64_t file_cur_pos;

	/// This refers to absolute positions of interesting parts of the
	/// input file. Sometimes it points to the *beginning* of a specific
	/// field and sometimes to the *end* of a field. The current target
	/// position at each moment is explained in the comments.
	uint64_t file_target_pos;

	/// Size of the .xz file (from the application).
	uint64_t file_size;

	/// Index decoder
	lzma_next_coder index_decoder;

	/// Number of bytes remaining in the Index field that is currently
	/// being decoded.
	lzma_vli index_remaining;

	/// The Index decoder will store the decoded Index in this pointer.
	lzma_index *this_index;

	/// Amount of Stream Padding in the current Stream.
	lzma_vli stream_padding;

	/// The final combined index is collected here.
	lzma_index *combined_index;

	/// Pointer from the application where to store the index information
	/// after successful decoding.
	lzma_index **dest_index;

	/// Pointer to lzma_stream.seek_pos to be used when returning
	/// LZMA_SEEK_NEEDED. This is set by seek_to_pos() when needed.
	uint64_t *external_seek_pos;

	/// Memory usage limit
	uint64_t memlimit;

	/// Stream Flags from the very beginning of the file.
	lzma_stream_flags first_header_flags;

	/// Stream Flags from Stream Header of the current Stream.
	lzma_stream_flags header_flags;

	/// Stream Flags from Stream Footer of the current Stream.
	lzma_stream_flags footer_flags;

	size_t temp_pos;
	size_t temp_size;
	uint8_t temp[8192];

} lzma_file_info_coder;


/// Copies data from in[*in_pos] into coder->temp until
/// coder->temp_pos == coder->temp_size. This also keeps coder->file_cur_pos
/// in sync with *in_pos. Returns true if more input is needed.
static bool
fill_temp(lzma_file_info_coder *coder, const uint8_t *restrict in,
		size_t *restrict in_pos, size_t in_size)
{
	coder->file_cur_pos += lzma_bufcpy(in, in_pos, in_size,
			coder->temp, &coder->temp_pos, coder->temp_size);
	return coder->temp_pos < coder->temp_size;
}


/// Seeks to the absolute file position specified by target_pos.
/// This tries to do the seeking by only modifying *in_pos, if possible.
/// The main benefit of this is that if one passes the whole file at once
/// to lzma_code(), the decoder will never need to return LZMA_SEEK_NEEDED
/// as all the seeking can be done by adjusting *in_pos in this function.
///
/// Returns true if an external seek is needed and the caller must return
/// LZMA_SEEK_NEEDED.
static bool
seek_to_pos(lzma_file_info_coder *coder, uint64_t target_pos,
		size_t in_start, size_t *in_pos, size_t in_size)
{
	// The input buffer doesn't extend beyond the end of the file.
	// This has been checked by file_info_decode() already.
	assert(coder->file_size - coder->file_cur_pos >= in_size - *in_pos);

	const uint64_t pos_min = coder->file_cur_pos - (*in_pos - in_start);
	const uint64_t pos_max = coder->file_cur_pos + (in_size - *in_pos);

	bool external_seek_needed;

	if (target_pos >= pos_min && target_pos <= pos_max) {
		// The requested position is available in the current input
		// buffer or right after it. That is, in a corner case we
		// end up setting *in_pos == in_size and thus will immediately
		// need new input bytes from the application.
		*in_pos += (size_t)(target_pos - coder->file_cur_pos);
		external_seek_needed = false;
	} else {
		// Ask the application to seek the input file.
		*coder->external_seek_pos = target_pos;
		external_seek_needed = true;

		// Mark the whole input buffer as used. This way
		// lzma_stream.total_in will have a better estimate
		// of the amount of data read. It still won't be perfect
		// as the value will depend on the input buffer size that
		// the application uses, but it should be good enough for
		// those few who want an estimate.
		*in_pos = in_size;
	}

	// After seeking (internal or external) the current position
	// will match the requested target position.
	coder->file_cur_pos = target_pos;

	return external_seek_needed;
}


/// The caller sets coder->file_target_pos so that it points to the *end*
/// of the desired file position. This function then determines how far
/// backwards from that position we can seek. After seeking fill_temp()
/// can be used to read data into coder->temp. When fill_temp() has finished,
/// coder->temp[coder->temp_size] will match coder->file_target_pos.
///
/// This also validates that coder->target_file_pos is sane in sense that
/// we aren't trying to seek too far backwards (too close or beyond the
/// beginning of the file).
static lzma_ret
reverse_seek(lzma_file_info_coder *coder,
		size_t in_start, size_t *in_pos, size_t in_size)
{
	// Check that there is enough data before the target position
	// to contain at least Stream Header and Stream Footer. If there
	// isn't, the file cannot be valid.
	if (coder->file_target_pos < 2 * LZMA_STREAM_HEADER_SIZE)
		return LZMA_DATA_ERROR;

	coder->temp_pos = 0;

	// The Stream Header at the very beginning of the file gets handled
	// specially in SEQ_MAGIC_BYTES and thus we will never need to seek
	// there. By not seeking to the first LZMA_STREAM_HEADER_SIZE bytes
	// we avoid a useless external seek after SEQ_MAGIC_BYTES if the
	// application uses an extremely small input buffer and the input
	// file is very small.
	if (coder->file_target_pos - LZMA_STREAM_HEADER_SIZE
			< sizeof(coder->temp))
		coder->temp_size = (size_t)(coder->file_target_pos
				- LZMA_STREAM_HEADER_SIZE);
	else
		coder->temp_size = sizeof(coder->temp);

	// The above if-statements guarantee this. This is important because
	// the Stream Header/Footer decoders assume that there's at least
	// LZMA_STREAM_HEADER_SIZE bytes in coder->temp.
	assert(coder->temp_size >= LZMA_STREAM_HEADER_SIZE);

	if (seek_to_pos(coder, coder->file_target_pos - coder->temp_size,
			in_start, in_pos, in_size))
		return LZMA_SEEK_NEEDED;

	return LZMA_OK;
}


/// Gets the number of zero-bytes at the end of the buffer.
static size_t
get_padding_size(const uint8_t *buf, size_t buf_size)
{
	size_t padding = 0;
	while (buf_size > 0 && buf[--buf_size] == 0x00)
		++padding;

	return padding;
}


/// With the Stream Header at the very beginning of the file, LZMA_FORMAT_ERROR
/// is used to tell the application that Magic Bytes didn't match. In other
/// Stream Header/Footer fields (in the middle/end of the file) it could be
/// a bit confusing to return LZMA_FORMAT_ERROR as we already know that there
/// is a valid Stream Header at the beginning of the file. For those cases
/// this function is used to convert LZMA_FORMAT_ERROR to LZMA_DATA_ERROR.
static lzma_ret
hide_format_error(lzma_ret ret)
{
	if (ret == LZMA_FORMAT_ERROR)
		ret = LZMA_DATA_ERROR;

	return ret;
}


/// Calls the Index decoder and updates coder->index_remaining.
/// This is a separate function because the input can be either directly
/// from the application or from coder->temp.
static lzma_ret
decode_index(lzma_file_info_coder *coder, const lzma_allocator *allocator,
		const uint8_t *restrict in, size_t *restrict in_pos,
		size_t in_size, bool update_file_cur_pos)
{
	const size_t in_start = *in_pos;

	const lzma_ret ret = coder->index_decoder.code(
			coder->index_decoder.coder,
			allocator, in, in_pos, in_size,
			NULL, NULL, 0, LZMA_RUN);

	coder->index_remaining -= *in_pos - in_start;

	if (update_file_cur_pos)
		coder->file_cur_pos += *in_pos - in_start;

	return ret;
}


static lzma_ret
file_info_decode(void *coder_ptr, const lzma_allocator *allocator,
		const uint8_t *restrict in, size_t *restrict in_pos,
		size_t in_size,
		uint8_t *restrict out lzma_attribute((__unused__)),
		size_t *restrict out_pos lzma_attribute((__unused__)),
		size_t out_size lzma_attribute((__unused__)),
		lzma_action action lzma_attribute((__unused__)))
{
	lzma_file_info_coder *coder = coder_ptr;
	const size_t in_start = *in_pos;

	// If the caller provides input past the end of the file, trim
	// the extra bytes from the buffer so that we won't read too far.
	assert(coder->file_size >= coder->file_cur_pos);
	if (coder->file_size - coder->file_cur_pos < in_size - in_start)
		in_size = in_start
			+ (size_t)(coder->file_size - coder->file_cur_pos);

	while (true)
	switch (coder->sequence) {
	case SEQ_MAGIC_BYTES:
		// Decode the Stream Header at the beginning of the file
		// first to check if the Magic Bytes match. The flags
		// are stored in coder->first_header_flags so that we
		// don't need to seek to it again.
		//
		// Check that the file is big enough to contain at least
		// Stream Header.
		if (coder->file_size < LZMA_STREAM_HEADER_SIZE)
			return LZMA_FORMAT_ERROR;

		// Read the Stream Header field into coder->temp.
		if (fill_temp(coder, in, in_pos, in_size))
			return LZMA_OK;

		// This is the only Stream Header/Footer decoding where we
		// want to return LZMA_FORMAT_ERROR if the Magic Bytes don't
		// match. Elsewhere it will be converted to LZMA_DATA_ERROR.
		return_if_error(lzma_stream_header_decode(
				&coder->first_header_flags, coder->temp));

		// Now that we know that the Magic Bytes match, check the
		// file size. It's better to do this here after checking the
		// Magic Bytes since this way we can give LZMA_FORMAT_ERROR
		// instead of LZMA_DATA_ERROR when the Magic Bytes don't
		// match in a file that is too big or isn't a multiple of
		// four bytes.
		if (coder->file_size > LZMA_VLI_MAX || (coder->file_size & 3))
			return LZMA_DATA_ERROR;

		// Start looking for Stream Padding and Stream Footer
		// at the end of the file.
		coder->file_target_pos = coder->file_size;

	// Fall through

	case SEQ_PADDING_SEEK:
		coder->sequence = SEQ_PADDING_DECODE;
		return_if_error(reverse_seek(
				coder, in_start, in_pos, in_size));

	// Fall through

	case SEQ_PADDING_DECODE: {
		// Copy to coder->temp first. This keeps the code simpler if
		// the application only provides input a few bytes at a time.
		if (fill_temp(coder, in, in_pos, in_size))
			return LZMA_OK;

		// Scan the buffer backwards to get the size of the
		// Stream Padding field (if any).
		const size_t new_padding = get_padding_size(
				coder->temp, coder->temp_size);
		coder->stream_padding += new_padding;

		// Set the target position to the beginning of Stream Padding
		// that has been observed so far. If all Stream Padding has
		// been seen, then the target position will be at the end
		// of the Stream Footer field.
		coder->file_target_pos -= new_padding;

		if (new_padding == coder->temp_size) {
			// The whole buffer was padding. Seek backwards in
			// the file to get more input.
			coder->sequence = SEQ_PADDING_SEEK;
			break;
		}

		// Size of Stream Padding must be a multiple of 4 bytes.
		if (coder->stream_padding & 3)
			return LZMA_DATA_ERROR;

		coder->sequence = SEQ_FOOTER;

		// Calculate the amount of non-padding data in coder->temp.
		coder->temp_size -= new_padding;
		coder->temp_pos = coder->temp_size;

		// We can avoid an external seek if the whole Stream Footer
		// is already in coder->temp. In that case SEQ_FOOTER won't
		// read more input and will find the Stream Footer from
		// coder->temp[coder->temp_size - LZMA_STREAM_HEADER_SIZE].
		//
		// Otherwise we will need to seek. The seeking is done so
		// that Stream Footer wil be at the end of coder->temp.
		// This way it's likely that we also get a complete Index
		// field into coder->temp without needing a separate seek
		// for that (unless the Index field is big).
		if (coder->temp_size < LZMA_STREAM_HEADER_SIZE)
			return_if_error(reverse_seek(
					coder, in_start, in_pos, in_size));
	}

	// Fall through

	case SEQ_FOOTER:
		// Copy the Stream Footer field into coder->temp.
		// If Stream Footer was already available in coder->temp
		// in SEQ_PADDING_DECODE, then this does nothing.
		if (fill_temp(coder, in, in_pos, in_size))
			return LZMA_OK;

		// Make coder->file_target_pos and coder->temp_size point
		// to the beginning of Stream Footer and thus to the end
		// of the Index field. coder->temp_pos will be updated
		// a bit later.
		coder->file_target_pos -= LZMA_STREAM_HEADER_SIZE;
		coder->temp_size -= LZMA_STREAM_HEADER_SIZE;

		// Decode Stream Footer.
		return_if_error(hide_format_error(lzma_stream_footer_decode(
				&coder->footer_flags,
				coder->temp + coder->temp_size)));

		// Check that we won't seek past the beginning of the file.
		//
		// LZMA_STREAM_HEADER_SIZE is added because there must be
		// space for Stream Header too even though we won't seek
		// there before decoding the Index field.
		//
		// There's no risk of integer overflow here because
		// Backward Size cannot be greater than 2^34.
		if (coder->file_target_pos < coder->footer_flags.backward_size
				+ LZMA_STREAM_HEADER_SIZE)
			return LZMA_DATA_ERROR;

		// Set the target position to the beginning of the Index field.
		coder->file_target_pos -= coder->footer_flags.backward_size;
		coder->sequence = SEQ_INDEX_INIT;

		// We can avoid an external seek if the whole Index field is
		// already available in coder->temp.
		if (coder->temp_size >= coder->footer_flags.backward_size) {
			// Set coder->temp_pos to point to the beginning
			// of the Index.
			coder->temp_pos = coder->temp_size
					- coder->footer_flags.backward_size;
		} else {
			// These are set to zero to indicate that there's no
			// useful data (Index or anything else) in coder->temp.
			coder->temp_pos = 0;
			coder->temp_size = 0;

			// Seek to the beginning of the Index field.
			if (seek_to_pos(coder, coder->file_target_pos,
					in_start, in_pos, in_size))
				return LZMA_SEEK_NEEDED;
		}

	// Fall through

	case SEQ_INDEX_INIT: {
		// Calculate the amount of memory already used by the earlier
		// Indexes so that we know how big memory limit to pass to
		// the Index decoder.
		//
		// NOTE: When there are multiple Streams, the separate
		// lzma_index structures can use more RAM (as measured by
		// lzma_index_memused()) than the final combined lzma_index.
		// Thus memlimit may need to be slightly higher than the final
		// calculated memory usage will be. This is perhaps a bit
		// confusing to the application, but I think it shouldn't
		// cause problems in practice.
		uint64_t memused = 0;
		if (coder->combined_index != NULL) {
			memused = lzma_index_memused(coder->combined_index);
			assert(memused <= coder->memlimit);
			if (memused > coder->memlimit) // Extra sanity check
				return LZMA_PROG_ERROR;
		}

		// Initialize the Index decoder.
		return_if_error(lzma_index_decoder_init(
				&coder->index_decoder, allocator,
				&coder->this_index,
				coder->memlimit - memused));

		coder->index_remaining = coder->footer_flags.backward_size;
		coder->sequence = SEQ_INDEX_DECODE;
	}

	// Fall through

	case SEQ_INDEX_DECODE: {
		// Decode (a part of) the Index. If the whole Index is already
		// in coder->temp, read it from there. Otherwise read from
		// in[*in_pos] onwards. Note that index_decode() updates
		// coder->index_remaining and optionally coder->file_cur_pos.
		lzma_ret ret;
		if (coder->temp_size != 0) {
			assert(coder->temp_size - coder->temp_pos
					== coder->index_remaining);
			ret = decode_index(coder, allocator, coder->temp,
					&coder->temp_pos, coder->temp_size,
					false);
		} else {
			// Don't give the decoder more input than the known
			// remaining size of the Index field.
			size_t in_stop = in_size;
			if (in_size - *in_pos > coder->index_remaining)
				in_stop = *in_pos
					+ (size_t)(coder->index_remaining);

			ret = decode_index(coder, allocator,
					in, in_pos, in_stop, true);
		}

		switch (ret) {
		case LZMA_OK:
			// If the Index docoder asks for more input when we
			// have already given it as much input as Backward Size
			// indicated, the file is invalid.
			if (coder->index_remaining == 0)
				return LZMA_DATA_ERROR;

			// We cannot get here if we were reading Index from
			// coder->temp because when reading from coder->temp
			// we give the Index decoder exactly
			// coder->index_remaining bytes of input.
			assert(coder->temp_size == 0);

			return LZMA_OK;

		case LZMA_STREAM_END:
			// If the decoding seems to be successful, check also
			// that the Index decoder consumed as much input as
			// indicated by the Backward Size field.
			if (coder->index_remaining != 0)
				return LZMA_DATA_ERROR;

			break;

		default:
			return ret;
		}

		// Calculate how much the Index tells us to seek backwards
		// (relative to the beginning of the Index): Total size of
		// all Blocks plus the size of the Stream Header field.
		// No integer overflow here because lzma_index_total_size()
		// cannot return a value greater than LZMA_VLI_MAX.
		const uint64_t seek_amount
				= lzma_index_total_size(coder->this_index)
					+ LZMA_STREAM_HEADER_SIZE;

		// Check that Index is sane in sense that seek_amount won't
		// make us seek past the beginning of the file when locating
		// the Stream Header.
		//
		// coder->file_target_pos still points to the beginning of
		// the Index field.
		if (coder->file_target_pos < seek_amount)
			return LZMA_DATA_ERROR;

		// Set the target to the beginning of Stream Header.
		coder->file_target_pos -= seek_amount;

		if (coder->file_target_pos == 0) {
			// We would seek to the beginning of the file, but
			// since we already decoded that Stream Header in
			// SEQ_MAGIC_BYTES, we can use the cached value from
			// coder->first_header_flags to avoid the seek.
			coder->header_flags = coder->first_header_flags;
			coder->sequence = SEQ_HEADER_COMPARE;
			break;
		}

		coder->sequence = SEQ_HEADER_DECODE;

		// Make coder->file_target_pos point to the end of
		// the Stream Header field.
		coder->file_target_pos += LZMA_STREAM_HEADER_SIZE;

		// If coder->temp_size is non-zero, it points to the end
		// of the Index field. Then the beginning of the Index
		// field is at coder->temp[coder->temp_size
		// - coder->footer_flags.backward_size].
		assert(coder->temp_size == 0 || coder->temp_size
				>= coder->footer_flags.backward_size);

		// If coder->temp contained the whole Index, see if it has
		// enough data to contain also the Stream Header. If so,
		// we avoid an external seek.
		//
		// NOTE: This can happen only with small .xz files and only
		// for the non-first Stream as the Stream Flags of the first
		// Stream are cached and already handled a few lines above.
		// So this isn't as useful as the other seek-avoidance cases.
		if (coder->temp_size != 0 && coder->temp_size
				- coder->footer_flags.backward_size
				>= seek_amount) {
			// Make temp_pos and temp_size point to the *end* of
			// Stream Header so that SEQ_HEADER_DECODE will find
			// the start of Stream Header from coder->temp[
			// coder->temp_size - LZMA_STREAM_HEADER_SIZE].
			coder->temp_pos = coder->temp_size
					- coder->footer_flags.backward_size
					- seek_amount
					+ LZMA_STREAM_HEADER_SIZE;
			coder->temp_size = coder->temp_pos;
		} else {
			// Seek so that Stream Header will be at the end of
			// coder->temp. With typical multi-Stream files we
			// will usually also get the Stream Footer and Index
			// of the *previous* Stream in coder->temp and thus
			// won't need a separate seek for them.
			return_if_error(reverse_seek(coder,
					in_start, in_pos, in_size));
		}
	}

	// Fall through

	case SEQ_HEADER_DECODE:
		// Copy the Stream Header field into coder->temp.
		// If Stream Header was already available in coder->temp
		// in SEQ_INDEX_DECODE, then this does nothing.
		if (fill_temp(coder, in, in_pos, in_size))
			return LZMA_OK;

		// Make all these point to the beginning of Stream Header.
		coder->file_target_pos -= LZMA_STREAM_HEADER_SIZE;
		coder->temp_size -= LZMA_STREAM_HEADER_SIZE;
		coder->temp_pos = coder->temp_size;

		// Decode the Stream Header.
		return_if_error(hide_format_error(lzma_stream_header_decode(
				&coder->header_flags,
				coder->temp + coder->temp_size)));

		coder->sequence = SEQ_HEADER_COMPARE;

	// Fall through

	case SEQ_HEADER_COMPARE:
		// Compare Stream Header against Stream Footer. They must
		// match.
		return_if_error(lzma_stream_flags_compare(
				&coder->header_flags, &coder->footer_flags));

		// Store the decoded Stream Flags into the Index. Use the
		// Footer Flags because it contains Backward Size, although
		// it shouldn't matter in practice.
		if (lzma_index_stream_flags(coder->this_index,
				&coder->footer_flags) != LZMA_OK)
			return LZMA_PROG_ERROR;

		// Store also the size of the Stream Padding field. It is
		// needed to calculate the offsets of the Streams correctly.
		if (lzma_index_stream_padding(coder->this_index,
				coder->stream_padding) != LZMA_OK)
			return LZMA_PROG_ERROR;

		// Reset it so that it's ready for the next Stream.
		coder->stream_padding = 0;

		// Append the earlier decoded Indexes after this_index.
		if (coder->combined_index != NULL)
			return_if_error(lzma_index_cat(coder->this_index,
					coder->combined_index, allocator));

		coder->combined_index = coder->this_index;
		coder->this_index = NULL;

		// If the whole file was decoded, tell the caller that we
		// are finished.
		if (coder->file_target_pos == 0) {
			// The combined index must indicate the same file
			// size as was told to us at initialization.
			assert(lzma_index_file_size(coder->combined_index)
					== coder->file_size);

			// Make the combined index available to
			// the application.
			*coder->dest_index = coder->combined_index;
			coder->combined_index = NULL;

			// Mark the input buffer as used since we may have
			// done internal seeking and thus don't know how
			// many input bytes were actually used. This way
			// lzma_stream.total_in gets a slightly better
			// estimate of the amount of input used.
			*in_pos = in_size;
			return LZMA_STREAM_END;
		}

		// We didn't hit the beginning of the file yet, so continue
		// reading backwards in the file. If we have unprocessed
		// data in coder->temp, use it before requesting more data
		// from the application.
		//
		// coder->file_target_pos, coder->temp_size, and
		// coder->temp_pos all point to the beginning of Stream Header
		// and thus the end of the previous Stream in the file.
		coder->sequence = coder->temp_size > 0
				? SEQ_PADDING_DECODE : SEQ_PADDING_SEEK;
		break;

	default:
		assert(0);
		return LZMA_PROG_ERROR;
	}
}


static lzma_ret
file_info_decoder_memconfig(void *coder_ptr, uint64_t *memusage,
		uint64_t *old_memlimit, uint64_t new_memlimit)
{
	lzma_file_info_coder *coder = coder_ptr;

	// The memory usage calculation comes from three things:
	//
	// (1) The Indexes that have already been decoded and processed into
	//     coder->combined_index.
	//
	// (2) The latest Index in coder->this_index that has been decoded but
	//     not yet put into coder->combined_index.
	//
	// (3) The latest Index that we have started decoding but haven't
	//     finished and thus isn't available in coder->this_index yet.
	//     Memory usage and limit information needs to be communicated
	//     from/to coder->index_decoder.
	//
	// Care has to be taken to not do both (2) and (3) when calculating
	// the memory usage.
	uint64_t combined_index_memusage = 0;
	uint64_t this_index_memusage = 0;

	// (1) If we have already successfully decoded one or more Indexes,
	// get their memory usage.
	if (coder->combined_index != NULL)
		combined_index_memusage = lzma_index_memused(
				coder->combined_index);

	// Choose between (2), (3), or neither.
	if (coder->this_index != NULL) {
		// (2) The latest Index is available. Use its memory usage.
		this_index_memusage = lzma_index_memused(coder->this_index);

	} else if (coder->sequence == SEQ_INDEX_DECODE) {
		// (3) The Index decoder is activate and hasn't yet stored
		// the new index in coder->this_index. Get the memory usage
		// information from the Index decoder.
		//
		// NOTE: If the Index decoder doesn't yet know how much memory
		// it will eventually need, it will return a tiny value here.
		uint64_t dummy;
		if (coder->index_decoder.memconfig(coder->index_decoder.coder,
					&this_index_memusage, &dummy, 0)
				!= LZMA_OK) {
			assert(0);
			return LZMA_PROG_ERROR;
		}
	}

	// Now we know the total memory usage/requirement. If we had neither
	// old Indexes nor a new Index, this will be zero which isn't
	// acceptable as lzma_memusage() has to return non-zero on success
	// and even with an empty .xz file we will end up with a lzma_index
	// that takes some memory.
	*memusage = combined_index_memusage + this_index_memusage;
	if (*memusage == 0)
		*memusage = lzma_index_memusage(1, 0);

	*old_memlimit = coder->memlimit;

	// If requested, set a new memory usage limit.
	if (new_memlimit != 0) {
		if (new_memlimit < *memusage)
			return LZMA_MEMLIMIT_ERROR;

		// In the condition (3) we need to tell the Index decoder
		// its new memory usage limit.
		if (coder->this_index == NULL
				&& coder->sequence == SEQ_INDEX_DECODE) {
			const uint64_t idec_new_memlimit = new_memlimit
					- combined_index_memusage;

			assert(this_index_memusage > 0);
			assert(idec_new_memlimit > 0);

			uint64_t dummy1;
			uint64_t dummy2;

			if (coder->index_decoder.memconfig(
					coder->index_decoder.coder,
					&dummy1, &dummy2, idec_new_memlimit)
					!= LZMA_OK) {
				assert(0);
				return LZMA_PROG_ERROR;
			}
		}

		coder->memlimit = new_memlimit;
	}

	return LZMA_OK;
}


static void
file_info_decoder_end(void *coder_ptr, const lzma_allocator *allocator)
{
	lzma_file_info_coder *coder = coder_ptr;

	lzma_next_end(&coder->index_decoder, allocator);
	lzma_index_end(coder->this_index, allocator);
	lzma_index_end(coder->combined_index, allocator);

	lzma_free(coder, allocator);
	return;
}


static lzma_ret
lzma_file_info_decoder_init(lzma_next_coder *next,
		const lzma_allocator *allocator, uint64_t *seek_pos,
		lzma_index **dest_index,
		uint64_t memlimit, uint64_t file_size)
{
	lzma_next_coder_init(&lzma_file_info_decoder_init, next, allocator);

	if (dest_index == NULL)
		return LZMA_PROG_ERROR;

	lzma_file_info_coder *coder = next->coder;
	if (coder == NULL) {
		coder = lzma_alloc(sizeof(lzma_file_info_coder), allocator);
		if (coder == NULL)
			return LZMA_MEM_ERROR;

		next->coder = coder;
		next->code = &file_info_decode;
		next->end = &file_info_decoder_end;
		next->memconfig = &file_info_decoder_memconfig;

		coder->index_decoder = LZMA_NEXT_CODER_INIT;
		coder->this_index = NULL;
		coder->combined_index = NULL;
	}

	coder->sequence = SEQ_MAGIC_BYTES;
	coder->file_cur_pos = 0;
	coder->file_target_pos = 0;
	coder->file_size = file_size;

	lzma_index_end(coder->this_index, allocator);
	coder->this_index = NULL;

	lzma_index_end(coder->combined_index, allocator);
	coder->combined_index = NULL;

	coder->stream_padding = 0;

	coder->dest_index = dest_index;
	coder->external_seek_pos = seek_pos;

	// If memlimit is 0, make it 1 to ensure that lzma_memlimit_get()
	// won't return 0 (which would indicate an error).
	coder->memlimit = my_max(1, memlimit);

	// Prepare these for reading the first Stream Header into coder->temp.
	coder->temp_pos = 0;
	coder->temp_size = LZMA_STREAM_HEADER_SIZE;

	return LZMA_OK;
}


extern LZMA_API(lzma_ret)
lzma_file_info_decoder(lzma_stream *strm, lzma_index **dest_index,
		uint64_t memlimit, uint64_t file_size)
{
	lzma_next_strm_init(lzma_file_info_decoder_init, strm, &strm->seek_pos,
			dest_index, memlimit, file_size);

	// We allow LZMA_FINISH in addition to LZMA_RUN for convenience.
	// lzma_code() is able to handle the LZMA_FINISH + LZMA_SEEK_NEEDED
	// combination in a sane way. Applications still need to be careful
	// if they use LZMA_FINISH so that they remember to reset it back
	// to LZMA_RUN after seeking if needed.
	strm->internal->supported_actions[LZMA_RUN] = true;
	strm->internal->supported_actions[LZMA_FINISH] = true;

	return LZMA_OK;
}
