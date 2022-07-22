#ifndef Py_PTRHEAD_STUBS_H
#define Py_PTRHEAD_STUBS_H

#ifdef HAVE_PTHREAD_STUBS

// WASI's bits/alltypes.h provides type definitions when __NEED_ is set.
// The file can be included multiple times.
#ifdef __wasi__
#  define __NEED_pthread_cond_t 1
#  define __NEED_pthread_condattr_t 1
#  define __NEED_pthread_mutex_t 1
#  define __NEED_pthread_mutexattr_t 1
#  define __NEED_pthread_key_t 1
#  define __NEED_pthread_t 1
#  define __NEED_pthread_attr_t 1
#  include <bits/alltypes.h>
#else
#  error "HAVE_PTHREAD_STUBS not defined for this platform"
#endif

// mutex
static inline int
pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr) {
    return 0;
}

static inline int
pthread_mutex_destroy(pthread_mutex_t *mutex) {
    return 0;
}

static inline int
pthread_mutex_trylock(pthread_mutex_t *mutex) {
    return 0;
}

static inline int
pthread_mutex_lock(pthread_mutex_t *mutex) {
    return 0;
}

static inline int
pthread_mutex_unlock(pthread_mutex_t *mutex) {
    return 0;
}

// condition
static inline int
pthread_condattr_init(pthread_condattr_t *attr) {
   return 0;
}

static inline int
pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr) {
   return 0;
}

static inline int
pthread_cond_destroy(pthread_cond_t *cond) {
    return 0;
}

static inline int
pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id) {
   return 0;
}

static inline int
pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex) {
    return 0;
}

static inline int
pthread_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict abstime) {
    return 0;
}

static inline int
pthread_cond_signal(pthread_cond_t *cond) {
    return 0;
}

// pthread attr
static inline int
pthread_attr_init(pthread_attr_t *attr)
{
    return 0;
}

static inline int
pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    return 0;
}

static inline int
pthread_attr_destroy(pthread_attr_t *attr)
{
    return 0;
}

// pthread

static inline int
pthread_create(pthread_t *restrict thread, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg)
{
    errno = EAGAIN;
    return -1;
}

static inline int
pthread_detach(pthread_t thread)
{
    errno = ESRCH;
    return -1;
}

static inline pthread_t
pthread_self(void)
{
    return (pthread_t)1;
}

static inline _Noreturn void
pthread_exit(void *retval)
{
    exit(0);
}

// key, implemented in thread_pthread.h
int pthread_key_create(pthread_key_t *out, void (*destructor)(void *a));
int pthread_key_delete(pthread_key_t key);
void* pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

#endif // HAVE_PTHREAD_STUBS

#endif // Py_PTRHEAD_STUBS_H
