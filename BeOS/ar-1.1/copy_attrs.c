/*
** copy_attrs.h - copy BeFS attributes from one file to another
**
** Jan. 11, 1998 Chris Herborth (chrish@qnx.com)
**
** This code is donated to the PUBLIC DOMAIN.  You can use, abuse, modify,
** redistribute, steal, or otherwise manipulate this code.  No restrictions
** at all.  If you laugh at this code, you can't use it.
*/

#include <support/Errors.h>
#ifndef NO_DEBUG
#include <assert.h>
#define ASSERT(cond) assert(cond)
#else
#define ASSERT(cond) ((void)0)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <kernel/fs_attr.h>
#include <fcntl.h>

#include "copy_attrs.h"

static const char *rcs_version_id = "$Id$";

/* ----------------------------------------------------------------------
** Copy file attributes from src_file to dst_file.
*/

status_t copy_attrs( const char *dst_file, const char *src_file )
{
	int dst_fd, src_fd;
	status_t retval = B_OK;
	DIR *fa_dir = NULL;
	struct dirent *fa_ent = NULL;
	char *buff = NULL;
	struct attr_info fa_info;
	off_t read_bytes, wrote_bytes;

	ASSERT( dst_file != NULL );
	ASSERT( src_file != NULL );

	/* Attempt to open the files.
	*/
	src_fd = open( src_file, O_RDONLY );
	if( src_fd < 0 ) {
		return B_FILE_NOT_FOUND;
	}

	dst_fd = open( dst_file, O_WRONLY );
	if( dst_fd < 0 ) {
		close( src_fd );
		return B_FILE_NOT_FOUND;
	}

	/* Read the attributes, and write them to the destination file.
	*/
	fa_dir = fs_fopen_attr_dir( src_fd );
	if( fa_dir == NULL ) {
		retval = B_IO_ERROR;
		goto close_return;
	}

	fa_ent = fs_read_attr_dir( fa_dir );
	while( fa_ent != NULL ) {
		retval = fs_stat_attr( src_fd, fa_ent->d_name, &fa_info );
		if( retval != B_OK ) {
			/* TODO: Print warning message?
			*/
			goto read_next_attr;
		}

		if( fa_info.size > (off_t)UINT_MAX ) {
			/* TODO: That's too big.  Print a warning message?  You could
			**       copy it in chunks...
			*/
			goto read_next_attr;
		}

		if( fa_info.size > (off_t)0 ) {
			buff = malloc( (size_t)fa_info.size );
			if( buff == NULL ) {
				/* TODO: Can't allocate memory for this attribute.  Warning?
				*/
				goto read_next_attr;
			}
			
			read_bytes = fs_read_attr( src_fd, fa_ent->d_name, fa_info.type,
									   0, buff, fa_info.size );
			if( read_bytes != fa_info.size ) {
				/* TODO: Couldn't read entire attribute.  Warning?
				*/
				goto free_attr_buff;
			}
			
			wrote_bytes = fs_write_attr( dst_fd, fa_ent->d_name, fa_info.type,
										 0, buff, fa_info.size );
			if( wrote_bytes != fa_info.size ) {
				/* TODO: Couldn't write entire attribute.  Warning?
				*/
				;
			}

		free_attr_buff:
			free( buff );

			retval = B_OK;
		}

		/* Read the next entry.
		*/
	read_next_attr:
		fa_ent = fs_read_attr_dir( fa_dir );
	}

close_return:
	close( dst_fd );
	close( src_fd );

	return retval;
}

