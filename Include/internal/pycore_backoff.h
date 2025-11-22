
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

   The 16-bit counter is structured as a 13-bit unsigned 'value'
   and a 3-bit 'backoff' field. When resetting the counter, the
   backoff field is incremented (until it reaches a limit) and the
   value is set to a bit mask representing some prime value - 1.
   New values and backoffs for each backoff are calculated once
   at compile time and saved to value_and_backoff_next table.
   The maximum backoff is 6, since 7 is an UNREACHABLE_BACKOFF.

   There is an exceptional value which must not be updated, 0xFFFF.
*/

#define BACKOFF_BITS 3
#define BACKOFF_MASK 7
#define MAX_BACKOFF 6
#define UNREACHABLE_BACKOFF 7
#define MAX_VALUE 0x1FFF

#define MAKE_VALUE_AND_BACKOFF(value, backoff) \
    ((value << BACKOFF_BITS) | backoff)

// For previous backoff b we use value x such that
// x + 1 is near to 2**(2*b+1) and x + 1 is prime.
static const uint16_t value_and_backoff_next[] = {
    MAKE_VALUE_AND_BACKOFF(1, 1),
    MAKE_VALUE_AND_BACKOFF(6, 2),
    MAKE_VALUE_AND_BACKOFF(30, 3),
    MAKE_VALUE_AND_BACKOFF(126, 4),
    MAKE_VALUE_AND_BACKOFF(508, 5),
    MAKE_VALUE_AND_BACKOFF(2052, 6),
    // We use the same backoff counter for all backoffs >= MAX_BACKOFF.
    MAKE_VALUE_AND_BACKOFF(8190, 6),
    MAKE_VALUE_AND_BACKOFF(8190, 6),
};

static inline _Py_BackoffCounter
make_backoff_counter(uint16_t value, uint16_t backoff)
{
    assert(backoff <= UNREACHABLE_BACKOFF);
    assert(value <= MAX_VALUE);
    return ((_Py_BackoffCounter){
        .value_and_backoff = MAKE_VALUE_AND_BACKOFF(value, backoff)
    });
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
    uint16_t backoff = counter.value_and_backoff & BACKOFF_MASK;
    assert(backoff <= MAX_BACKOFF);
    return ((_Py_BackoffCounter){
        .value_and_backoff = value_and_backoff_next[backoff]
    });
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

static inline _Py_BackoffCounter
trigger_backoff_counter(void)
{
    _Py_BackoffCounter result;
    result.value_and_backoff = 0;
    return result;
}

// Initial JUMP_BACKWARD counter.
// Must be larger than ADAPTIVE_COOLDOWN_VALUE, otherwise when JIT code is
// invalidated we may construct a new trace before the bytecode has properly
// re-specialized:
// Note: this should be a prime number-1. This increases the likelihood of
// finding a "good" loop iteration to trace.
// For example, 4095 does not work for the nqueens benchmark on pyperformance
// as we always end up tracing the loop iteration's
// exhaustion iteration. Which aborts our current tracer.
#define JUMP_BACKWARD_INITIAL_VALUE 4000
#define JUMP_BACKWARD_INITIAL_BACKOFF 6
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
#define SIDE_EXIT_INITIAL_VALUE 4000
#define SIDE_EXIT_INITIAL_BACKOFF 6

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
