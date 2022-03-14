#include "Python.h"
#include "pycore_atomic.h"
#include "pycore_instruments.h"

#ifdef Py_TIMERS

/* Each item on the stack holds the accumulated time so far,
 * except for `top` which holds the effective start time for that call.
 * The effective start time is the time of the latest call less the
 * cummulative time of all previous calls.
 */

/* "now" is updated by the timer thread every ~10 000 ns.
 * This will cause the first read of "now" to suffer a L2 cache
 * miss, causing a delay of no more than 50ns, on x86-64 machines.
 * So we expect the overhead of this to be no more than 0.5%.
 * The additional overhead of storing the timing information
 * should also be less than 0.5%, for an overall overhead of < 1%.
 */

typedef struct _timerstack {
    _PyTime_t start;
    _Py_atomic_int stop;
    _Py_atomic_int now;
    int *top;
    int times[1<<16];
} _PyTimerStack;

static _PyTimerStack the_timer = { .top = &the_timer.times[1] };

_PyTimerRecords _PyTimes = { 0 };

#define REFRESH_NANOS 10000

#ifndef HAVE_CLOCK_NANOSLEEP
#error Need a nanosleep implementation
#endif

static struct timespec short_pause = {
        .tv_sec = 0,
        .tv_nsec = REFRESH_NANOS
    };

void sleep_a_bit(void) {
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &short_pause, NULL);
}

static void
run_timer(void *unused)
{
    the_timer.start = _PyTime_GetPerfCounter();
    while (_Py_atomic_load(&the_timer.stop) == 0) {
        sleep_a_bit();
        _PyTime_t diff = _PyTime_GetPerfCounter() - the_timer.start;
        _Py_atomic_store(&the_timer.now, (int)(diff/1000LL));
    }
}

void _PyTimer_Start(void) {
    PyThread_start_new_thread(run_timer, NULL);
    enter_timer_call();
}

void _PyTimer_Stop(void) {
    _Py_atomic_store(&the_timer.stop, 1);
}

void enter_timer_call(void)
{
    int now = _Py_atomic_load(&(the_timer.now));
    *the_timer.top = now - *the_timer.top;
    the_timer.top++;
    *the_timer.top = now;
}

extern void exit_timer_call(uint64_t* time)
{
    int now = _Py_atomic_load(&(the_timer.now));
    *time += (now - *the_timer.top);
    the_timer.top--;
    *the_timer.top = now - *the_timer.top;
}

#define PRINT_TIME(field) \
    fprintf(out, #field ": %" PRIu64 "\n", times->field);

static void
print_times(FILE *out, _PyTimerRecords *times)
{
    PRINT_TIME(main);
    PRINT_TIME(eval);
    PRINT_TIME(malloc);
    PRINT_TIME(free);
    PRINT_TIME(dealloc);
    PRINT_TIME(gc);
    PRINT_TIME(import_);
    PRINT_TIME(native);
}

void
_Py_PrintTimings(int to_file)
{
    FILE *out = stderr;
    if (to_file) {
        /* Write to a file instead of stderr. */
# ifdef MS_WINDOWS
        const char *dirname = "c:\\temp\\py_timings\\";
# else
        const char *dirname = "/tmp/py_timings/";
# endif
        /* Use random 160 bit number as file name,
        * to avoid both accidental collisions and
        * symlink attacks. */
        unsigned char rand[20];
        char hex_name[41];
        _PyOS_URandomNonblockNoRaise(rand, 20);
        for (int i = 0; i < 20; i++) {
            hex_name[2*i] = "0123456789abcdef"[rand[i]&15];
            hex_name[2*i+1] = "0123456789abcdef"[(rand[i]>>4)&15];
        }
        hex_name[40] = '\0';
        char buf[64];
        assert(strlen(dirname) + 40 + strlen(".txt") < 64);
        sprintf(buf, "%s%s.txt", dirname, hex_name);
        FILE *fout = fopen(buf, "w");
        if (fout) {
            out = fout;
        }
    }
    else {
        fprintf(out, "Timings:\n");
    }
    print_times(out, &_PyTimes);
    if (out != stderr) {
        fclose(out);
    }
}
#endif
