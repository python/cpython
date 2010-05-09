#include "oslib/osmodule.h"
#include <stdio.h>
#include "kernel.h"
#include <limits.h>
#include <errno.h>
#include "oslib/taskwindow.h"
#include "Python.h"


int riscos_sleep(double delay)
{
    os_t starttime, endtime, time; /* monotonic times (centiseconds) */
    int *pollword, ret;
    osbool claimed;

    /* calculate end time */
    starttime = os_read_monotonic_time();
    if (starttime + 100.0*delay >INT_MAX)
        endtime = INT_MAX;
    else
        endtime = (os_t)(starttime + 100.0*delay);

    /* allocate (in RMA) and set pollword for xupcall_sleep */
    pollword = osmodule_alloc(4);
    *pollword = 1;

    time = starttime;
    ret = 0;
    while ( time<endtime && time>=starttime ) {
        xupcall_sleep (pollword, &claimed);
        if (PyErr_CheckSignals()) {
            ret = 1;
            break;
        }
        time = os_read_monotonic_time();
    }

    /* deallocate pollword */
    osmodule_free(pollword);
    return ret;
}
