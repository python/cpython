/*
** mwlib.h - POSIX 1003.2 "ar" command
**
** $Id$
**
** This isn't a pure POSIX 1003.2 ar; it only manipulates Metrowerks
** Library files, not general-purpose POSIX 1003.2 format archives.
**
** Dec. 14, 1997 Chris Herborth (chrish@qnx.com)
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

#include <support/SupportDefs.h>
#include <time.h>

/* ----------------------------------------------------------------------
** Constants
**
*/
#define MWLIB_MAGIC_WORD 'MWOB'
#define MWLIB_MAGIC_PROC 'PPC '

/* ----------------------------------------------------------------------
** Structures
**
** This is based on the "Metrowerks CodeWarrior Library Reference
** Specification", which isn't 100% accurate for BeOS.
*/

typedef struct MWLibHeader {
	uint32 magicword;
	uint32 magicproc;
	uint32 magicflags;
	uint32 version;

	uint32 code_size;
	uint32 data_size;

	uint32 num_objects;
} MWLibHeader;

typedef struct MWLibFile {
	time_t m_time;

	uint32 off_filename;
	uint32 off_fullpath;
	uint32 off_object;
	uint32 object_size;
} MWLibFile;

typedef struct MWLib {
	MWLibHeader header;

	MWLibFile *files;

	char **names;

	char **data;
} MWLib;

/* This bears no resemblance to what's in the Metrowerks docs.
**
** Note that this is incomplete; this is all the info I needed for
** ar though.
*/
typedef struct MWObject {
	uint32 magic_word;			/* 'MWOB' */
	uint32 arch;				/* 'PPC '; this isn't in the docs */
	uint32 version;
	uint32 flags;

	uint32 code_size;
	uint32 data_size;
} MWObject;

/* ----------------------------------------------------------------------
** Function prototypes
**
** load_MW_lib() - load a Metrowerks library
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
**
** write_MW_lib() - write a Metrowerks library
**
** Returns:
**    B_OK - all is well
**
** write_MW_lib() - write a Metrowerks library file
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
*/
status_t load_MW_lib( MWLib *lib, const char *filename );
status_t write_MW_lib( MWLib *lib, const char *filename );
void setfiletype( const char *filename, const char *type );
