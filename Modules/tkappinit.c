/* appinit.c -- Tcl and Tk application initialization. */

#include <tcl.h>
#include <tk.h>

#ifdef WITH_BLT
#include "blt.h"
#endif

int
Tcl_AppInit (interp)
	Tcl_Interp *interp;
{
	Tk_Window main;

	main = Tk_MainWindow(interp);

	if (Tcl_Init (interp) == TCL_ERROR)
		return TCL_ERROR;
	if (Tk_Init (interp) == TCL_ERROR)
		return TCL_ERROR;

#ifdef WITH_MOREBUTTONS
	{
		extern Tcl_CmdProc studButtonCmd;
		extern Tcl_CmdProc triButtonCmd;

		Tcl_CreateCommand(interp, "studbutton", studButtonCmd,
				  (ClientData) main, NULL);
		Tcl_CreateCommand(interp, "tributton", triButtonCmd,
				  (ClientData) main, NULL);
	}
#endif

#ifdef WITH_PIL /* 0.2b5 and later -- not yet released as of May 14 */
	{
		extern void TkImaging_Init(Tcl_Interp *interp);
		TkImaging_Init(interp);
	}
#endif

#ifdef WITH_PIL_OLD /* 0.2b4 and earlier */
	{
		extern void TkImaging_Init(void);
		TkImaging_Init();
	}
#endif

#ifdef WITH_TIX
	if (Tix_Init (interp) == TCL_ERROR) {
		fprintf(stderr, "Tix_Init error: #s\n", interp->result);
		return TCL_ERROR;
	}
#endif

#ifdef WITH_BLT
	if (Blt_Init(interp) != TCL_OK) {
		fprintf(stderr, "BLT_Init error: #s\n", interp->result);
		return TCL_ERROR;
	}
	Tcl_StaticPackage(interp, "Blt", Blt_Init, Blt_SafeInit);
#endif

#ifdef WITH_XXX

#endif
	return TCL_OK;
}
