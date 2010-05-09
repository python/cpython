/*
   Template for a setuid program that calls a script.

   The script should be in an unwritable directory and should itself
   be unwritable.  In fact all parent directories up to the root
   should be unwritable.  The script must not be setuid, that's what
   this program is for.

   This is a template program.  You need to fill in the name of the
   script that must be executed.  This is done by changing the
   definition of FULL_PATH below.

   There are also some rules that should be adhered to when writing
   the script itself.

   The first and most important rule is to never, ever trust that the
   user of the program will behave properly.  Program defensively.
   Check your arguments for reasonableness.  If the user is allowed to
   create files, check the names of the files.  If the program depends
   on argv[0] for the action it should perform, check it.

   Assuming the script is a Bourne shell script, the first line of the
   script should be
    #!/bin/sh -
   The - is important, don't omit it.  If you're using esh, the first
   line should be
    #!/usr/local/bin/esh -f
   and for ksh, the first line should be
    #!/usr/local/bin/ksh -p
   The script should then set the variable IFS to the string
   consisting of <space>, <tab>, and <newline>.  After this (*not*
   before!), the PATH variable should be set to a reasonable value and
   exported.  Do not expect the PATH to have a reasonable value, so do
   not trust the old value of PATH.  You should then set the umask of
   the program by calling
    umask 077 # or 022 if you want the files to be readable
   If you plan to change directories, you should either unset CDPATH
   or set it to a good value.  Setting CDPATH to just ``.'' (dot) is a
   good idea.
   If, for some reason, you want to use csh, the first line should be
    #!/bin/csh -fb
   You should then set the path variable to something reasonable,
   without trusting the inherited path.  Here too, you should set the
   umask using the command
    umask 077 # or 022 if you want the files to be readable
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

/* CONFIGURATION SECTION */

#ifndef FULL_PATH       /* so that this can be specified from the Makefile */
/* Uncomment the following line:
#define FULL_PATH       "/full/path/of/script"
* Then comment out the #error line. */
#error "You must define FULL_PATH somewhere"
#endif
#ifndef UMASK
#define UMASK           077
#endif

/* END OF CONFIGURATION SECTION */

#if defined(__STDC__) && defined(__sgi)
#define environ _environ
#endif

/* don't change def_IFS */
char def_IFS[] = "IFS= \t\n";
/* you may want to change def_PATH, but you should really change it in */
/* your script */
#ifdef __sgi
char def_PATH[] = "PATH=/usr/bsd:/usr/bin:/bin:/usr/local/bin:/usr/sbin";
#else
char def_PATH[] = "PATH=/usr/ucb:/usr/bin:/bin:/usr/local/bin";
#endif
/* don't change def_CDPATH */
char def_CDPATH[] = "CDPATH=.";
/* don't change def_ENV */
char def_ENV[] = "ENV=:";

/*
   This function changes all environment variables that start with LD_
   into variables that start with XD_.  This is important since we
   don't want the script that is executed to use any funny shared
   libraries.

   The other changes to the environment are, strictly speaking, not
   needed here.  They can safely be done in the script.  They are done
   here because we don't trust the script writer (just like the script
   writer shouldn't trust the user of the script).
   If IFS is set in the environment, set it to space,tab,newline.
   If CDPATH is set in the environment, set it to ``.''.
   Set PATH to a reasonable default.
*/
void
clean_environ(void)
{
    char **p;
    extern char **environ;

    for (p = environ; *p; p++) {
        if (strncmp(*p, "LD_", 3) == 0)
            **p = 'X';
        else if (strncmp(*p, "_RLD", 4) == 0)
            **p = 'X';
        else if (strncmp(*p, "PYTHON", 6) == 0)
            **p = 'X';
        else if (strncmp(*p, "IFS=", 4) == 0)
            *p = def_IFS;
        else if (strncmp(*p, "CDPATH=", 7) == 0)
            *p = def_CDPATH;
        else if (strncmp(*p, "ENV=", 4) == 0)
            *p = def_ENV;
    }
    putenv(def_PATH);
}

int
main(int argc, char **argv)
{
    struct stat statb;
    gid_t egid = getegid();
    uid_t euid = geteuid();

    /*
       Sanity check #1.
       This check should be made compile-time, but that's not possible.
       If you're sure that you specified a full path name for FULL_PATH,
       you can omit this check.
    */
    if (FULL_PATH[0] != '/') {
        fprintf(stderr, "%s: %s is not a full path name\n", argv[0],
            FULL_PATH);
        fprintf(stderr, "You can only use this wrapper if you\n");
        fprintf(stderr, "compile it with an absolute path.\n");
        exit(1);
    }

    /*
       Sanity check #2.
       Check that the owner of the script is equal to either the
       effective uid or the super user.
    */
    if (stat(FULL_PATH, &statb) < 0) {
        perror("stat");
        exit(1);
    }
    if (statb.st_uid != 0 && statb.st_uid != euid) {
        fprintf(stderr, "%s: %s has the wrong owner\n", argv[0],
            FULL_PATH);
        fprintf(stderr, "The script should be owned by root,\n");
        fprintf(stderr, "and shouldn't be writeable by anyone.\n");
        exit(1);
    }

    if (setregid(egid, egid) < 0)
        perror("setregid");
    if (setreuid(euid, euid) < 0)
        perror("setreuid");

    clean_environ();

    umask(UMASK);

    while (**argv == '-')       /* don't let argv[0] start with '-' */
        (*argv)++;
    execv(FULL_PATH, argv);
    fprintf(stderr, "%s: could not execute the script\n", argv[0]);
    exit(1);
}
