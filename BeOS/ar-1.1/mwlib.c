/*
** mwlib.c - POSIX 1003.2 "ar" command
**
** This isn't a pure POSIX 1003.2 ar; it only manipulates Metrowerks
** Library files, not general-purpose POSIX 1003.2 format archives.
**
** Dec. 14, 1997 Chris Herborth (chrish@kagi.com)
**
** This code is donated to the PUBLIC DOMAIN.  You can use, abuse, modify,
** redistribute, steal, or otherwise manipulate this code.  No restrictions
** at all.  If you laugh at this code, you can't use it.
**
** This "ar" was implemented using IEEE Std 1003.2-1992 as the basis for
** the interface, and Metrowerk's published docs detailing their library
** format.  Look inside for clues about how reality differs from MW's
** documentation on BeOS...
*/

#include <support/Errors.h>
#include <support/byteorder.h>
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
#include <fcntl.h>			/* is open() really here?!? sheesh... */
#include <storage/Mime.h>
#include <sys/stat.h>

#include "mwlib.h"
#include "copy_attrs.h"

static const char *rcs_version_id = "$Id$";

/* ----------------------------------------------------------------------
** Local prototypes
*/
static status_t load_MWLibFile( FILE *file, MWLibFile *libfile );
static size_t fwrite_big32( uint32 x, FILE *fp );
static size_t fwrite_big32_seek( uint32 x, long offset, FILE *fp );
static status_t add_object_sizes( MWObject *obj, uint32 *code, uint32 *data );

/* ----------------------------------------------------------------------
** Load a Metrowerks library file into the given MWLib object.
**
** Returns:
**    B_OK - all is well
**    B_FILE_NOT_FOUND - can't open the given file
**    B_IO_ERROR - can't read from the given file
**    B_BAD_VALUE - invalid magic word in the file
**    B_MISMATCHED_VALUES - invalid processor value (ie, not a PowerPC lib),
**                          or the magicflags member is not 0, or the file
**                          version number isn't 1.
**    B_NO_MEMORY - unable to allocate memory while loading the lib
*/
status_t load_MW_lib( MWLib *lib, const char *filename )
{
	FILE *fp = NULL;
	size_t recs = 0;
	status_t retval = B_OK;
	uint32 tmp32 = 0;
	uint32 idx = 0;
	char obj_name[PATH_MAX];

	ASSERT( lib != NULL );
	ASSERT( filename != NULL );

	fp = fopen( filename, "r" );
	if( !fp ) {
		return B_FILE_NOT_FOUND;
	}

	/* Read and check the magic number.
	*/
	recs = fread( &tmp32, sizeof( uint32 ), 1, fp );
	lib->header.magicword = B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		retval = B_IO_ERROR;
		goto close_return;
	} else if( lib->header.magicword != MWLIB_MAGIC_WORD ) {
		retval = B_BAD_VALUE;
		goto close_return;
	}

	/* Read and check the processor.
	*/
	recs = fread( &tmp32, sizeof( uint32 ), 1, fp );
	lib->header.magicproc = B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		retval = B_IO_ERROR;
		goto close_return;
	} else if( lib->header.magicproc != MWLIB_MAGIC_PROC ) {
		retval = B_MISMATCHED_VALUES;
		goto close_return;
	}

	/* Read and check the flags.
	*/
	recs = fread( &tmp32, sizeof( uint32 ), 1, fp );
	lib->header.magicflags = B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		retval = B_IO_ERROR;
		goto close_return;
	} else if( lib->header.magicflags != 0 ) {
		retval = B_MISMATCHED_VALUES;
		goto close_return;
	}

	/* Read and check the file version.
	*/
	recs = fread( &tmp32, sizeof( uint32 ), 1, fp );
	lib->header.version = B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		retval = B_IO_ERROR;
		goto close_return;
	} else if( lib->header.version != 1 ) {
		retval = B_MISMATCHED_VALUES;
		goto close_return;
	}

	/* Read the code size, data size, and number of objects.
	*/
	recs = fread( &tmp32, sizeof( uint32 ), 1, fp );
	lib->header.code_size = B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		retval = B_IO_ERROR;
		goto close_return;
	}

	recs = fread( &tmp32, sizeof( uint32 ), 1, fp );
	lib->header.data_size = B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		retval = B_IO_ERROR;
		goto close_return;
	}

	recs = fread( &tmp32, sizeof( uint32 ), 1, fp );
	lib->header.num_objects = B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		retval = B_IO_ERROR;
		goto close_return;
	}

	/* Load the MWLibFile objects from the lib.
	*/
	lib->files = (MWLibFile *)malloc( lib->header.num_objects * sizeof( MWLibFile ) );
	if( lib->files == NULL ) {
		retval = B_NO_MEMORY;
		goto close_return;
	}

	for( idx = 0; idx < lib->header.num_objects; idx++ ) {
		retval = load_MWLibFile( fp, &(lib->files[idx]) );
		if( retval != B_OK ) {
			goto close_return;
		}
	}

	/* Load the file names and object data.
	**
	** The file name actually appears twice in the library file; according
	** to the docs, one is the file name, the other is the "full path name".
	** In all of the libraries on my system, they're the same.
	*/
	lib->names = (char **)malloc( lib->header.num_objects * sizeof( char * ) );
	lib->data = (char **)malloc( lib->header.num_objects * sizeof( char * ) );
	if( lib->names == NULL || lib->data == NULL ) {
		retval = B_NO_MEMORY;
		goto close_return;
	}

	for( idx = 0; idx < lib->header.num_objects; idx ++ ) {
		/* Load the name and copy it into the right spot.
		*/
		retval = fseek( fp, lib->files[idx].off_filename, SEEK_SET );
		if( retval ) {
			retval = B_IO_ERROR;
			goto close_return;
		}

		if( fgets( obj_name, PATH_MAX, fp ) == NULL ) {
			retval = B_IO_ERROR;
			goto close_return;
		}

		lib->names[idx] = strdup( obj_name );
		if( lib->names[idx] == NULL ) {
			retval = B_NO_MEMORY;
			goto close_return;
		}

		/* Load the object data.
		*/
		lib->data[idx] = (char *)malloc( lib->files[idx].object_size );
		if( lib->data[idx] == NULL ) {
			retval = B_NO_MEMORY;
			goto close_return;
		}

		retval = fseek( fp, lib->files[idx].off_object, SEEK_SET );
		if( retval ) {
			retval = B_IO_ERROR;
			goto close_return;
		}

		recs = fread( lib->data[idx], lib->files[idx].object_size, 1, fp );
		if( recs != 1 ) {
			retval = B_IO_ERROR;
			goto close_return;
		}
	}

close_return:
	fclose( fp );
	return retval;
}

/* ----------------------------------------------------------------------
** Load the file header from a Metrowerks library file.
**
** Returns:
**    B_OK - All is well
**    B_IO_ERROR - Error reading the file
*/
static status_t load_MWLibFile( FILE *file, MWLibFile *libfile )
{
	size_t recs = 0;
	uint32 tmp32 = 0;

	ASSERT( file != NULL );
	ASSERT( libfile != NULL );

	/* Load the modification time.
	*/
	recs = fread( &tmp32, sizeof( uint32 ), 1, file );
	libfile->m_time = (time_t)B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		return B_IO_ERROR;
	}

	/* Load the various offsets.
	*/
	recs = fread( &tmp32, sizeof( uint32 ), 1, file );
	libfile->off_filename = B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		return B_IO_ERROR;
	}

	recs = fread( &tmp32, sizeof( uint32 ), 1, file );
	libfile->off_fullpath = B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		return B_IO_ERROR;
	}

	recs = fread( &tmp32, sizeof( uint32 ), 1, file );
	libfile->off_object = B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		return B_IO_ERROR;
	}

	/* Load the object size.
	*/
	recs = fread( &tmp32, sizeof( uint32 ), 1, file );
	libfile->object_size = B_BENDIAN_TO_HOST_INT32( tmp32 );
	if( recs != 1 ) {
		return B_IO_ERROR;
	}

	return B_OK;
}

/* ----------------------------------------------------------------------
** Write a Metrowerks library file.
**
** Returns:
**    B_OK - all is well; doesn't necessarily mean the file was written
**           properly, just that you didn't lose any data
**    B_NO_MEMORY - unable to allocate offset buffers
**    B_ERROR - problem with backup file (can't rename existing file)
**
** Note:
**    If you use this in a long-lived program, it leaks memory; the MWLib
**    contents are never free()'d.
**
** Two-pass technique:
**
** pass 1 - write file header (need CODE SIZE and DATA SIZE)
** 		 write object headers (need offsets for FILENAME, FULL PATH, DATA)
** 		 write filenames (store offsets for object headers)
** 		 write padding (file size % 4 bytes)
** 		 write file data (store offset for object header; add to code/data
** 		                  size for file header; pad data size % 4 bytes)
**
** pass 2 - fill in file header CODE SIZE and DATA SIZE
** 		 fill in object headers FILENAME, FULL PATH, DATA
**
** You could avoid this by building up the file headers in memory, but this is
** easier (?) and I'm not that concerned with speed...
**
*/

typedef struct file_header_offsets {
	long codesize_offset;
	long datasize_offset;
} file_header_offsets;

typedef struct obj_header_offsets {
	long filename_offset;
	uint32 filename_offset_value;

	long fullpath_offset;
	uint32 fullpath_offset_value;

	long data_offset;
	uint32 data_offset_value;
} obj_header_offsets;

static size_t fwrite_big32( uint32 x, FILE *fp )
{
	uint32 tmp32 = B_HOST_TO_BENDIAN_INT32( x );

	ASSERT( fp != NULL );

	return fwrite( &tmp32, sizeof(tmp32), 1, fp );
}

static size_t fwrite_big32_seek( uint32 x, long offset, FILE *fp )
{
	uint32 tmp32 = B_HOST_TO_BENDIAN_INT32( x );

	ASSERT( fp != NULL );

	if( fseek( fp, offset, SEEK_SET ) ) {
		return 0;
	}

	return fwrite( &tmp32, sizeof(tmp32), 1, fp );
}

static status_t add_object_sizes( MWObject *obj, uint32 *code, uint32 *data )
{
	ASSERT( obj != NULL );
	ASSERT( code != NULL );
	ASSERT( data != NULL );

	if( B_BENDIAN_TO_HOST_INT32( obj->magic_word ) != 'MWOB' || 
		B_BENDIAN_TO_HOST_INT32( obj->arch ) != 'PPC ' ) {
		return B_ERROR;
	}

	*code += B_BENDIAN_TO_HOST_INT32( obj->code_size );
	*data += B_BENDIAN_TO_HOST_INT32( obj->data_size );

	return B_OK;
}

void setfiletype( const char *file, const char *type )
{
    int fd;
    attr_info fa;
    ssize_t wrote_bytes;

    fd = open( file, O_RDWR );
    if( fd < 0 ) {
        fprintf( stderr, "ar: can't open %s to write file type, %s",
				 file, strerror( errno ) );
        return;
    }

    fa.type = B_MIME_STRING_TYPE;
    fa.size = (off_t)(strlen( type ) + 1);

    wrote_bytes = fs_write_attr( fd, "BEOS:TYPE", fa.type, 0,
                                 type, fa.size );
    if( wrote_bytes != (ssize_t)fa.size ) {
        fprintf( stderr, "ar: couldn't write complete file type, %s",
				 strerror( errno ) );
    }

    close( fd );
}

status_t write_MW_lib( MWLib *lib, const char *filename )
{
	char *padding = "\0\0\0\0";
	long pad_amount;

	file_header_offsets head_offs;
	obj_header_offsets *obj_offs;
	uint32 codesize = 0;
	uint32 datasize = 0;
	uint32 num_objects = 0;

	FILE *fp;
	char *tmp_filename = NULL;
	uint32 idx = 0;
	size_t recs;
	status_t retval;

	mode_t mode_bits = 0666;

	ASSERT( lib != NULL );
	ASSERT( filename != NULL );

#if 0
/* Apparently, I don't understand umask()... */

	/* Find the default file creation mask.  The semantics of umask() suck.
	*/
	mode_bits = umask( (mode_t)0 );
	(void)umask( mode_bits );
#endif

	/* Easier access to the number of objects.
	*/
	num_objects = lib->header.num_objects;

	/* Allocate some storage for keeping track of the things we need to
	** write out in the second pass.
	*/
	head_offs.codesize_offset = 0;
	head_offs.datasize_offset = 0;

	obj_offs = (obj_header_offsets *)malloc( sizeof(obj_header_offsets) *
											 num_objects );
	if( obj_offs == NULL ) {
		fprintf( stderr, "ar: can't allocate memory for writing file\n" );

		return B_NO_MEMORY;
	}
	memset( obj_offs, 0, sizeof(obj_header_offsets) * num_objects );

	/* If the file exists, move it somewhere so we can recover if there's
	** an error.
	*/
	retval = access( filename, F_OK );
	if( retval == 0 ) {
		struct stat s;

		/* Preserve the mode bits.
		*/
		retval = stat( filename, &s );
		if( retval != 0 ) {
			fprintf( stderr, "ar: can't stat %s, %s\n", filename,
					 strerror( errno ) );
		} else {
			mode_bits = s.st_mode;
		}

		tmp_filename = (char *)malloc( strlen( filename ) + 2 );
		if( tmp_filename == NULL ) {
			fprintf( stderr, 
					 "ar: can't allocate memory for temporary filename\n" );
		} else {
			sprintf( tmp_filename, "%s~", filename );

			retval = access( tmp_filename, F_OK );
			if( retval == 0 ) {
				retval = unlink( tmp_filename );
				if( retval != 0 ) {
					fprintf( stderr, "ar: can't unlink %s, %s\n", tmp_filename,
							 strerror( errno ) );

					free( tmp_filename );
					tmp_filename = NULL;
				}
			}

			if( tmp_filename ) {
				retval = rename( filename, tmp_filename );
				if( retval != 0 ) {
					fprintf( stderr, "ar: can't move %s to backup, %s\n",
							 filename, strerror( errno ) );

					return B_ERROR;
				}
			}
		}
	}

	/* Attempt to open the archive file.
	*/
	fp = fopen( filename, "w" );
	if( fp == NULL ) {
		fprintf( stderr, "ar: can't open %s for write, %s\n", filename,
				 strerror( errno ) );
		goto error_return;
	}

	/* ----------------------------------------------------------------------
	** Write the file.  Pass 1.
	*/
	recs = fwrite_big32( lib->header.magicword, fp );
	recs += fwrite_big32( lib->header.magicproc, fp );
	recs += fwrite_big32( lib->header.magicflags, fp );
	recs += fwrite_big32( lib->header.version, fp );
	if( recs != 4 ) {
		fprintf( stderr, "ar: error writing header for %s, %s\n", filename,
				 strerror( errno ) );
		goto error_return;
	}

	/* Keep track of the code/data size offsets.
	*/
	head_offs.codesize_offset = ftell( fp );
	recs = fwrite_big32( codesize, fp );
	head_offs.datasize_offset = ftell( fp );
	recs += fwrite_big32( datasize, fp );
	if( recs != 2 ) {
		fprintf( stderr, "ar: error writing header for %s, %s\n", filename,
				 strerror( errno ) );
		goto error_return;
	}

	recs = fwrite_big32( num_objects, fp );
	if( recs != 1 ) {
		fprintf( stderr, "ar: error writing header for %s, %s\n", filename,
				 strerror( errno ) );
		goto error_return;
	}

	/* Write the object headers.
	*/
	for( idx = 0; idx < num_objects; idx++ ) {
		recs = fwrite_big32( lib->files[idx].m_time, fp );

		/* Keep track of the offsets, and write 0 for now.
		*/
		obj_offs[idx].filename_offset = ftell( fp );
		recs += fwrite_big32( 0, fp );
		obj_offs[idx].fullpath_offset = ftell( fp );
		recs += fwrite_big32( 0, fp );
		obj_offs[idx].data_offset = ftell( fp );
		recs += fwrite_big32( 0, fp );

		recs += fwrite_big32( lib->files[idx].object_size, fp );

		if( recs != 5 ) {
			fprintf( stderr, "ar: error writing object header for %s, %s\n", 
					 filename, strerror( errno ) );
			goto error_return;
		}
	}

	/* Write the file names.
	*/
	for( idx = 0; idx < num_objects; idx++ ) {
		/* Need to make sure that all the file names we write into the
		** library DO NOT HAVE PATHS IN THEM, the world might end or something.
		*/
		size_t name_len = 0;
		char *name_ptr = strrchr( lib->names[idx], '/' );

		if( name_ptr == NULL ) {
			name_ptr = lib->names[idx];
		} else {
			name_ptr++;
		}

		name_len = strlen( name_ptr ) + 1;

		obj_offs[idx].filename_offset_value = ftell( fp );
		recs = fwrite( name_ptr, name_len, 1, fp );
		obj_offs[idx].fullpath_offset_value = ftell( fp );
		recs += fwrite( name_ptr, name_len, 1, fp );

		if( recs != 2 ) {
			fprintf( stderr, "ar: error writing object name for %s, %s\n", 
					 filename, strerror( errno ) );
			goto error_return;
		}
	}

	/* Pad the file if necessary.
	*/
	pad_amount = ftell( fp ) % 4;
	if( pad_amount > 0 ) {
		recs = fwrite( padding, pad_amount, 1, fp );

		if( recs != 1 ) {
			fprintf( stderr, "ar: error padding file %s, %s\n", filename,
					 strerror( errno ) );
			goto error_return;
		}
	}

	/* Write the object file data.
	*/
	for( idx = 0; idx < num_objects; idx++ ) {
		obj_offs[idx].data_offset_value = ftell( fp );

		recs = fwrite( lib->data[idx], lib->files[idx].object_size, 1, fp );
		if( recs != 1 ) {
			fprintf( stderr, "ar: writing object data for %s, %s\n", filename,
					 strerror( errno ) );
			goto error_return;
		}

		/* Add up the code/data size.
		*/
		retval = add_object_sizes( (MWObject *)lib->data[idx], 
								   &codesize, &datasize );
		if( retval != B_OK ) {
			fprintf( stderr, "ar - warning: %s is not an object file!\n",
					 lib->names[idx] );
			goto error_return;
		}

		pad_amount = ftell( fp ) % 4;
		if( pad_amount > 0 ) {
			recs = fwrite( padding, pad_amount, 1, fp );

			if( recs != 1 ) {
				fprintf( stderr, "ar: error padding file %s, %s\n", filename,
						 strerror( errno ) );
				goto error_return;
			}
		}
	}

	/* ----------------------------------------------------------------------
	** Write the offsets into the file.  Pass 2.
	*/

	/* Write the code/data sizes.
	*/
	recs = fwrite_big32_seek( codesize, head_offs.codesize_offset, fp );
	recs += fwrite_big32_seek( datasize, head_offs.datasize_offset, fp );
	if( recs != 2 ) {
		fprintf( stderr, "ar - error writing code and data sizes, %s\n",
				 strerror( errno ) );
		goto error_return;
	}

	/* Write the offsets for each file.
	*/
	for( idx = 0; idx < num_objects; idx++ ) {
		recs = fwrite_big32_seek( obj_offs[idx].filename_offset_value,
								  obj_offs[idx].filename_offset, fp );
		recs += fwrite_big32_seek( obj_offs[idx].fullpath_offset_value,
								   obj_offs[idx].fullpath_offset, fp );
		recs += fwrite_big32_seek( obj_offs[idx].data_offset_value,
								   obj_offs[idx].data_offset, fp );

		if( recs != 3 ) {
			fprintf( stderr, "ar - error writing object offsets, %s\n",
					 strerror( errno ) );
			goto error_return;
		}
	}

	/* If all is OK, close the file and get out of here after nuking the
	** temp file (if any), preserving the original file mode, and preserving
	** the file attributes.
	*/

	fclose( fp );

	/* Preserve the original file mode bits.
	*/
	retval = chmod( filename, mode_bits );
	if( retval != 0 ) {
		fprintf( stderr, "ar: unable to change file mode for %s, %s\n",
				 filename, strerror( errno ) );
	}

	/* Nuke the temp file (if any), after copying over any file attributes.
	*/
	if( tmp_filename != NULL ) {
		retval = copy_attrs( filename, tmp_filename );

		retval = unlink( tmp_filename );
		if( retval != 0 ) {
			fprintf( stderr, "ar - error unlinking %s, %s\n", tmp_filename,
					 strerror( errno ) );
		}
		free( tmp_filename );
	} else {
		/* If there isn't a temp file, we should still give this new
		** file a file type attribute.
		*/
		setfiletype( filename, "application/x-mw-library" );
	}

	return B_OK;

error_return:
	/* Restore the original file if we had any problems.
	*/
	fclose( fp );

	if( tmp_filename != NULL ) {
		retval = unlink( filename );
		retval = rename( tmp_filename, filename );
		if( retval != 0 ) {
			fprintf( stderr, "ar: can't restore %s to %s, %s\n",
					 tmp_filename, filename, strerror( errno ) );
		}
	}

	return B_ERROR;
}
