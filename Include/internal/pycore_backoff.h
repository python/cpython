
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

typedef struct {
    union {
        uint16_t counter;
        struct {
            uint16_t value : 12;
            uint16_t backoff : 4;
        };
    };
} backoff_counter_t;

static_assert(sizeof(backoff_counter_t) == 2, "backoff counter size should be 2 bytes");

#define UNREACHABLE_BACKOFF 0xFFFF
#define UNREACHABLE_BACKOFF_COUNTER ((backoff_counter_t){.counter = UNREACHABLE_BACKOFF})

/* Alias used by optimizer */
#define OPTIMIZER_UNREACHABLE_THRESHOLD UNREACHABLE_BACKOFF

static inline bool
is_unreachable_backoff_counter(backoff_counter_t counter)
{
    return counter.counter == 0xFFFF;
}

static inline backoff_counter_t
make_backoff_counter(uint16_t value, uint16_t backoff)
{
    assert(backoff <= 12);
    assert(value <= 0xFFF);
    return (backoff_counter_t){.value = value, .backoff = backoff};
}

static inline backoff_counter_t
forge_backoff_counter(uint16_t counter)
{
    return (backoff_counter_t){.counter = counter};
}

static inline backoff_counter_t
reset_backoff_counter(backoff_counter_t counter)
{
    assert(!is_unreachable_backoff_counter(counter));
    if (counter.backoff < 12) {
        return make_backoff_counter((1 << (counter.backoff + 1)) - 1, counter.backoff + 1);
    }
    else {
        return make_backoff_counter((1 << 12) - 1, 12);
    }
}

static inline backoff_counter_t
pause_backoff_counter(backoff_counter_t counter)
{
    return make_backoff_counter(counter.value | 1, counter.backoff);
}

static inline backoff_counter_t
advance_backoff_counter(backoff_counter_t counter)
{
    if (!is_unreachable_backoff_counter(counter)) {
        assert(counter.value != 0);
        return make_backoff_counter(counter.value - 1, counter.backoff);
    }
    else {
        return counter;
    }
}

static inline bool
backoff_counter_is_zero(backoff_counter_t counter)
{
    return counter.value == 0;
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BACKOFF_H */
