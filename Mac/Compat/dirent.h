/*
 * "Dir.h" for the Macintosh.
 * Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
 */

#define MAXNAMLEN 31
#define MAXPATH 256

#define DIR  struct _dir

struct _dir {
	short vrefnum;
	long dirid;
	int nextfile;
};

struct dirent {
	char d_name[MAXPATH];
};

extern DIR *opendir(char *);
extern struct dirent *readdir(DIR *);
extern void closedir(DIR *);
