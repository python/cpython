#ifndef SEMAPHORE_MACOSX_H
#define SEMAPHORE_MACOSX_H

#include <unistd.h>     // sysconf(SC_PAGESIZE)
#include <sys/mman.h>   // shm_open, shm_unlink

/*
On my MacOSX m4 pro, sysconf(_SC_SEM_NSEM_MAX) returns 87381.
Perharps, this value is to high ?
*/
#define NSEMS_MAX               sysconf(_SC_SEM_NSEMS_MAX)

#define CALC_SIZE_SHM           (NSEMS_MAX * sizeof(CounterObject)) + sizeof(HeaderObject);

#define SC_PAGESIZE             sysconf(_SC_PAGESIZE)
#define ALIGN_SHM_PAGE(s)       ((int)((s)/SC_PAGESIZE)+1)*SC_PAGESIZE

#define CALC_NB_SLOTS(s)        (int)((((s)) - sizeof(HeaderObject)) / sizeof(CounterObject))

/*
Structure in shared memory
*/
typedef struct {
    int n_semlocks;   // Current number of semaphores. Starts 0.
    int n_slots;      // Current slots in the counter array.
    int size_shm;     // Size of allocated shared memory (this and N counters).
    int n_procs;      // Number of attached processes (Used to check).
} HeaderObject;

typedef struct {
    char sem_name[16];  // Name of semaphore.
    int internal_value; // Internal value of semaphore, update on each acquire/release.
    int unlink_done;    // Can reset counter if unlink is done.
    time_t ctimestamp;  // Created timestamp.
} CounterObject;

/*
2 -> Structure of static memory:
*/

typedef int MEMORY_HANDLE;
enum _state {THIS_NOT_OPEN, THIS_AVAILABLE, THIS_CLOSED};

typedef struct {
    /*-- global datas --*/
    int state_this;           // State of this structure.
    char *name_shm;
    MEMORY_HANDLE handle_shm; // Memory handle.
    int create_shm;           // Did I create this shared memory ?
    char *name_shm_lock;
    SEM_HANDLE handle_shm_lock; // Global memory lock to handle shared memory.
    /*-- Pointers to shared memory --*/
    HeaderObject *header;     // Pointer to header (shared memory).
    CounterObject*counters;   // Pointer to first item of fix array (shared memory).
}  CountersWorkaround;

#define ACQUIRE_SHM_LOCK     (acquire_lock(shm_semlock_counters.handle_shm_lock) >= 0)
#define RELEASE_SHM_LOCK     (release_lock(shm_semlock_counters.handle_shm_lock) >= 0)

#define ACQUIRE_COUNTER_MUTEX(s) (acquire_lock((s)) >= 0)
#define RELEASE_COUNTER_MUTEX(s) (release_lock((s)) >= 0)

#define ISSEMAPHORE2(m, k) ((m) > 1 && (k) == SEMAPHORE)
#define ISSEMAPHORE(o) ((o)->maxvalue > 1 && (o)->kind == SEMAPHORE)

#define NO_VALUE (-11111111)

#endif /* SEMAPHORE_MACOSX_H */
