/*
** commands.h - POSIX 1003.2 "ar" command
**
** $Id$
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

#include <be/support/SupportDefs.h>

status_t do_delete( const char *archive_name, char **files, int verbose );
status_t do_print( const char *archive_name, char **files, int verbose );
status_t do_replace( const char *archive_name, char **files, int verbose,
					 int create, int update );
status_t do_table( const char *archive_name, char **files, int verbose );
status_t do_extract( const char *archive_name, char **files, int verobse );
