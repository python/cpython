/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

#include "Python.h"
#include "osdefs.h"
#include "macglue.h"
#include "macdefs.h"
#include "pythonresources.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


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

#ifdef USE_GUSI1
#include <GUSI.h>
#endif

#ifndef USE_BUILTIN_PATH
staticforward char *PyMac_GetPythonPath();
#endif

#define PYTHONPATH "\
:\n\
:Lib\n\
:Lib:stdwin\n\
:Lib:test\n\
:Lib:mac"

static int
getpreffilefss(FSSpec *fssp)
{
	static int diditbefore=0;
	static int rv = 1;
	static FSSpec fss;
	short prefdirRefNum;
	long prefdirDirID;
	long pyprefdirDirID;
	Handle namehandle;
	OSErr err;
	
	if ( !diditbefore ) {
		if ( (namehandle=GetNamedResource('STR ', PREFFILENAME_NAME)) == NULL ) {
			(void)StopAlert(NOPREFNAME_ID, NULL);
			exit(1);
		}
		
		if ( **namehandle == '\0' ) {
			/* Empty string means don't use preferences file */
			rv = 0;
		} else {
			/* There is a filename, construct the fsspec */
			if ( FindFolder(kOnSystemDisk, 'pref', kDontCreateFolder, &prefdirRefNum,
							&prefdirDirID) != noErr ) {
				/* Something wrong with preferences folder */
				(void)StopAlert(NOPREFDIR_ID, NULL);
				exit(1);
			}
			/* make fsspec for the "Python" folder inside the prefs folder */
			err = FSMakeFSSpec(prefdirRefNum, prefdirDirID, "\pPython", &fss);
			if (err == fnfErr) {
				/* it doesn't exist: create it */
				err = FSpDirCreate(&fss, smSystemScript, &pyprefdirDirID);
			} else {
				/* it does exist, now find out the dirID of the Python prefs folder, brrr. */
				CInfoPBRec info;
				info.dirInfo.ioVRefNum 		= fss.vRefNum;
				info.dirInfo.ioDrDirID 		= fss.parID;
				info.dirInfo.ioNamePtr 		= fss.name;
				info.dirInfo.ioFDirIndex 	= 0;
				info.dirInfo.ioACUser 		= 0;
				err = PBGetCatInfo(&info, 0);
				if (err == noErr) {
					pyprefdirDirID = info.dirInfo.ioDrDirID;
				}
			}
			if (err != noErr) {
				(void)StopAlert(NOPREFDIR_ID, NULL);
				exit(1);
			}
			HLock(namehandle);
			err = FSMakeFSSpec(fss.vRefNum, pyprefdirDirID, (unsigned char *)*namehandle, &fss);
			HUnlock(namehandle);
			if (err != noErr && err != fnfErr) {
				(void)StopAlert(NOPREFDIR_ID, NULL);
				exit(1);
			}
		}
		ReleaseResource(namehandle);
		diditbefore = 1;
	}
	*fssp = fss;
	return rv;
}

char *
Py_GetPath()
{
	/* Modified by Jack to do something a bit more sensible:
	** - Prepend the python home-directory (which is obtained from a Preferences
	**   resource)
	** - Add :
	*/
	static char *pythonpath;
	char *p, *endp;
	int newlen;
	char *curwd;
	
	if ( pythonpath ) return pythonpath;
#ifndef USE_BUILTIN_PATH
	if ( pythonpath = PyMac_GetPythonPath() )
		return pythonpath;
	printf("Warning: No pythonpath resource found, using builtin default\n");
#endif
	curwd = PyMac_GetPythonDir();
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
	OSErr err;

	if ( !getpreffilefss(&dirspec))
		return -1;
	prefrh = FSpOpenResFile(&dirspec, fsRdWrShPerm);
	if ( prefrh < 0 ) {
#if 0
		action = CautionAlert(NOPREFFILE_ID, NULL);
		if ( action == NOPREFFILE_NO )
			exit(1);
#endif
		FSpCreateResFile(&dirspec, 'Pyth', 'pref', 0);
		prefrh = FSpOpenResFile(&dirspec, fsRdWrShPerm);
		if ( prefrh == -1 ) {
			/* This "cannot happen":-) */
			printf("Cannot create preferences file, error %d\n", ResError());
			exit(1);
		}
		if ( (err=PyMac_init_process_location()) != 0 ) {
			printf("Cannot get application location, error %d\n", err);
			exit(1);
		}
		dirspec = PyMac_ApplicationFSSpec;
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
	static int diditbefore = 0;
	static char name[256] = {':', '\0'};
	AliasHandle handle;
	FSSpec dirspec;
	Boolean modified = 0;
	short oldrh, prefrh = -1, homerh;
	
	if ( diditbefore )
		return name;
		
	oldrh = CurResFile();

	/* First look for an override in the application file */
	UseResFile(PyMac_AppRefNum);
	handle = (AliasHandle)Get1Resource('alis', PYTHONHOMEOVERRIDE_ID);
	UseResFile(oldrh);
	if ( handle != NULL ) {
		homerh = PyMac_AppRefNum;
	} else {   
		/* Try to open preferences file in the preferences folder. */
		prefrh = PyMac_OpenPrefFile();
		handle = (AliasHandle)Get1Resource('alis', PYTHONHOME_ID);
		if ( handle == NULL ) {
			/* (void)StopAlert(BADPREFFILE_ID, NULL); */
			diditbefore=1;
			return ":";
		}
		homerh = prefrh;
	}
	/* It exists. Resolve it (possibly updating it) */
	if ( ResolveAlias(NULL, handle, &dirspec, &modified) != noErr ) {
		(void)StopAlert(BADPREFFILE_ID, NULL);
		diditbefore=1;
		return ":";
	}
	if ( modified ) {
   		ChangedResource((Handle)handle);
		UpdateResFile(homerh);
	}
	if ( prefrh != -1 ) CloseResFile(prefrh);
	UseResFile(oldrh);

   	if ( PyMac_GetFullPath(&dirspec, name) == 0 ) {
   		strcat(name, ":");
	} else {
 		/* If all fails, we return the current directory */
   		printf("Python home dir exists but I cannot find the pathname!!\n");
		name[0] = 0;
		(void)getwd(name);
	}
	diditbefore = 1;
	return name;
}

#ifndef USE_BUILTIN_PATH
char *
PyMac_GetPythonPath(void)
{
	short oldrh, prefrh = -1;
	char *rv;
	int i, newlen;
	Str255 pathitem;
	int resource_id;
	OSErr err;
	Handle h;
	
	oldrh = CurResFile();
	/*
	** This is a bit tricky. We check here whether the application file
	** contains an override. This is to forestall us finding another STR# resource
	** with "our" id and using that for path initialization
	*/
	UseResFile(PyMac_AppRefNum);
	SetResLoad(0);
	if ( (h=Get1Resource('STR#', PYTHONPATHOVERRIDE_ID)) ) {
		ReleaseResource(h);
		resource_id = PYTHONPATHOVERRIDE_ID;
	} else {
		resource_id = PYTHONPATH_ID;
	}
	SetResLoad(1);
	UseResFile(oldrh);
	
	/* Open the preferences file only if there is no override */
	if ( resource_id != PYTHONPATHOVERRIDE_ID )
		prefrh = PyMac_OpenPrefFile();
	/* At this point, we may or may not have the preferences file open, and it
	** may or may not contain a sys.path STR# resource. We don't care, if it doesn't
	** exist we use the one from the application (the default).
	** We put an initial '\n' in front of the path that we don't return to the caller
	*/
	if( (rv = malloc(2)) == NULL )
		goto out;
	strcpy(rv, "\n");

	for(i=1; ; i++) {
		GetIndString(pathitem, resource_id, i);
		if( pathitem[0] == 0 )
			break;
		if ( pathitem[0] >= 9 && strncmp((char *)pathitem+1, "$(PYTHON)", 9) == 0 ) {
			/* We have to put the directory in place */
			char *dir = PyMac_GetPythonDir();
			
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
		} else if ( pathitem[0] >= 14 && strncmp((char *)pathitem+1, "$(APPLICATION)", 14) == 0 ) {
			/* This is the application itself */
			
			if ( (err=PyMac_init_process_location()) != 0 ) {
				printf("Cannot get  application location, error %d\n", err);
				exit(1);
			}

			newlen = strlen(rv) + strlen(PyMac_ApplicationPath) + 2;
			if( (rv=realloc(rv, newlen)) == NULL)
				goto out;
			strcpy(rv+strlen(rv), PyMac_ApplicationPath);
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
	if ( prefrh != -1) CloseResFile(prefrh);
	UseResFile(oldrh);
	return rv;
}
#endif /* !USE_BUILTIN_PATH */

void
PyMac_PreferenceOptions(PyMac_PrefRecord *pr)
{
	short oldrh, prefrh = -1;
	Handle handle;
	int size;
	PyMac_PrefRecord *p;
	int action;
	
	
	oldrh = CurResFile();
	
	/* Attempt to load overrides from application */
	UseResFile(PyMac_AppRefNum);
	handle = Get1Resource('Popt', PYTHONOPTIONSOVERRIDE_ID);
	UseResFile(oldrh);
	
	/* Otherwise get options from prefs file or any other open resource file */
	if ( handle == NULL ) {
		prefrh = PyMac_OpenPrefFile();
		handle = GetResource('Popt', PYTHONOPTIONS_ID);
	}
	if ( handle == NULL ) {
		return;
	}
	HLock(handle);
	size = GetHandleSize(handle);
	p = (PyMac_PrefRecord *)*handle;
	if ( p->version == POPT_VERSION_CURRENT && size == sizeof(PyMac_PrefRecord) ) {
		*pr = *p;
	} else {
		action = CautionAlert(BADPREFERENCES_ID, NULL);
		if ( action == BADPREF_DELETE ) {
			OSErr err;
			
			RemoveResource(handle);
			if ( (err=ResError()) ) printf("RemoveResource: %d\n", err);
			if ( prefrh != -1 ) {
				UpdateResFile(prefrh);
				if ( (err=ResError()) ) printf("UpdateResFile: %d\n", err);
			}
		} else if ( action == BADPREF_QUIT )
			exit(1);
	}
	HUnlock(handle);

   	if ( prefrh != -1) CloseResFile(prefrh);
	UseResFile(oldrh);
}

#ifdef USE_GUSI1
void
PyMac_SetGUSIOptions()
{
	Handle h;
	short oldrh, prefrh = -1;
	
	oldrh = CurResFile();
	
	/* Try override from the application resource fork */
	UseResFile(PyMac_AppRefNum);
	h = Get1Resource('GU\267I', GUSIOPTIONSOVERRIDE_ID);
	UseResFile(oldrh);
	
	/* If that didn't work try nonoverride from anywhere */
	if ( h == NULL ) {
		prefrh = PyMac_OpenPrefFile();
		h = GetResource('GU\267I', GUSIOPTIONS_ID);
	}
	if ( h ) GUSILoadConfiguration(h);
   	if ( prefrh != -1) CloseResFile(prefrh);
	UseResFile(oldrh);
}
#endif /* USE_GUSI1 */	
