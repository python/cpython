/* Include file belonging to stat emulator.
   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
   Updated August 1994. */

struct macstat {
	unsigned short st_dev;
	unsigned long st_ino;
	unsigned short st_mode;
	unsigned short st_nlink;
	unsigned short st_uid;
	unsigned short st_gid;
	unsigned short st_rdev;
	unsigned long st_size;
	unsigned long st_atime;
	unsigned long st_mtime;
	unsigned long st_ctime;
	/* Non-standard additions */
	unsigned long st_rsize; /* Resource size */
	char st_type[4]; /* File type, e.g. 'APPL' or 'TEXT' */
	char st_creator[4]; /* File creator, e.g. 'PYTH' */
};

#define S_IFMT	0170000
#define S_IFDIR	0040000
#define S_IFREG 0100000
#define S_IREAD    0400
#define S_IWRITE   0200
#define S_IEXEC    0100

extern int macstat(char *, struct macstat *);
/* To stop inclusion of MWerks header: */
#ifndef _STAT
#define _STAT
#endif
