/*
** main.c - POSIX 1003.2 "ar" command
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "commands.h"

static const char *rcs_version_id = "$Id$";
static const char *ar_version_id = "1.0 " __DATE__;

/* ---------------------------------------------------------------------- */
typedef enum {
	delete_cmd,
	print_cmd,
	replace_cmd,
	table_cmd,
	extract_cmd,
	no_cmd = -1 } command;

/* ----------------------------------------------------------------------
** Prototypes
*/
void usage( void );
void version( void );
void check_command( command *cmd, int arg );

/* ----------------------------------------------------------------------
** Print a usage message and exit.
*/
void usage( void )
{
	printf( "ar [dprtx][cuv] archive [file ...]\n" );

	exit( EXIT_FAILURE );
}

/* ----------------------------------------------------------------------
** Print a version message and exit.
*/
void version( void )
{
	printf( "ar (POSIX 1003.2-1992), version %s\n", ar_version_id );
	printf( "by Chris Herborth (chrish@qnx.com)\n" );
	printf( "This code has been donated to the BeOS developer community.\n" );

	return;
}

/* ----------------------------------------------------------------------
** Set *cmd to the appropriate command enum if it isn't already set.
*/
void check_command( command *cmd, int arg )
{
	if( *cmd == no_cmd ) {
		switch( arg ) {
		case 'd':
			*cmd = delete_cmd;
			break;
		case 'p':
			*cmd = print_cmd;
			break;
		case 'r':
			*cmd = replace_cmd;
			break;
		case 't':
			*cmd = table_cmd;
			break;
		case 'x':
			*cmd = extract_cmd;
			break;
		}
	} else {
		printf( "ar: you can only specify one command at a time\n" );
		usage();
	}
}

/* ----------------------------------------------------------------------
** Mainline
*/
int main( int argc, char **argv )
{
	command cmd = no_cmd;
	int verbose_flag = 0;
	int create_flag = 0;	/* these two only apply to replace_cmd */
	int update_flag = 0;
	int c = 0;

	char *archive_name;
	char **files_list;
	int num_files;

	int idx;
	status_t retval;

	/* The argument parsing is a little hairier than usual; the idea is
	** to support the POSIX 1003.2 style of arguments, and the much more
	** common traditional argument style.
	*/
	if( argc < 3 ) {
		printf( "ar: invalid number of arguments\n" );
		usage();
	}

	/* Do we have traditional or POSIX-style args? */
	if( argv[1][0] == '-' ) {
		while( ( c = getopt( argc, argv, "dprtxcuvV" ) ) != EOF ) {
			switch( c ) {
			case 'd':	/* fall-through */
			case 'p':	/* fall-through */
			case 'r':	/* fall-through */
			case 't':	/* fall-through */
			case 'x':	/* fall-through */
				check_command( &cmd, c );
				break;

			case 'v':
				verbose_flag = 1;
				break;

			case 'c':
				if( cmd != no_cmd && cmd != replace_cmd ) {
					printf( "ar: invalid option, -c\n" );
					usage();
				} else {
					create_flag = 1;
				}
				break;

			case 'u':
				if( cmd != no_cmd && cmd != replace_cmd ) {
					printf( "ar: invalid option, -u\n" );
					usage();
				} else {
					update_flag = 1;
				}
				break;

			case 'V':
				version();
				break;

			default:
				printf( "ar: invalid option, -%c\n", c );
				usage();
				break;
			}

			idx = optind;
		}
	} else {
		/* In the traditional way, arguments ar:
		**
		** argv[1] = [dprtx][cuv]
		** argv[2] = archive
		** argv[...] = file ...
		**/
		char *ptr;

		idx = 1;

		ptr = argv[idx++];

		while( *ptr != '\0' ) {
			switch( *ptr ) {
			case 'd':	/* fall-through */
			case 'p':	/* fall-through */
			case 'r':	/* fall-through */
			case 't':	/* fall-through */
			case 'x':	/* fall-through */
				check_command( &cmd, *ptr );
				break;

			case 'v':
				verbose_flag = 1;
				break;

			case 'c':
				if( cmd != no_cmd && cmd != replace_cmd ) {
					printf( "ar: invalid option, -c\n" );
					usage();
				} else {
					create_flag = 1;
				}
				break;

			case 'u':
				if( cmd != no_cmd && cmd != replace_cmd ) {
					printf( "ar: invalid option, -u\n" );
					usage();
				} else {
					update_flag = 1;
				}
				break;

			case 'V':
				version();
				break;

			default:
				printf( "ar: invalid option, -%c\n", c );
				usage();
				break;
			}

			ptr++;
		}
	}

	/* Next arg is the archive. */
	archive_name = argv[idx++];

	/* Next are the files. */
	num_files = argc - idx;

	if( num_files == 0 ) {
		files_list = NULL;
	} else {
		int ctr = 0;

		files_list = (char **)malloc( ( num_files + 1 ) * sizeof( char * ) );

		while( idx < argc ) {
			files_list[ctr++] = argv[idx++];
		}

		files_list[idx] = NULL;
	}

	/* Now we can attempt to manipulate the archive. */
	switch( cmd ) {
	case delete_cmd:
		retval = do_delete( archive_name, files_list, verbose_flag );
		break;

	case print_cmd:
		retval = do_print( archive_name, files_list, verbose_flag );
		break;

	case replace_cmd:
		retval = do_replace( archive_name, files_list, verbose_flag,
							 create_flag, update_flag );
		break;

	case table_cmd:
		retval = do_table( archive_name, files_list, verbose_flag );
		break;

	case extract_cmd:
		retval = do_extract( archive_name, files_list, verbose_flag );
		break;

	default:
		printf( "ar: you must specify a command\n" );
		usage();
		break;
	}

	/* Check the return value.
	*/
	switch( retval ) {
	case B_OK:
		break;
	case B_FILE_NOT_FOUND:
		printf( "can't open the file %s\n", archive_name );
		return EXIT_FAILURE;
		break;
	case B_IO_ERROR:
		printf( "can't read from %s\n", archive_name );
		return EXIT_FAILURE;
		break;
	case B_BAD_VALUE:
		printf( "invalid magic word\n" );
		return EXIT_FAILURE;
		break;
	case B_MISMATCHED_VALUES:
		printf( "invalid processor value, or magicflags, or version\n" );
		return EXIT_FAILURE;
		break;
	case B_NO_MEMORY:
		printf( "unable to allocate memory\n" );
		return EXIT_FAILURE;
		break;
	case B_ERROR:
		printf( "error during processing\n" );
		return EXIT_FAILURE;
	default:
		printf( "unknown error: %ld\n", retval );
		return EXIT_FAILURE;
		break;
	}

	return EXIT_SUCCESS;
}
