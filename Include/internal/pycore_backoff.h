
#ifndef Py_INTERNAL_BACKOFF_H
#define Py_INTERNAL_BACKOFF_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <assert.h>
#include <stdbool.h>
#include "pycore_structs.h"       // _Py_BackoffCounter

/* 16-bit countdown counters using exponential backoff.

   These are used by the adaptive specializer to count down until
   it is time to specialize an instruction. If specialization fails
   the counter is reset using exponential backoff.

   Another use is for the Tier 2 optimizer to decide when to create
   a new Tier 2 trace (executor). Again, exponential backoff is used.

   The 16-bit counter is structured as a 12-bit unsigned 'value'
   and a 4-bit 'backoff' field. When resetting the counter, the
   backoff field is incremented (until it reaches a limit) and the
   value is set to a bit mask representing the value 2**backoff - 1.
   The maximum backoff is 12 (the number of bits in the value).

   There is an exceptional value which must not be updated, 0xFFFF.
*/

#define BACKOFF_BITS 4
#define MAX_BACKOFF 12
#define UNREACHABLE_BACKOFF 15

static inline bool
is_unreachable_backoff_counter(_Py_BackoffCounter counter)
{
    return counter.value_and_backoff == UNREACHABLE_BACKOFF;
}

static inline _Py_BackoffCounter
make_backoff_counter(uint16_t value, uint16_t backoff)
{
    assert(backoff <= 15);
    assert(value <= 0xFFF);
    _Py_BackoffCounter result;
    result.value_and_backoff = (value << BACKOFF_BITS) | backoff;
    return result;
}

static inline _Py_BackoffCounter
forge_backoff_counter(uint16_t counter)
{
    _Py_BackoffCounter result;
    result.value_and_backoff = counter;
    return result;
}

static inline _Py_BackoffCounter
restart_backoff_counter(_Py_BackoffCounter counter)
{
    assert(!is_unreachable_backoff_counter(counter));
    int backoff = counter.value_and_backoff & 15;
    if (backoff < MAX_BACKOFF) {
        return make_backoff_counter((1 << (backoff + 1)) - 1, backoff + 1);
    }
    else {
        return make_backoff_counter((1 << MAX_BACKOFF) - 1, MAX_BACKOFF);
    }
}

static inline _Py_BackoffCounter
pause_backoff_counter(_Py_BackoffCounter counter)
{
    _Py_BackoffCounter result;
    result.value_and_backoff = counter.value_and_backoff | (1 << BACKOFF_BITS);
    return result;
}

static inline _Py_BackoffCounter
advance_backoff_counter(_Py_BackoffCounter counter)
{
    _Py_BackoffCounter result;
    result.value_and_backoff = counter.value_and_backoff - (1 << BACKOFF_BITS);
    return result;
}

static inline bool
backoff_counter_triggers(_Py_BackoffCounter counter)
{
    /* Test whether the value is zero and the backoff is not UNREACHABLE_BACKOFF */
    return counter.value_and_backoff < UNREACHABLE_BACKOFF;
}

/* Initial JUMP_BACKWARD counter.
 * This determines when we create a trace for a loop. */
#define JUMP_BACKWARD_INITIAL_VALUE 4095
#define JUMP_BACKWARD_INITIAL_BACKOFF 12
static inline _Py_BackoffCounter
initial_jump_backoff_counter(void)
{
    return make_backoff_counter(JUMP_BACKWARD_INITIAL_VALUE,
                                JUMP_BACKWARD_INITIAL_BACKOFF);
}

/* Initial exit temperature.
 * Must be larger than ADAPTIVE_COOLDOWN_VALUE,
 * otherwise when a side exit warms up we may construct
 * a new trace before the Tier 1 code has properly re-specialized. */
#define SIDE_EXIT_INITIAL_VALUE 4095
#define SIDE_EXIT_INITIAL_BACKOFF 12

static inline _Py_BackoffCounter
initial_temperature_backoff_counter(void)
{
    return make_backoff_counter(SIDE_EXIT_INITIAL_VALUE,
                                SIDE_EXIT_INITIAL_BACKOFF);
}

/* Unreachable backoff counter. */
static inline _Py_BackoffCounter
initial_unreachable_backoff_counter(void)
{
    return make_backoff_counter(0, UNREACHABLE_BACKOFF);
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BACKOFF_H */
