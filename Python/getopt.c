/* An implementation of getopt() by Amrit Prem */

#include <stdio.h>
#include <string.h>

#define bool int
#define TRUE  1
#define FALSE 0
#define EOS   '\0'

bool    opterr = TRUE;          /* generate error messages */
int     optind = 1;             /* index into argv array   */
char *  optarg = NULL;          /* optional argument       */


int getopt(int argc, char *argv[], const char optstring[])
{
   static   char *opt_ptr = "";
   register char *ptr;
	    int   option;

   if (*opt_ptr == EOS) {

      if (optind >= argc || argv[optind][0] != '-')
	 return -1;

      else if (strcmp(argv[optind], "--") == 0) {
	 ++optind;
	 return -1;
      }

      opt_ptr = argv[optind++] + 1;
   }

   if ((ptr = strchr(optstring, option = *opt_ptr++)) == NULL) {
      if (opterr)
	 fprintf(stderr, "Unknown option: -%c\n", option);

      return '?';
   }

   if (*(ptr + 1) == ':') {
      if (optind >= argc) {
	 if (opterr)
	    fprintf(stderr, "Argument expected for the -%c option\n", option);

	 return '?';
      }

      optarg = argv[optind++];
   }

   return option;
}
