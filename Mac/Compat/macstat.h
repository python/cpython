/* Include file belonging to stat emulator.
   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987). */

struct stat {
	unsigned short st_mode;
	unsigned long st_size;
	unsigned long st_rsize; /* Resource size -- nonstandard */
	unsigned long st_mtime;
};

#ifdef UNIX_COMPAT
#define S_IFMT	0170000L
#define S_IFDIR	0040000L
#define S_IFREG 0100000L
#define S_IREAD    0400
#define S_IWRITE   0200
#define S_IEXEC    0100
#else
#define S_IFMT	0xFFFF
#define S_IFDIR	0x0000
#define S_IFREG 0x0003
#define S_IREAD    0400
#define S_IWRITE   0200
#define S_IEXEC    0100
#endif
