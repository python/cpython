#include <Python.h>
#include <stdio.h>

void print_subinterp(void)
{
    /* Just output some debug stuff */
    PyThreadState *ts = PyThreadState_Get();
    printf("interp %p, thread state %p: ", ts->interp, ts);
    fflush(stdout);
    PyRun_SimpleString(
        "import sys;"
        "print('id(modules) =', id(sys.modules));"
        "sys.stdout.flush()"
    );
}

int main(int argc, char *argv[])
{
    PyThreadState *mainstate, *substate;
    PyGILState_STATE gilstate;
    int i, j;

    for (i=0; i<3; i++) {
        printf("--- Pass %d ---\n", i);
        /* HACK: the "./" at front avoids a search along the PATH in
           Modules/getpath.c */
        Py_SetProgramName(L"./_testembed");
        Py_Initialize();
        mainstate = PyThreadState_Get();

        PyEval_InitThreads();
        PyEval_ReleaseThread(mainstate);

        gilstate = PyGILState_Ensure();
        print_subinterp();
        PyThreadState_Swap(NULL);

        for (j=0; j<3; j++) {
            substate = Py_NewInterpreter();
            print_subinterp();
            Py_EndInterpreter(substate);
        }

        PyThreadState_Swap(mainstate);
        print_subinterp();
        PyGILState_Release(gilstate);

        PyEval_RestoreThread(mainstate);
        Py_Finalize();
    }
    return 0;
}
