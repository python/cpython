/*
** copy_attrs.h - copy BeFS attributes from one file to another
**
** $Id$
**
** Jan. 11, 1998 Chris Herborth (chrish@qnx.com)
**
** This code is donated to the PUBLIC DOMAIN.  You can use, abuse, modify,
** redistribute, steal, or otherwise manipulate this code.  No restrictions
** at all.  If you laugh at this code, you can't use it.
*/

/* ----------------------------------------------------------------------
** Function prototypes
**
** copy_attrs() - copy BeFS attributes from one file to another
**
** Returns:
**    B_OK - all is well
**    B_FILE_NOT_FOUND - can't open one of the named files
**    B_IO_ERROR - can't read/write some of the file attributes
**    B_NO_MEMORY - unable to allocate a buffer for the attribute data
*/
status_t copy_attrs( const char *dest_file, const char *src_file );
