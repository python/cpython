/*
** Resource-IDs in use by Python.
**
** All resources used by the python interpreter itself fall
** in the range 128-256.
**
** Standard python modules use resources in the range
** 256-512.
**
** Python programs that use their own resources are advised to
** choose resource numbers higher than 512.
*/

/*
** Resources that reside in the python executable (or, for
** shared ppc python, in the core dynamic library)
*/

/* The alert for "No Python directory, where is it?" */
#define NOPYTHON_ALERT 128
#define YES_ITEM 1
#define NO_ITEM 2
#define CURWD_ITEM 3

/* The alert for "this is an applet template" */
#define NOPYC_ALERT 129

/* The dialog for our GetDirectory and PromptGetFile call */
#define GETDIR_ID 130		/* Resource ID for our "get directory" */
#define GETFILEPROMPT_ID 132	/* Resource id for prompted get file */
#define PROMPT_ITEM	10		/* The prompt, at the top */
#define SELECTCUR_ITEM 11	/* "Select current directory" button */


/* The dialog for interactive options */
#define OPT_DIALOG		131		/* Resource ID for dialog */
#define OPT_OK			1
#define OPT_CANCEL		2
#define OPT_INSPECT		3
#define OPT_VERBOSE		4
#define OPT_SUPPRESS	5
#define OPT_UNBUFFERED	6
#define OPT_DEBUGGING	7

/*
** The following are valid both in the binary (or shared library)
** and in the Preferences file
*/

/* The STR# resource for sys.path initialization */
#define PYTHONPATH_ID 128

/* The alis resource for locating the python home directory */
#define PYTHONHOME_ID 128
