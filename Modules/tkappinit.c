/* appinit.c -- Tcl and Tk application initialization. */

#include <tcl.h>
#include <tk.h>

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

#ifdef WITH_XXX

#endif
	return TCL_OK;
}
