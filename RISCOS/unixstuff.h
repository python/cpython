/* Fudge unix isatty and fileno for RISCOS */

#include <stdio.h>
#include <time.h>

int fileno(FILE *f);
int isatty(int fn);
unsigned int unixtime(unsigned int ld,unsigned int ex);
int acorntime(unsigned int *ex, unsigned int *ld, time_t ut);

int isdir(char *fn);
int isfile(char *fn);
int object_exists(char *fn);

