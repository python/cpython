/* Fudge unix isatty and fileno for RISCOS */

#include <stdio.h>

int fileno(FILE *f);
int isatty(int fn);
unsigned int unixtime(unsigned int ld,unsigned int ex);
/*long PyOS_GetLastModificationTime(char *name);*/
int unlink(char *fname);
int isdir(char *fn);
int isfile(char *fn);
int exists(char *fn);

