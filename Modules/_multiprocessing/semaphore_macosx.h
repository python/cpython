#ifndef SEMAPHORE_MACOSX_H
#define SEMAPHORE_MACOSX_H

#include <unistd.h>     // sysconf(SC_PAGESIZE)
#include <sys/sysctl.h>  // sysctlbyname

/*
Structures and constants in shared memory
*/

#define SIZE_SEM_NAME   16
#define SIZE_MUTEX_NAME (SIZE_SEM_NAME<<1)  /* "/mp-xxxxxxxx" (12) + "-gh125828" */

typedef struct {
    int n_semlocks;      // Current number of semaphores. Starts 0.
    int n_slots;         // Current slots in the counter array.
    int size_shm;        // Size of allocated shared memory (this and N counters).
    int sizeof_counter;  // Size of CounterObject.
} HeaderObject;

typedef struct {
    char sem_name[SIZE_SEM_NAME];   // Name of semaphore.
    short internal_value;           // Internal value of semaphore, update on each acquire/release.
    short pending_acquires;         // Threads in sem_wait not yet decremented internal_value.
    short unlink;                   // Indicate if already unlink.
#ifdef Py_REF_DEBUG
    time_t ctimestamp;              // Created timestamp (debug log).
#endif
} CounterObject;

/*
Structure, constants and macros of static memory:
*/

#define SC_PAGESIZE         sysconf(_SC_PAGESIZE)
#define CALC_SIZE_SHM       (shm_semlock_counters.max_open_sems * sizeof(CounterObject)) + sizeof(HeaderObject)
#define ALIGN_SHM_PAGE(n)   ((int)((n)/SC_PAGESIZE)+1)*SC_PAGESIZE

#define SHAREDMEM_NAME      "/psm-gh125828-"
#define SHMLOCK_NAME        "/mp-gh125828-"
#define MASK_MUTEX_NAME     "%s-gh125828"

#define SIZE_SHM_NAME      (SIZE_SEM_NAME<<1)  /* "/psm-gh125828-" (14) + 6 digits PID + '\0' */
#define SIZE_SHMLOCK_NAME  (SIZE_SEM_NAME<<1)  /* "/mp-gh125828-"  (13) + 6 digits PID + '\0' */

typedef int MEMORY_HANDLE;

struct _CountersWorkaround{
    /*-- global datas --*/
    char shm_name[SIZE_SHM_NAME];
    MEMORY_HANDLE handle_shm; // Memory handle.
    char shmlock_name[SIZE_SHMLOCK_NAME];
    SEM_HANDLE handle_shmlock; // Global memory lock to handle shared memory.
    int max_open_sems;              // Max number of opened semaphores.
    /*-- Pointers to shared memory --*/
    HeaderObject *header;     // Pointer to header (shared memory).
    CounterObject*counters;   // Pointer to the first item of fixed array (shared memory).
};

/*
lock and mutexes macros
*/

#define EXIST_SHMLOCK     (exist_lock(shm_semlock_counters.handle_shmlock) == 1)
#define ACQUIRE_SHMLOCK   (acquire_lock(shm_semlock_counters.handle_shmlock) == 0)
#define RELEASE_SHMLOCK   (release_lock(shm_semlock_counters.handle_shmlock) == 0)

#define ACQUIRE_COUNTER_MUTEX(h) (acquire_lock((h)) == 0)
#define RELEASE_COUNTER_MUTEX(h) (release_lock((h)) == 0)

#endif /* SEMAPHORE_MACOSX_H */
