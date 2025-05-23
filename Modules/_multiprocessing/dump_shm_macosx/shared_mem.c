#include <fcntl.h>      /* O_CREAT and O_EXCL */
#include <signal.h>     // signal
#include <stdio.h>      // printf, puts
#include <stdlib.h>     // atexit
#include <sys/errno.h>  // errno
#include <unistd.h>     // sysconf

#include <semaphore.h> // sem_open
typedef sem_t *SEM_HANDLE;

#include "../semaphore_macosx.h"
#include "shared_mem.h"

void sigterm(int code) {
    exit(EXIT_SUCCESS);
}

int acquire_lock(SEM_HANDLE sem) {
    sem_wait(sem);
    return 1;
}

int release_lock(SEM_HANDLE sem) {
    sem_post(sem);
    return 1;
}

void connect_shm_semlock_counters(int unlink, int force_open, int call_release_lock) {
puts(__func__);

    int oflag = O_RDWR;
    int shm = -1;
    int res = -1;
    SEM_HANDLE sem = SEM_FAILED;
    long size_shm_init = CALC_SIZE_SHM;
    long size_shm = ALIGN_SHM_PAGE(size_shm_init);

    // printf("size1: %lu vs size2:%lu\n", size_shm_init, size_shm);

    // Install signals.
    signal(SIGTERM, &sigterm);
    signal(SIGINT, &sigterm);

    errno = 0;
    if (sem == SEM_FAILED) {
        errno = 0;
        // Semaphore exists, just opens it.
        sem = sem_open(shm_semlock_counters.name_shm_lock, 0);
        // Not exists, creates it.
        if (force_open && sem == SEM_FAILED) {
            sem = sem_open(shm_semlock_counters.name_shm_lock, O_CREAT, 0600, 1);
        }
    }
    printf("sem:%p\n", sem);
    shm_semlock_counters.handle_shm_lock = sem;

    if (call_release_lock) {
        RELEASE_SHM_LOCK;
    }

    // Locks to semaphore.
    if (sem != SEM_FAILED && ACQUIRE_SHM_LOCK) {
        printf("Shm Lock ok on %p\n", sem);
        // connect to Shared mem
        shm = shm_open(shm_semlock_counters.name_shm, oflag, 0);
        if (shm != -1) {
            shm_semlock_counters.handle_shm = shm;
            printf("Shared Mem ok on '%d'\n", shm);
            char *ptr = (char *)mmap(NULL,
                                    size_shm,
                                    (PROT_WRITE | PROT_READ),
                                    MAP_SHARED,
                                    shm_semlock_counters.handle_shm,
                                    0L);
            shm_semlock_counters.header = (HeaderObject *)ptr;
            shm_semlock_counters.counters = (CounterObject *)(ptr+sizeof(HeaderObject));
            printf("Shared memory size is %lu vs %d\n", size_shm,
                                                       shm_semlock_counters.header->size_shm);
            // Initialization is successful.
            shm_semlock_counters.state_this = THIS_AVAILABLE;
            header = shm_semlock_counters.header;
            counter = shm_semlock_counters.counters;
            if (unlink) {
                atexit(delete_shm_semlock_counters);

            } else {
                atexit(delete_shm_semlock_counters_without_unlink);
            }
            puts("Ok....");
        } else {
            printf("The shared memory '%s' does not exist\n", shm_semlock_counters.name_shm);
        }
        RELEASE_SHM_LOCK;
        printf("Shm Unlock ok on %p\n", sem);
    } else {
        puts("No Semaphore opened !!");
    }
}

static void _delete_shm_semlock_counters(int unlink) {

    puts("clean up...");
    if (shm_semlock_counters.state_this == THIS_AVAILABLE) {
        if (shm_semlock_counters.counters) {
            if (ACQUIRE_SHM_LOCK) {
                // unmmap
                munmap(shm_semlock_counters.counters,
                       shm_semlock_counters.header->size_shm);
                if (unlink) {
                    shm_unlink(shm_semlock_counters.name_shm);
                }
                shm_semlock_counters.state_this = THIS_CLOSED;
                RELEASE_SHM_LOCK;
            }
        }
        // close lock
        sem_close(shm_semlock_counters.handle_shm_lock);
        sem_unlink(shm_semlock_counters.name_shm_lock);
    }
}


void delete_shm_semlock_counters_without_unlink(void) {
puts(__func__);
    _delete_shm_semlock_counters(0);
}

void delete_shm_semlock_counters(void) {
puts(__func__);
    _delete_shm_semlock_counters(1);
}

void dump_shm_semlock_header(void) {
    if (shm_semlock_counters.state_this == THIS_AVAILABLE) {
        printf("n sems:%d - n sem_slots:%d, n procs:%d, size_shm:%d\n", header->n_semlocks,
                                                                        header->n_slots,
                                                                        header->n_procs,
                                                                        header->size_shm);
    }
}

void dump_shm_semlock_header_counters(void) {
    if (shm_semlock_counters.state_this == THIS_AVAILABLE) {
        printf("header:%p - counter array:%p\n", header, counter);
    }
}
