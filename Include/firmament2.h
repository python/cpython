#ifndef Py_FIRMAMENT2_H
#define Py_FIRMAMENT2_H
#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h"
#include <pythread.h>
#include <stdint.h>
#include <stdlib.h>   /* getenv */
#include <stdio.h>    /* printf, fflush */
#include <string.h>   /* strlen, memcpy */
#include <stdatomic.h>
#if defined(_WIN32)
  #include <windows.h>
#else
  #include <unistd.h> /* getpid */
  #include <time.h>   /* clock_gettime */
#endif

/* ---------------- Global gate ---------------- */
static inline int _firm2_enabled(void) {
    static int cached = -1;
    if (cached == -1) {
        const char *env = getenv("FIRMAMENT2_ENABLE");
        cached = (env && env[0] != '\0') ? 1 : 0;
    }
    return cached;
}

/* ---------------- Envelope helpers ---------------- */
static inline unsigned long _firm2_pid(void) {
#if defined(_WIN32)
    return (unsigned long)GetCurrentProcessId();
#else
    return (unsigned long)getpid();
#endif
}

static inline unsigned long long _firm2_tid(void) {
    /* PyThread_get_thread_ident() returns a long. Cast to 64-bit. */
    return (unsigned long long)PyThread_get_thread_ident();
}

static inline long long _firm2_now_ns(void) {
#if !defined(_WIN32)
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return ((long long)ts.tv_sec * 1000000000LL) + (long long)ts.tv_nsec;
    }
#endif
    return 0;
}

static inline unsigned long long _firm2_next_eid(void) {
    static _Atomic unsigned long long ctr = 0;
    return ++ctr;  /* monotonic per-process event id */
}

/* ---------------- Per-source (compilation unit) TLS ----------------
 * Implemented in ceval.c so all core files can link against them.
 */
void _firm2_source_begin(const char *filename_utf8);
void _firm2_source_end(void);
const char *_firm2_current_filename(void);
const char *_firm2_current_source_id_hex(void);

#ifdef __cplusplus
}
#endif
#endif /* Py_FIRMAMENT2_H */
