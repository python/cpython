#include "cpython/pthread_stubs.h"

// mutex
int
pthread_mutex_init(pthread_mutex_t *restrict mutex,
                   const pthread_mutexattr_t *restrict attr)
{
    return 0;
}

int
pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    return 0;
}

int
pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    return 0;
}

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
    return 0;
}

int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    return 0;
}

// condition
int
pthread_cond_init(pthread_cond_t *restrict cond,
                  const pthread_condattr_t *restrict attr)
{
    return 0;
}

PyAPI_FUNC(int)pthread_cond_destroy(pthread_cond_t *cond)
{
    return 0;
}

int
pthread_cond_wait(pthread_cond_t *restrict cond,
                  pthread_mutex_t *restrict mutex)
{
    return 0;
}

int
pthread_cond_timedwait(pthread_cond_t *restrict cond,
                       pthread_mutex_t *restrict mutex,
                       const struct timespec *restrict abstime)
{
    return 0;
}

int
pthread_cond_signal(pthread_cond_t *cond)
{
    return 0;
}

int
pthread_condattr_init(pthread_condattr_t *attr)
{
    return 0;
}

int
pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id)
{
    return 0;
}

// pthread
int
pthread_create(pthread_t *restrict thread,
               const pthread_attr_t *restrict attr,
               void *(*start_routine)(void *),
               void *restrict arg)
{
    return EAGAIN;
}

int
pthread_detach(pthread_t thread)
{
    return 0;
}

PyAPI_FUNC(pthread_t) pthread_self(void)
{
    return 0;
}

int
pthread_exit(void *retval)
{
    exit(0);
}

int
pthread_attr_init(pthread_attr_t *attr)
{
    return 0;
}

int
pthread_attr_setstacksize(
    pthread_attr_t *attr, size_t stacksize)
{
    return 0;
}

int
pthread_attr_destroy(pthread_attr_t *attr)
{
    return 0;
}


typedef struct py_stub_tls_entry py_tls_entry;

#define py_tls_entries (_PyRuntime.threads.stubs.tls_entries)

int
pthread_key_create(pthread_key_t *key, void (*destr_function)(void *))
{
    if (!key) {
        return EINVAL;
    }
    if (destr_function != NULL) {
        Py_FatalError("pthread_key_create destructor is not supported");
    }
    for (pthread_key_t idx = 0; idx < PTHREAD_KEYS_MAX; idx++) {
        if (!py_tls_entries[idx].in_use) {
            py_tls_entries[idx].in_use = true;
            *key = idx;
            return 0;
        }
    }
    return EAGAIN;
}

int
pthread_key_delete(pthread_key_t key)
{
    if (key < 0 || key >= PTHREAD_KEYS_MAX || !py_tls_entries[key].in_use) {
        return EINVAL;
    }
    py_tls_entries[key].in_use = false;
    py_tls_entries[key].value = NULL;
    return 0;
}


void *
pthread_getspecific(pthread_key_t key) {
    if (key < 0 || key >= PTHREAD_KEYS_MAX || !py_tls_entries[key].in_use) {
        return NULL;
    }
    return py_tls_entries[key].value;
}

int
pthread_setspecific(pthread_key_t key, const void *value)
{
    if (key < 0 || key >= PTHREAD_KEYS_MAX || !py_tls_entries[key].in_use) {
        return EINVAL;
    }
    py_tls_entries[key].value = (void *)value;
    return 0;
}

// let thread_pthread define the Python API
#include "thread_pthread.h"
