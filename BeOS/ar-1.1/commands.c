/*
** commands.c - POSIX 1003.2 "ar" command
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
#include <utime.h>
#include <errno.h>
#include <sys/stat.h>

#include "mwlib.h"
#include "commands.h"

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

static const char *rcs_version_id = "$Id$";

/* ----------------------------------------------------------------------
** Local functions
**
** do_match() - find the index of the file, if it's in the archive; return
**              TRUE if found, else FALSE
**/
static int do_match( MWLib *lib, const char *file, int *idx );

static int do_match( MWLib *lib, const char *file, int *idx )
{
	int which = 0;
	char *name_ptr;

	ASSERT( lib != NULL );
	ASSERT( file != NULL );
	ASSERT( idx != NULL );

	/* Skip over the path, if any, so we can compare just the file name.
	*/
	name_ptr = strrchr( file, '/' );
	if( name_ptr == NULL ) {
		name_ptr = (char *)file;
	} else {
		name_ptr++;
	}

	for( which = 0; which < lib->header.num_objects; which++ ) {
		if( !strcmp( name_ptr, lib->names[which] ) ) {
			*idx = which;
			return TRUE;
		}
	}

	return FALSE;
}

/* ----------------------------------------------------------------------
** Delete an archive member.
**
** This isn't really optimal; you could make a more efficient version
** using a linked list instead of arrays for the data.  This was easier
** to write, and speed shouldn't be that big a deal here... you're not
** likely to be dealing with thousands of files.
*/
static status_t delete_lib_entry( MWLib *lib, int idx, int verbose );

status_t do_delete( const char *archive_name, char **files, int verbose )
{
	status_t retval = B_OK;
	MWLib lib;
	int idx = 0;
	int which = 0;

	ASSERT( archive_name != NULL );

	if( files == NULL ) {
		fprintf( stderr, "ar: %s, nothing to do\n", archive_name );
		return B_ERROR;
	}

	retval = load_MW_lib( &lib, archive_name );
	if( retval != B_OK ) {
		switch( retval ) {
		case B_FILE_NOT_FOUND:
			fprintf( stderr, "ar: %s, file not found\n", archive_name );
			return retval;
			break;
			
		default:
			return retval;
			break;
		}
	}

	/* Delete the specified files.
	*/
	for( idx = 0; files[idx] != NULL; idx++ ) {
		if( do_match( &lib, files[idx], &which ) ) {
			retval = delete_lib_entry( &lib, which, verbose );
		}
		which = 0;
	}

	/* Write the new file.
	*/
	retval = write_MW_lib( &lib, archive_name );

	return retval;
}

static status_t delete_lib_entry( MWLib *lib, int which, int verbose )
{
	uint32 new_num;
	MWLibFile *new_files = NULL;
	char **new_names = NULL;
	char **new_data = NULL;
	int ctr = 0;
	int idx = 0;

	ASSERT( lib != NULL );
	ASSERT( which <= lib->header.num_objects );

	new_num = lib->header.num_objects - 1;

	new_files = (MWLibFile *)malloc( new_num * ( sizeof( MWLibFile ) ) );
	new_names = (char **)malloc( new_num * ( sizeof( char * ) ) );
	new_data = (char **)malloc( new_num * ( sizeof( char * ) ) );
	if( new_files == NULL || new_names == NULL || new_data == NULL ) {
		return B_NO_MEMORY;
	}

	/* Copy the contents of the old lib to the new lib, skipping the one
	** we want to delete.
	*/
	for( ctr = 0; ctr < lib->header.num_objects; ctr++ ) {
		if( ctr != which ) {
			memcpy( &(new_files[idx]), &(lib->files[ctr]), 
					sizeof( MWLibFile ) );
			new_names[idx] = lib->names[ctr];
			new_data[idx] = lib->data[ctr];

			idx++;
		} else { 
			/* Free up the name and data.
			*/
			if( verbose ) {
				printf( "d - %s\n", lib->names[ctr] );
			}

			free( lib->names[idx] );
			lib->names[idx] = NULL;
			free( lib->data[idx] );
			lib->data[idx] = NULL;
		}
	}

	/* Free up the old lib's data.
	*/
	free( lib->files );
	free( lib->names );
	free( lib->data );

	lib->files = new_files;
	lib->names = new_names;
	lib->data = new_data;

	lib->header.num_objects = new_num;

	return B_OK;
}

/* ----------------------------------------------------------------------
** Print an archive member to stdout.
*/
static status_t print_lib_entry( MWLib *lib, int idx, int verbose );

status_t do_print( const char *archive_name, char **files, int verbose )
{
	status_t retval = B_OK;
	MWLib lib;
	int idx = 0;

	ASSERT( archive_name != NULL );

	retval = load_MW_lib( &lib, archive_name );
	if( retval != B_OK ) {
		switch( retval ) {
		case B_FILE_NOT_FOUND:
			fprintf( stderr, "ar: %s, file not found\n", archive_name );
			return retval;
			break;

		default:
			return retval;
			break;
		}
	}

	if( files == NULL ) {
		/* Then we print the entire archive.
		*/
		for( idx = 0; idx < lib.header.num_objects; idx++ ) {
			retval = print_lib_entry( &lib, idx, verbose );
		}
	} else {
		/* Then we print the specified files.
		*/
		int which = 0;

		for( idx = 0; files[idx] != NULL; idx++ ) {
			if( do_match( &lib, files[idx], &which ) ) {
				retval = print_lib_entry( &lib, which, verbose );
			}
			which = 0;
		}
	}

	return retval;
}

static status_t print_lib_entry( MWLib *lib, int idx, int verbose )
{
	int recs;

	ASSERT( lib != NULL );

	if( verbose ) {
		printf( "\n<%s>\n\n", lib->names[idx] );
	}

	recs = fwrite( lib->data[idx], lib->files[idx].object_size, 1, stdout );
	fflush( stdout );
	if( recs != 1 ) {
		fprintf( stderr, "error printing %s, %s\n", lib->names[idx],
				 strerror( errno ) );
		return B_OK;
	}

	return B_OK;
}

/* ----------------------------------------------------------------------
** Add/replace/update files in an archive.
*/
static status_t add_lib_entry( MWLib *lib, const char *filename, int verbose );
static status_t replace_lib_entry( MWLib *lib, const char *filename, 
								   int idx, int verbose );
static status_t load_lib_file( const char *filename,
							   char **data, MWLibFile *info );

static status_t load_lib_file( const char *filename,
							   char **data, MWLibFile *info )
{
	status_t retval = B_OK;
	struct stat s;
	FILE *fp;
	uint32 recs;

	ASSERT( filename != NULL );
	ASSERT( info != NULL );

	/* Initialize the info area.
	*/
	info->m_time = (time_t)0;	/* Only this... */
	info->off_filename = 0;
	info->off_fullpath = 0;
	info->off_object = 0;
	info->object_size = 0;		/* ... and this will actually be updated. */

	/* stat() the file to get the info we need.
	*/
	retval = stat( filename, &s );
	if( retval != 0 ) {
		fprintf( stderr, "ar: can't stat %s, %s\n", filename,
				 strerror( errno ) );
		return B_FILE_NOT_FOUND;
	}

	/* Possible errors here; if you have an object that's larger
	** than a size_t can hold (malloc() can only allocate a size_t size,
	** not a full off_t)...
	*/
	if( s.st_size > (off_t)ULONG_MAX ) {
		fprintf( stderr, "ar: %s is too large!\n", filename );
		return B_NO_MEMORY;
	}

	/* Allocate enough memory to hold the file data.
	*/
	*data = (char *)malloc( (size_t)s.st_size );
	if( *data == NULL ) {
		fprintf( stderr, "ar: can't allocate memory for %s\n", filename );
		return B_NO_MEMORY;
	}

	/* Read the file's data.
	*/
	fp = fopen( filename, "r" );
	if( fp == NULL ) {
		fprintf( stderr, "ar: can't open %s, %s\n", filename,
				 strerror( errno ) );
		retval = B_FILE_NOT_FOUND;
		goto free_data_return;
	}

	recs = fread( *data, (size_t)s.st_size, 1, fp );
	if( recs != 1 ) {
		fprintf( stderr, "ar: can't read %s, %s\n", filename,
				 strerror( errno ) );
		retval = B_IO_ERROR;
		goto close_fp_return;
	}

	fclose( fp );

	/* Now that all the stuff that can fail has succeeded, fill in the info
	** we need.
	*/
	info->m_time = s.st_mtime;
	info->object_size = (uint32)s.st_size;

	return B_OK;

	/* How we should return if an error occurred.
	*/
close_fp_return:
	fclose( fp );

free_data_return:
	free( *data );
	*data = NULL;

	return retval;
}

status_t do_replace( const char *archive_name, char **files, int verbose,
					 int create, int update )
{
	status_t retval = B_OK;
	MWLib lib;
	int idx = 0;
	int which = 0;

	ASSERT( archive_name != NULL );

	memset( &lib, 0, sizeof( MWLib ) );

	if( files == NULL ) {
		fprintf( stderr, "ar: %s, nothing to do\n", archive_name );
		return B_ERROR;
	}

	retval = load_MW_lib( &lib, archive_name );
	if( retval != B_OK ) {
		switch( retval ) {
		case B_FILE_NOT_FOUND:
			lib.header.magicword = 'MWOB';
			lib.header.magicproc = 'PPC ';
			lib.header.magicflags = 0;
			lib.header.version = 1;

			if( lib.files != NULL ) {
				free( lib.files );
				lib.files = NULL;
			}

			if( lib.names != NULL ) {
				free( lib.names );
				lib.names = NULL;
			}

			if( lib.data != NULL ) {
				lib.data = NULL;
			}

			if( !create ) {
				fprintf( stderr, "ar: creating %s\n", archive_name );
			}

			if( update ) {
				fprintf( stderr, "ar: nothing to do for %s\n", archive_name );
				return retval;
			}
			break;

		default:
			return retval;
			break;
		}
	}

	for( idx = 0; files[idx] != NULL; idx++ ) {
		if( do_match( &lib, files[idx], &which ) ) {
			/* Then the file exists, and we need to replace it or update it.
			*/
			if( update ) {
				/* Compare m_times
				** then replace this entry
				*/
				struct stat s;

				retval = stat( files[idx], &s );
				if( retval != 0 ) {
					fprintf( stderr, "ar: can't stat %s, %s\n", files[idx],
							 strerror( errno ) );
				}

				if( s.st_mtime >= lib.files[which].m_time ) {
					retval = replace_lib_entry( &lib, files[idx], which, 
												verbose );
				} else {
					fprintf( stderr, "ar: a newer %s is already in %s\n",
							 files[idx], archive_name );
				}
			} else {
				/* replace this entry
				*/
				retval = replace_lib_entry( &lib, files[idx], which, verbose );
			}
		} else {
			/* add this entry
			*/
			retval = add_lib_entry( &lib, files[idx], verbose );
		}
	}

	/* Write the new file.
	*/
	retval = write_MW_lib( &lib, archive_name );

	return retval;
}

static status_t add_lib_entry( MWLib *lib, const char *filename, int verbose )
{
	status_t retval = B_OK;
	uint32 new_num_objects;
	uint32 idx;

	ASSERT( lib != NULL );
	ASSERT( filename != NULL );

	/* Find out how many objects we'll have after we add this one.
	*/
	new_num_objects = lib->header.num_objects + 1;
	idx = lib->header.num_objects;

	/* Attempt to reallocate the MWLib's buffers.  If one of these fails,
	** we could leak a little memory, but it shouldn't be a big deal in
	** a short-lived app like this.
	*/
	lib->files = (MWLibFile *)realloc( lib->files, 
									   sizeof(MWLibFile) * new_num_objects );
	lib->names = (char **)realloc( lib->names,
								   sizeof(char *) * new_num_objects );
	lib->data = (char **)realloc( lib->data,
								  sizeof(char *) * new_num_objects );
	if( lib->files == NULL || lib->names == NULL || lib->data == NULL ) {
		fprintf( stderr, "ar: can't allocate memory to add %s\n", filename );
		return B_NO_MEMORY;
	}

	/* Load the file's data and info into the MWLib structure.
	*/
	retval = load_lib_file( filename, &(lib->data[idx]), &(lib->files[idx]) );
	if( retval != B_OK ) {
		fprintf( stderr, "ar: error adding %s, %s\n", filename,
				 strerror( errno ) );

		return retval;
	}

	/* Save a copy of the filename.  This is where we leak 
	** sizeof(MWLibFile) + 2 * sizeof(char *) bytes because we don't
	** shrink lib->files, lib->names, and lib->data.
	*/
	lib->names[idx] = strdup( filename );
	if( lib->names == NULL ) {
		fprintf( stderr, "ar: error allocating memory for filename\n" );

		return B_NO_MEMORY;
	}

	/* Now that everything's OK, we can update the MWLib header.
	*/
	lib->header.num_objects++;

	/* Give a little feedback.
	*/
	if( verbose ) {
		printf( "a - %s\n", filename );
	}

	return B_OK;
}

static status_t replace_lib_entry( MWLib *lib, const char *filename, 
								   int idx, int verbose )
{
	char *buff;
	MWLibFile info;
	char *dup_name;

	status_t retval = B_OK;

	ASSERT( lib != NULL );
	ASSERT( filename != NULL );
	ASSERT( idx <= lib->header.num_objects );

	/* Load the file's data and info into the MWLib structure.
	**
	** We'll do it safely so we don't end up writing a bogus library in
	** the event of failure.
	*/
	retval = load_lib_file( filename, &buff, &info );
	if( retval != B_OK ) {
		fprintf( stderr, "ar: error adding %s, %s\n", filename,
				 strerror( errno ) );

		return retval;
	}

	/* Attempt to allocate memory for a duplicate of the file name.
	*/
	dup_name = strdup( filename );
	if( dup_name == NULL ) {
		fprintf( stderr, "ar: unable to allocate memory for filename\n",
				 filename );

		free( buff );

		return B_NO_MEMORY;
	}

	/* All is well, so let's update the MWLib object appropriately.
	*/
	lib->files[idx].m_time = info.m_time;
	lib->files[idx].off_filename = 0;
	lib->files[idx].off_fullpath = 0;
	lib->files[idx].off_object = 0;
	lib->files[idx].object_size = info.object_size;

	lib->data[idx] = buff;

	free( lib->names[idx] );
	lib->names[idx] = dup_name;

	/* Give a little feedback.
	*/
	if( verbose ) {
		printf( "r - %s\n", filename );
	}

	return B_OK;
}

/* ----------------------------------------------------------------------
** Print the table for an archive.
*/
static status_t table_lib_entry( MWLib *lib, int idx, int verbose );

status_t do_table( const char *archive_name, char **files, int verbose )
{
	status_t retval = B_OK;
	MWLib lib;
	int idx = 0;

	ASSERT( archive_name != NULL );

	retval = load_MW_lib( &lib, archive_name );
	if( retval != B_OK ) {
		switch( retval ) {
		case B_FILE_NOT_FOUND:
			fprintf( stderr, "ar: %s, file not found\n", archive_name );
			return retval;
			break;

		default:
			return retval;
			break;
		}
	}

	if( files == NULL ) {
		/* Then we print the table for the entire archive.
		*/
		for( idx = 0; idx < lib.header.num_objects; idx++ ) {
			retval = table_lib_entry( &lib, idx, verbose );
		}
	} else {
		/* Then we print the table for the specified files.
		*/
		int which = 0;

		for( idx = 0; files[idx] != NULL; idx++ ) {
			if( do_match( &lib, files[idx], &which ) ) {
				retval = table_lib_entry( &lib, which, verbose );
			}
			which = 0;
		}
	}

	return retval;
}

static status_t table_lib_entry( MWLib *lib, int idx, int verbose )
{
	struct tm *t;
	char month_buff[4];

	ASSERT( lib != NULL );

	if( verbose ) {
		t = localtime( &(lib->files[idx].m_time) );
		if( t == NULL ) {
			fprintf( stderr, "localtime() failed, %s\n",
					 strerror( errno ) );
			return B_OK;
		}

		if( strftime( month_buff, sizeof( month_buff ),
					  "%b", t ) == 0 ) {
			/* TODO: error message */
			fprintf( stderr, "strftime() failed, %s\n",
					 strerror( errno ) );
			return B_OK;
		}

		/* I wish POSIX allowed for a nicer format; even using tabs
		 * between some entries would be better.
		 */
		printf( "%s %u/%u %u %s %d %d:%d %d %s\n",
				"-rw-r--r--", /* simulated mode */
				getuid(), getgid(), /* simulated uid & gid */
				lib->files[idx].object_size,
				month_buff, /* abbreviated month */
				t->tm_mon, /* day of month */
				t->tm_hour, /* hour */
				t->tm_min, /* minute */
				t->tm_year, /* year */
				lib->names[idx] );
	} else {
		printf( "%s\n", lib->names[idx] );
	}

	return B_OK;
}

/* ----------------------------------------------------------------------
** Extract one or more files from the archive.
*/
static status_t extract_lib_entry( MWLib *lib, int idx, int verbose );

status_t do_extract( const char *archive_name, char **files, int verbose )
{
	status_t retval = B_OK;
	MWLib lib;
	int idx = 0;

	ASSERT( archive_name != NULL );

	retval = load_MW_lib( &lib, archive_name );
	if( retval != B_OK ) {
		switch( retval ) {
		case B_FILE_NOT_FOUND:
			fprintf( stderr, "ar: %s, file not found\n", archive_name );
			return retval;
			break;

		default:
			return retval;
			break;
		}
	}

	if( files == NULL ) {
		/* Then we extract all the files.
		*/
		for( idx = 0; idx < lib.header.num_objects; idx++ ) {
			retval = extract_lib_entry( &lib, idx, verbose );
		}
	} else {
		/* Then we extract the specified files.
		*/
		int which = 0;

		for( idx = 0; files[idx] != NULL; idx++ ) {
			if( do_match( &lib, files[idx], &which ) ) {
				retval = extract_lib_entry( &lib, which, verbose );
			}
			which = 0;
		}
	}

	return retval;
}

static status_t extract_lib_entry( MWLib *lib, int idx, int verbose )
{
	FILE *fp;
	int recs;
	status_t retval = B_OK;
	struct stat s;
	mode_t mode_bits = 0666;	/* TODO: use user's umask() instead */

	ASSERT( lib != NULL );

	/* Delete the file if it already exists.
	*/
	retval = access( lib->names[idx], F_OK );
	if( retval == 0 ) {
		retval = stat( lib->names[idx], &s );
		if( retval != 0 ) {
			fprintf( stderr, "ar: can't stat %s, %s\n", lib->names[idx],
					 strerror( errno ) );
		} else {
			mode_bits = s.st_mode;
		}
		retval = unlink( lib->names[idx] );
		if( retval != 0 ) {
			fprintf( stderr, "ar: can't unlink %s, %s\n", lib->names[idx],
					strerror( retval ) );
			return B_OK;
		}
	}

	/* Write the file.
	*/
	if( verbose ) {
		printf( "x - %s\n", lib->names[idx] );
	}

	fp = fopen( lib->names[idx], "w" );
	if( fp == NULL ) {
		fprintf( stderr, "ar: can't open %s for write, %s\n", lib->names[idx],
				 strerror( errno ) );
		return B_OK;
	}

	recs = fwrite( lib->data[idx], lib->files[idx].object_size, 1, fp );
	if( recs != 1 ) {
		fprintf( stderr, "error writing %s, %s\n", lib->names[idx],
				 strerror( errno ) );
	}

	retval = fclose( fp );

	/* Set the newly extracted file's modification time to the time
	** stored in the archive.
	*/
	retval = stat( lib->names[idx], &s );
	if( retval != 0 ) {
		fprintf( stderr, "ar: can't stat %s, %s\n", lib->names[idx], 
				strerror( errno ) );
	} else {
		struct utimbuf new_times;

		new_times.actime = s.st_atime;
		new_times.modtime = lib->files[idx].m_time;

		retval = utime( lib->names[idx], &new_times );
		if( retval != 0 ) {
			fprintf( stderr, "ar: can't set modification time for %s, %s\n",
					 lib->names[idx], strerror( retval ) );
		}
	}

	/* Set the newly extracted file's mode.
	*/
	retval = chmod( lib->names[idx], mode_bits );
	if( retval != 0 ) {
		fprintf( stderr, "ar: unable to change file mode for %s, %s\n",
				 lib->names[idx], strerror( errno ) );
	}

	/* Set the newly extracted file's type.
	*/
	setfiletype( lib->names[idx], "application/x-mw-library" );

	return B_OK;
}
