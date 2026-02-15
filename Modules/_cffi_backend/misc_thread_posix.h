/*
  Logic for a better replacement of PyGILState_Ensure().

  This version is ready to handle the case of a non-Python-started
  thread in which we do a large number of calls to CFFI callbacks.  If
  we were to rely on PyGILState_Ensure() for that, we would constantly
  be creating and destroying PyThreadStates---it is slow, and
  PyThreadState_Delete() will actually walk the list of all thread
  states, making it O(n). :-(

  This version only creates one PyThreadState object the first time we
  see a given thread, and keep it alive until the thread is really
  shut down, using a destructor on the tls key.
*/

#include <pthread.h>
#include "misc_thread_common.h"


static pthread_key_t cffi_tls_key;

static void init_cffi_tls(void)
{
    if (pthread_key_create(&cffi_tls_key, &cffi_thread_shutdown) != 0) {
        /* On platforms with limited pthread TLS (e.g. Nanvix),
           silently continue - TLS will be unavailable but basic
           cffi functionality still works. */
    }
}

static struct cffi_tls_s *_make_cffi_tls(void)
{
    void *p = calloc(1, sizeof(struct cffi_tls_s));
    if (p == NULL)
        return NULL;
    if (pthread_setspecific(cffi_tls_key, p) != 0) {
        free(p);
        return NULL;
    }
    return p;
}

static struct cffi_tls_s *get_cffi_tls(void)
{
    void *p = pthread_getspecific(cffi_tls_key);
    if (p == NULL)
        p = _make_cffi_tls();
    return (struct cffi_tls_s *)p;
}

#define save_errno      save_errno_only
#define restore_errno   restore_errno_only
