#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ALLOC_HASH_TABLE_ITEMS 1048576
#define ALLOC_ITEM_SIZE 24
#define ALLOC_HASH_TABLE_TOTAL_SIZE (ALLOC_HASH_TABLE_ITEMS * ALLOC_ITEM_SIZE)
#define ALLOC_HASH_TABLE_OFFSET 0

#define ALLOC_NAME_LIST_POINTER_OFFSET ALLOC_HASH_TABLE_TOTAL_SIZE
#define ALLOC_NAME_LIST_OFFSET (ALLOC_NAME_LIST_POINTER_OFFSET + 8)
#define ALLOC_NAME_LIST_SIZE 24000000

#define TOTAL_SIZE (ALLOC_NAME_LIST_OFFSET + ALLOC_NAME_LIST_SIZE)

static void on_incref(void *op)
{

    static int alloc_info_enabled = -1;
    if (alloc_info_enabled == -1)
    {
        if (getenv("ALLOC_INFO_ENABLED") == NULL)
        {
            alloc_info_enabled = 0;
        }
        else
        {
            alloc_info_enabled = 1;
        }
    }


    // op is a PyObject
    // op.ob_refcnt is the first field in op
    size_t* refcnt = (size_t*)((void*)op);
    if (alloc_info_enabled && *refcnt == 2)
    {
        // This is extremely hacky, but it is the first approach I could make work.
        // I need some kind of global data structure for keeping track of objects.  If I define it in this header,
        // linking files due to duplicate symbols - looks like multiple modules(?) import this header and get
        // their own copy of the data structure.  If I define it in object.c, then importing modules doesn't work
        // due to missing symbols.  Separately, I haven't found a way to create new .c files and include them
        // in the compilation.  So all the logic has to live inside this function (defining other functions gives
        // duplicates just like global variables - I think I even tried always-inline functions at one point).
        // Using a static array kinda works, but each module gets its own static array.  So shared memory is a
        // way to make all the modules share the same data structure.  Annoylingly this requires deleting the
        // leftover shared memory object before running the program again, otherwise we start with existing data.
        // One upside of using shared memory is that it gives an easy way of reading the data from Python.

        static int debug_verbosity = -1;
        if (debug_verbosity == -1)
        {
            char *getenv_result = getenv("ALLOC_INFO_DEBUG");
            if (getenv_result == NULL)
            {
                debug_verbosity = 0;
            }
            else
            {
                debug_verbosity = (*getenv_result) - '0';
            }
        }

        if (debug_verbosity >= 8)
            printf("incref begin\n");

        static void *shared_memory = NULL;
        if (shared_memory == NULL)
        {
            char shm_name[20];
            sprintf(shm_name, "alloc-info-%d", getpid());
            int fd = shm_open(shm_name, O_RDWR, 0666);
            if (fd == -1)
            {
                if (debug_verbosity >= 2)
                    printf("creating shm\n");
                fd = shm_open(shm_name, O_CREAT | O_RDWR | O_EXCL, 0666);
                if (fd == -1)
                {
                    printf("shm_open failed\n");
                    exit(1);
                }
                int result = ftruncate(fd, TOTAL_SIZE * 24);
                if (result == -1)
                {
                    printf("ftruncate failed\n");
                    exit(1);
                }
                shared_memory = mmap(0, TOTAL_SIZE * 24, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if (shared_memory == (void *)-1)
                {
                    printf("mmap failed\n");
                    exit(1);
                }
                memset(shared_memory, 0, TOTAL_SIZE * 24);
            }
            else
            {
                if (debug_verbosity >= 8)
                    printf("opening existing shm\n");
                shared_memory = mmap(0, TOTAL_SIZE * 24, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if (shared_memory == (void *)-1)
                {
                    printf("mmap failed\n");
                    exit(1);
                }
            }
        }

        if (debug_verbosity >= 8)
            printf("incref have shared_memory\n");

        // op is a PyObject
        // op.ob_type is 8 bytes into op
        void *ob_type = *((void **)(((void *)op) + 8));
        uint32_t hashed_type = (((uint32_t)(uint64_t)ob_type) * ((uint32_t)2654435761)) >> 12;
        if (hashed_type >= ALLOC_HASH_TABLE_ITEMS)
        {
            printf("hash out of bounds\n");
            exit(1);
        }

        if (debug_verbosity >= 8)
            printf("incref have hashed type\n");

        size_t current_table_index = hashed_type;
    label_try_next_index:
        void **type_ptr = (void **)(shared_memory + ALLOC_HASH_TABLE_OFFSET + current_table_index * ALLOC_ITEM_SIZE);
        void *current_type = *type_ptr;
        if (current_type != NULL && current_type != ob_type)
        {
            current_table_index = (current_table_index + 1) % ALLOC_HASH_TABLE_ITEMS;
            if (current_table_index == hashed_type)
            {
                printf("hash table full\n");
                exit(1);
            }
            goto label_try_next_index;
        }

        if (debug_verbosity >= 8)
            printf("incref have hash table entry\n");

        if (current_type == NULL)
        {
            if (debug_verbosity >= 6)
                printf("incref adding new type\n");

            (*type_ptr) = ob_type;

            // ob_type is a PyTypeObject
            // ob_type.tp_name is 24 bytes into ob_type
            const char *tp_name = *((const char **)(ob_type + 24));
            if (debug_verbosity >= 4)
                printf("adding type %s\n", tp_name);
            if (debug_verbosity >= 9)
                printf("shared_memory %ld\n", (size_t)shared_memory);
            char **name_list_pointer = (char **)(shared_memory + ALLOC_NAME_LIST_POINTER_OFFSET);
            if (debug_verbosity >= 9)
                printf("name_list_pointer %ld\n", (size_t)name_list_pointer);

            size_t num_chars_to_add = strlen(tp_name) + 1;
            if (debug_verbosity >= 9)
                printf("num_chars_to_add %ld\n", num_chars_to_add);
            char *name_list = (*name_list_pointer);
            if (debug_verbosity >= 9)
                printf("name_list %ld\n", (size_t)name_list);
            if (name_list == NULL)
            {
                name_list = (char *)(shared_memory + ALLOC_NAME_LIST_OFFSET);
                if (debug_verbosity >= 9)
                    printf("name_list %ld\n", (size_t)name_list);
            }
            (*name_list_pointer) = name_list + num_chars_to_add;
            if (debug_verbosity >= 9)
                printf("updated *name_list_pointer\n");

            strcpy(name_list, tp_name);
            if (debug_verbosity >= 9)
                printf("strcpy done\n");
        }

        if (debug_verbosity >= 8)
            printf("incref type exists\n");

        if ((*type_ptr) != ob_type)
        {
            printf("hash table error\n");
            exit(1);
        }

        size_t *counter_ptr = (size_t *)(((void *)type_ptr) + 8);
        size_t *total_size_ptr = (size_t *)(((void *)type_ptr) + 16);

        (*counter_ptr)++;
        (*total_size_ptr) += sizeof(op); // TODO
    }
}