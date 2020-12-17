/*
 * tkMacWinMenu.c --
 *
 *	This module implements the common elements of the Mac and Windows
 *	specific features of menus. This file is not used for UNIX.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkMenu.h"

typedef struct ThreadSpecificData {
    int postCommandGeneration;
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

static int		PreprocessMenu(TkMenu *menuPtr);

/*
 *----------------------------------------------------------------------
 *
 * PreprocessMenu --
 *
 *	The guts of the preprocessing. Recursive.
 *
 * Results:
 *	The return value is a standard Tcl result (errors can occur while the
 *	postcommands are being processed).
 *
 * Side effects:
 *	Since commands can get executed while this routine is being executed,
 *	the entire world can change.
 *
 *----------------------------------------------------------------------
 */

static int
PreprocessMenu(
    TkMenu *menuPtr)
{
    int index, result, finished;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    Tcl_Preserve(menuPtr);

    /*
     * First, let's process the post command on ourselves. If this command
     * destroys this menu, or if there was an error, we are done.
     */

    result = TkPostCommand(menuPtr);
    if ((result != TCL_OK) || (menuPtr->tkwin == NULL)) {
	goto done;
    }

    /*
     * Now, we go through structure and process all of the commands. Since the
     * structure is changing, we stop after we do one command, and start over.
     * When we get through without doing any, we are done.
     */

    do {
	finished = 1;
	for (index = 0; index < menuPtr->numEntries; index++) {
	    register TkMenuEntry *entryPtr = menuPtr->entries[index];

	    if ((entryPtr->type == CASCADE_ENTRY)
		    && (entryPtr->namePtr != NULL)
		    && (entryPtr->childMenuRefPtr != NULL)
		    && (entryPtr->childMenuRefPtr->menuPtr != NULL)) {
		TkMenu *cascadeMenuPtr = entryPtr->childMenuRefPtr->menuPtr;

		if (cascadeMenuPtr->postCommandGeneration !=
			tsdPtr->postCommandGeneration) {
		    cascadeMenuPtr->postCommandGeneration =
			    tsdPtr->postCommandGeneration;
		    result = PreprocessMenu(cascadeMenuPtr);
		    if (result != TCL_OK) {
			goto done;
		    }
		    finished = 0;
		    break;
		}
	    }
	}
    } while (!finished);

  done:
    Tcl_Release(menuPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkPreprocessMenu --
 *
 *	On the Mac and on Windows, all of the postcommand processing has to be
 *	done on the entire tree underneath the main window to be posted. This
 *	means that we have to traverse the menu tree and issue the
 *	postcommands for all of the menus that have cascades attached. Since
 *	the postcommands can change the menu structure while we are
 *	traversing, we have to be extremely careful. Basically, the idea is to
 *	traverse the structure until we succesfully process one postcommand.
 *	Then we start over, and do it again until we traverse the whole
 *	structure without processing any postcommands.
 *
 *	We are also going to set up the cascade back pointers in here since we
 *	have to traverse the entire structure underneath the menu anyway. We
 *	can clear the postcommand marks while we do that.
 *
 * Results:
 *	The return value is a standard Tcl result (errors can occur while the
 *	postcommands are being processed).
 *
 * Side effects:
 *	Since commands can get executed while this routine is being executed,
 *	the entire world can change.
 *
 *----------------------------------------------------------------------
 */

int
TkPreprocessMenu(
    TkMenu *menuPtr)
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    tsdPtr->postCommandGeneration++;
    menuPtr->postCommandGeneration = tsdPtr->postCommandGeneration;
    return PreprocessMenu(menuPtr);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
