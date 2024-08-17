
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
#include <stdint.h>


typedef struct {
    union {
        struct {
            uint16_t backoff : 4;
            uint16_t value : 12;
        };
        uint16_t as_counter;  // For printf("%#x", ...)
    };
} _Py_BackoffCounter;


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
   The maximum backoff is 12 (the number of value bits).

   There is an exceptional value which must not be updated, 0xFFFF.
*/

#define UNREACHABLE_BACKOFF 0xFFFF

static inline bool
is_unreachable_backoff_counter(_Py_BackoffCounter counter)
{
    return counter.as_counter == UNREACHABLE_BACKOFF;
}

static inline _Py_BackoffCounter
make_backoff_counter(uint16_t value, uint16_t backoff)
{
    assert(backoff <= 15);
    assert(value <= 0xFFF);
    _Py_BackoffCounter result;
    result.value = value;
    result.backoff = backoff;
    return result;
}

static inline _Py_BackoffCounter
forge_backoff_counter(uint16_t counter)
{
    _Py_BackoffCounter result;
    result.as_counter = counter;
    return result;
}

static inline _Py_BackoffCounter
restart_backoff_counter(_Py_BackoffCounter counter)
{
    assert(!is_unreachable_backoff_counter(counter));
    if (counter.backoff < 12) {
        return make_backoff_counter((1 << (counter.backoff + 1)) - 1, counter.backoff + 1);
    }
    else {
        return make_backoff_counter((1 << 12) - 1, 12);
    }
}

static inline _Py_BackoffCounter
pause_backoff_counter(_Py_BackoffCounter counter)
{
    return make_backoff_counter(counter.value | 1, counter.backoff);
}

static inline _Py_BackoffCounter
advance_backoff_counter(_Py_BackoffCounter counter)
{
    if (!is_unreachable_backoff_counter(counter)) {
        return make_backoff_counter((counter.value - 1) & 0xFFF, counter.backoff);
    }
    else {
        return counter;
    }
}

static inline bool
backoff_counter_triggers(_Py_BackoffCounter counter)
{
    return counter.value == 0;
}

/* Initial JUMP_BACKWARD counter.
 * This determines when we create a trace for a loop.
* Backoff sequence 16, 32, 64, 128, 256, 512, 1024, 2048, 4096. */
#define JUMP_BACKWARD_INITIAL_VALUE 16
#define JUMP_BACKWARD_INITIAL_BACKOFF 4
static inline _Py_BackoffCounter
initial_jump_backoff_counter(void)
{
    return make_backoff_counter(JUMP_BACKWARD_INITIAL_VALUE,
                                JUMP_BACKWARD_INITIAL_BACKOFF);
}

/* Initial exit temperature.
 * Must be larger than ADAPTIVE_COOLDOWN_VALUE,
 * otherwise when a side exit warms up we may construct
 * a new trace before the Tier 1 code has properly re-specialized.
 * Backoff sequence 64, 128, 256, 512, 1024, 2048, 4096. */
#define COLD_EXIT_INITIAL_VALUE 64
#define COLD_EXIT_INITIAL_BACKOFF 6

static inline _Py_BackoffCounter
initial_temperature_backoff_counter(void)
{
    return make_backoff_counter(COLD_EXIT_INITIAL_VALUE,
                                COLD_EXIT_INITIAL_BACKOFF);
}

/* Unreachable backoff counter. */
static inline _Py_BackoffCounter
initial_unreachable_backoff_counter(void)
{
    return forge_backoff_counter(UNREACHABLE_BACKOFF);
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BACKOFF_H */
