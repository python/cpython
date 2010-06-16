#include "Python.h"
#include "_time.h"

/* Exposed in timefuncs.h. */
time_t
_PyTime_DoubleToTimet(double x)
{
    time_t result;
    double diff;

    result = (time_t)x;
    /* How much info did we lose?  time_t may be an integral or
     * floating type, and we don't know which.  If it's integral,
     * we don't know whether C truncates, rounds, returns the floor,
     * etc.  If we lost a second or more, the C rounding is
     * unreasonable, or the input just doesn't fit in a time_t;
     * call it an error regardless.  Note that the original cast to
     * time_t can cause a C error too, but nothing we can do to
     * worm around that.
     */
    diff = x - (double)result;
    if (diff <= -1.0 || diff >= 1.0) {
        PyErr_SetString(PyExc_ValueError,
                        "timestamp out of range for platform time_t");
        result = (time_t)-1;
    }
    return result;
}
