#include "Python.h"
#include "osdefs.h"

#include "pythonresources.h"


/* Return the initial python search path.  This is called once from
** initsys() to initialize sys.path.
**
** If USE_BUILTIN_PATH is defined the path defined here is used
** (after prepending the python home dir to each item).
** If it is not defined the path is gotten from a resource in the
** Preferences file.
**
** XXXX This code needs cleaning up. The routines here have moved
** around quite a bit, and they're pretty messy for that reason.
*/

#include <Files.h>
#include <Aliases.h>
#include <Folders.h>
#include <Resources.h>
#include <TextUtils.h>
#include <Dialogs.h>

#define PYTHONPATH "\
:\n\
:Lib\n\
:Lib:stdwin\n\
:Lib:test\n\
:Lib:mac"


char *
getpythonpath()
{
	/* Modified by Jack to do something a bit more sensible:
	** - Prepend the python home-directory (which is obtained from a Preferences
	**   resource)
	** - Add :
	*/
	static char *pythonpath;
	char *curwd;
	char *p, *endp;
	int newlen;
	extern char *PyMac_GetPythonDir();
#ifndef USE_BUILTIN_PATH
	extern char *PyMac_GetPythonPath();
#endif
	
	if ( pythonpath ) return pythonpath;
	curwd = PyMac_GetPythonDir();
#ifndef USE_BUILTIN_PATH
	if ( pythonpath = PyMac_GetPythonPath(curwd) )
		return pythonpath;
	printf("Warning: No pythonpath resource found, using builtin default\n");
#endif
	p = PYTHONPATH;
	endp = p;
	pythonpath = malloc(2);
	if ( pythonpath == NULL ) return PYTHONPATH;
	strcpy(pythonpath, ":");
	while (*endp) {
		endp = strchr(p, '\n');
		if ( endp == NULL )
			endp = p + strlen(p);
		newlen = strlen(pythonpath) + 1 + strlen(curwd) + (endp-p);
		pythonpath = realloc(pythonpath, newlen+1);
		if ( pythonpath == NULL ) return PYTHONPATH;
		strcat(pythonpath, "\n");
		if ( *p == ':' ) {
			p++;
			strcat(pythonpath, curwd);
			strncat(pythonpath, p, (endp-p));
			newlen--;   /* Ok, ok, we've allocated one byte too much */
		} else {
			/* We've allocated too much in this case */
			newlen -= strlen(curwd);
			pythonpath = realloc(pythonpath, newlen+1);
			if ( pythonpath == NULL ) return PYTHONPATH;
			strncat(pythonpath, p, (endp-p));
		}
		pythonpath[newlen] = '\0';
		p = endp + 1;
	}
	return pythonpath;
}

/*
** Open/create the Python Preferences file, return the handle
*/
short
PyMac_OpenPrefFile()
{
    AliasHandle handle;
    FSSpec dirspec;
    short prefrh;
    short prefdirRefNum;
    long prefdirDirID;
    short action;
    OSErr err;

    if ( FindFolder(kOnSystemDisk, 'pref', kDontCreateFolder, &prefdirRefNum,
    				&prefdirDirID) != noErr ) {
    	/* Something wrong with preferences folder */
    	(void)StopAlert(NOPREFDIR_ID, NULL);
    	exit(1);
    }
    
	(void)FSMakeFSSpec(prefdirRefNum, prefdirDirID, "\pPython Preferences", &dirspec);
	prefrh = FSpOpenResFile(&dirspec, fsRdWrShPerm);
	if ( prefrh < 0 ) {
		action = CautionAlert(NOPREFFILE_ID, NULL);
		if ( action == NOPREFFILE_NO )
			exit(1);
	
		FSpCreateResFile(&dirspec, 'PYTH', 'pref', 0);
		prefrh = FSpOpenResFile(&dirspec, fsRdWrShPerm);
		if ( prefrh == -1 ) {
			/* This "cannot happen":-) */
			printf("Cannot create preferences file, error %d\n", ResError());
			exit(1);
		}
		if ( (err=PyMac_process_location(&dirspec)) != 0 ) {
			printf("Cannot get FSSpec for application, error %d\n", err);
			exit(1);
		}
		dirspec.name[0] = 0;
		if ((err=NewAlias(NULL, &dirspec, &handle)) != 0 ) {
			printf("Cannot make alias to application directory, error %d\n", err);
			exit(1);
		}
    	AddResource((Handle)handle, 'alis', PYTHONHOME_ID, "\p");
    	UpdateResFile(prefrh);

	} else {
		UseResFile(prefrh);
	}
	return prefrh;
}

/*
** Return the name of the Python directory
*/
char *
PyMac_GetPythonDir()
{
    static char name[256];
    AliasHandle handle;
    FSSpec dirspec;
    Boolean modified = 0;
    short oldrh, prefrh;
    
    /*
    ** Remember old resource file and try to open preferences file
    ** in the preferences folder.
    */
    oldrh = CurResFile();
    prefrh = PyMac_OpenPrefFile();
    /* So, we've opened our preferences file, we hope. Look for the alias */
    handle = (AliasHandle)Get1Resource('alis', PYTHONHOME_ID);
    if ( handle == NULL ) {
    	(void)StopAlert(BADPREFFILE_ID, NULL);
    	exit(1);
    }
	/* It exists. Resolve it (possibly updating it) */
	if ( ResolveAlias(NULL, handle, &dirspec, &modified) != noErr ) {
    	(void)StopAlert(BADPREFFILE_ID, NULL);
    	exit(1);
    }
    if ( modified ) {
   		ChangedResource((Handle)handle);
    	UpdateResFile(prefrh);
    }
   	CloseResFile(prefrh);
    UseResFile(oldrh);

   	if ( nfullpath(&dirspec, name) == 0 ) {
   		strcat(name, ":");
    } else {
 		/* If all fails, we return the current directory */
   		printf("Python home dir exists but I cannot find the pathname!!\n");
		name[0] = 0;
		(void)getwd(name);
	}
	return name;
}

#ifndef USE_BUILTIN_PATH
char *
PyMac_GetPythonPath(dir)
char *dir;
{
    FSSpec dirspec;
    short oldrh, prefrh = -1;
    short prefdirRefNum;
    long prefdirDirID;
    char *rv;
    int i, newlen;
    Str255 pathitem;
    
    /*
    ** Remember old resource file and try to open preferences file
    ** in the preferences folder.
    */
    oldrh = CurResFile();
    if ( FindFolder(kOnSystemDisk, 'pref', kDontCreateFolder, &prefdirRefNum,
    				&prefdirDirID) == noErr ) {
    	(void)FSMakeFSSpec(prefdirRefNum, prefdirDirID, "\pPython Preferences", &dirspec);
		prefrh = FSpOpenResFile(&dirspec, fsRdWrShPerm);
    }
    /* At this point, we may or may not have the preferences file open, and it
    ** may or may not contain a sys.path STR# resource. We don't care, if it doesn't
    ** exist we use the one from the application (the default).
    ** We put an initial '\n' in front of the path that we don't return to the caller
    */
    if( (rv = malloc(2)) == NULL )
    	goto out;
    strcpy(rv, "\n");
    for(i=1; ; i++) {
    	GetIndString(pathitem, PYTHONPATH_ID, i);
    	if( pathitem[0] == 0 )
    		break;
    	if ( pathitem[0] >= 9 && strncmp((char *)pathitem+1, "$(PYTHON)", 9) == 0 ) {
    		/* We have to put the directory in place */
    		newlen = strlen(rv) + strlen(dir) + (pathitem[0]-9) + 2;
    		if( (rv=realloc(rv, newlen)) == NULL)
    			goto out;
    		strcat(rv, dir);
    		/* Skip a colon at the beginning of the item */
    		if ( pathitem[0] > 9 && pathitem[1+9] == ':' ) {
				memcpy(rv+strlen(rv), pathitem+1+10, pathitem[0]-10);
				newlen--;
			} else {
				memcpy(rv+strlen(rv), pathitem+1+9, pathitem[0]-9);
			}
    		rv[newlen-2] = '\n';
    		rv[newlen-1] = 0;
    	} else {
    		/* Use as-is */
    		newlen = strlen(rv) + (pathitem[0]) + 2;
    		if( (rv=realloc(rv, newlen)) == NULL)
    			goto out;
    		memcpy(rv+strlen(rv), pathitem+1, pathitem[0]);
    		rv[newlen-2] = '\n';
    		rv[newlen-1] = 0;
    	}
	}
	if( strlen(rv) == 1) {
		free(rv);
		rv = NULL;
	}
	if ( rv ) {
		rv[strlen(rv)-1] = 0;
		rv++;
	}
out:
	if ( prefrh ) {
		CloseResFile(prefrh);
		UseResFile(oldrh);
	}
	return rv;
}
#endif /* !USE_BUILTIN_PATH */

