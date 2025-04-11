#include <unistd.h>
#include <stdio.h>  // puts, printf, scanf
#include <string.h> // memcpy, memcmp, memset

#include <semaphore.h>
typedef sem_t *SEM_HANDLE;

#include "../semaphore_macosx.h"
#include "shared_mem.h"

// Static datas for each process.
CountersWorkaround shm_semlock_counters = {
    .state_this = THIS_NOT_OPEN,
    .name_shm = "/shm_gh125828",
    .handle_shm = (MEMORY_HANDLE)0,
    .create_shm = 0,
    .name_shm_lock = "/mp_gh125828",
    .handle_shm_lock = (SEM_HANDLE)0,
    .header = (HeaderObject *)NULL,
    .counters = (CounterObject *)NULL,
};

HeaderObject *header = NULL;
CounterObject *counter =  NULL;

static void reset_shm_semlock_counters(int size, int nb_slots) {
puts(__func__);

    if (shm_semlock_counters.state_this == THIS_AVAILABLE) {
        if (ACQUIRE_SHM_LOCK) {
            CounterObject *counter = shm_semlock_counters.counters;
            HeaderObject *header = shm_semlock_counters.header;
            dump_shm_semlock_header_counters();
            dump_shm_semlock_header();
            long size_to_reset = header->size_shm-sizeof(HeaderObject);
            printf("1 - size to reset:%lu\n", size_to_reset);
            if (size && size <= size_to_reset) {
                memset(counter, 0, size);

            } else {
                memset(counter, 0, size_to_reset);
            }
            puts("2 - Reset all header parameters");
            if (nb_slots) {
                header->n_slots = nb_slots;
            }
            header->n_semlocks = 0;
            header->n_slots = CALC_NB_SLOTS(header->size_shm);
            header->n_procs = 0;
            dump_shm_semlock_header();
            RELEASE_SHM_LOCK;
        }
    } else {
        puts("No datas");
    }
}

int main(int argc, char *argv[]) {
    char c;
    int size = CALC_SIZE_SHM;
    int nb_slots = CALC_NB_SLOTS(size);
    int unlink = 1;
    int force_open = 1;
    int release_lock = 1;

    if (argc >= 2) {
        sscanf(argv[2], "%d", &size);
        nb_slots = CALC_NB_SLOTS(size);
    }
    puts("--------");
    printf("size:%d, sem slots:%d\n", size, nb_slots);
    connect_shm_semlock_counters(unlink, force_open, release_lock);
    puts("+++++++++");
    dump_shm_semlock_header_counters();
    dump_shm_semlock_header();
    if (shm_semlock_counters.state_this == THIS_AVAILABLE) {
        puts("confirm (Y/N):");
        c = getchar();
        if ( c == 'Y' || c == 'y') {
            reset_shm_semlock_counters(size, nb_slots);
        }
    }
    return 1;
}
