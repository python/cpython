/* OS/2 PM main program - creates a hidden window, and starts Python
 * interpreter in a separate thread, so that Python scripts can be
 * run in PM process space without a console Window.  The interpreter
 * is incorporated by linking in the Python DLL.
 *
 * As it stands, I don't think this is adequate for supporting Python
 * GUI modules, as the Python thread doesn't have its own message
 * queue - which is required of threads that want to create/use
 * PM windows.
 *
 * This code owes a lot to "OS/2 Presentation Manager Programming", by
 * Charles Petzold.
 *
 * Andrew MacIntyre <andymac@bullseye.apana.org.au>, August 2001.
 * Released under the terms of the Python 2.1.1 licence - see the LICENCE
 * file in the Python v2.1.1 (or later) source distribution.
 * Copyright assigned to the Python Software Foundation, 2001.
 */

#define INCL_DOS
#define INCL_WIN
#include <os2.h>
#include <process.h>

#include "Python.h"

/* use structure to pass command line to Python thread */
typedef struct
{
    int argc;
    char **argv;
    HWND Frame;
    int running;
} arglist;

/* make this a global to simplify access.
 * it should only be set from the Python thread, or by the code that
 * initiates the Python thread when the thread cannot be created.
 */
int PythonRC;

extern DL_EXPORT(int) Py_Main(int, char **);
void PythonThread(void *);

int
main(int argc, char **argv)
{
    ULONG FrameFlags = FCF_TITLEBAR |
                       FCF_SYSMENU |
                       FCF_SIZEBORDER |
                       FCF_HIDEBUTTON |
                       FCF_SHELLPOSITION |
                       FCF_TASKLIST;
    HAB hab;
    HMQ hmq;
    HWND Client;
    QMSG qmsg;
    arglist args;
    int python_tid;

    /* init PM and create message queue */
    hab = WinInitialize(0);
    hmq = WinCreateMsgQueue(hab, 0);

    /* create a (hidden) Window to house the window procedure */
    args.Frame = WinCreateStdWindow(HWND_DESKTOP,
                                    0,
                                    &FrameFlags,
                                    NULL,
                                    "PythonPM",
                                    0L,
                                    0,
                                    0,
                                    &Client);

    /* run Python interpreter in a thread */
    args.argc = argc;
    args.argv = argv;
    args.running = 0;
    if (-1 == (python_tid = _beginthread(PythonThread, NULL, 1024 * 1024, &args)))
    {
        /* couldn't start thread */
        WinAlarm(HWND_DESKTOP, WA_ERROR);
        PythonRC = 1;
    }
    else
    {
        /* process PM messages, until Python exits */
        while (WinGetMsg(hab, &qmsg, NULLHANDLE, 0, 0))
            WinDispatchMsg(hab, &qmsg);
        if (args.running > 0)
            DosKillThread(python_tid);
    }

    /* destroy window, shutdown message queue and PM */
    WinDestroyWindow(args.Frame);
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);

    return PythonRC;
}

void PythonThread(void *argl)
{
    HAB hab;
    arglist *args;

    /* PM initialisation */
    hab = WinInitialize(0);

    /* start Python */
    args = (arglist *)argl;
    args->running = 1;
    PythonRC = Py_Main(args->argc, args->argv);

    /* enter a critical section and send the termination message */
    DosEnterCritSec();
    args->running = 0;
    WinPostMsg(args->Frame, WM_QUIT, NULL, NULL);

    /* shutdown PM and terminate thread */
    WinTerminate(hab);
    _endthread();
}
