#ifndef Py_INTERNAL_INSTRUMENTS_H
#define Py_INTERNAL_INSTRUMENTS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef Py_TIMERS

typedef struct timer_records {
    uint64_t main;
    uint64_t runmain;
    uint64_t eval;
    uint64_t malloc;
    uint64_t free;
    uint64_t dealloc;
    uint64_t gc;
    uint64_t import_;
    uint64_t native;
} _PyTimerRecords;

extern _PyTimerRecords _PyTimes;

extern void enter_timer_call(void);
extern void exit_timer_call(uint64_t* time);

#define RECORD_TIME(CALL, NAME) \
    do { \
        enter_timer_call(); \
        CALL; \
        exit_timer_call(&_PyTimes.NAME); \
    } while (0)

#define TIMER_START() _PyTimer_Start()
#define TIMER_STOP_AND_PRINT() do { \
    _PyTimer_Stop(); \
    _Py_PrintTimings(1); \
} while (0)

extern void _Py_PrintTimings(int to_file);

extern void _PyTimer_Start(void);
extern void _PyTimer_Stop(void);

#else

#define RECORD_TIME(CALL, NAME) CALL
#define TIMER_START() ((void)0)
#define TIMER_STOP_AND_PRINT() ((void)0)
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_INSTRUMENTS_H */
