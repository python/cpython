#define _GNU_SOURCE

#ifdef __linux__
#    include <elf.h>
#    include <sys/uio.h>
#    if INTPTR_MAX == INT64_MAX
#        define Elf_Ehdr Elf64_Ehdr
#        define Elf_Shdr Elf64_Shdr
#        define Elf_Phdr Elf64_Phdr
#    else
#        define Elf_Ehdr Elf32_Ehdr
#        define Elf_Shdr Elf32_Shdr
#        define Elf_Phdr Elf32_Phdr
#    endif
#    include <sys/mman.h>
#endif

#if defined(__APPLE__)
#  include <TargetConditionals.h>
// Older macOS SDKs do not define TARGET_OS_OSX
#  if !defined(TARGET_OS_OSX)
#     define TARGET_OS_OSX 1
#  endif
#  if TARGET_OS_OSX
#    include <libproc.h>
#    include <mach-o/fat.h>
#    include <mach-o/loader.h>
#    include <mach-o/nlist.h>
#    include <mach/mach.h>
#    include <mach/mach_vm.h>
#    include <mach/machine.h>
#    include <sys/mman.h>
#    include <sys/proc.h>
#    include <sys/sysctl.h>
#  endif
#endif

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef Py_BUILD_CORE_BUILTIN
#    define Py_BUILD_CORE_MODULE 1
#endif
#include "Python.h"
#include <internal/pycore_debug_offsets.h>  // _Py_DebugOffsets
#include <internal/pycore_frame.h>          // FRAME_SUSPENDED_YIELD_FROM
#include <internal/pycore_interpframe.h>    // FRAME_OWNED_BY_CSTACK
#include <internal/pycore_stackref.h>       // Py_TAG_BITS

#ifndef HAVE_PROCESS_VM_READV
#    define HAVE_PROCESS_VM_READV 0
#endif

struct _Py_AsyncioModuleDebugOffsets {
    struct _asyncio_task_object {
        uint64_t size;
        uint64_t task_name;
        uint64_t task_awaited_by;
        uint64_t task_is_task;
        uint64_t task_awaited_by_is_set;
        uint64_t task_coro;
    } asyncio_task_object;
    struct _asyncio_thread_state {
        uint64_t size;
        uint64_t asyncio_running_loop;
        uint64_t asyncio_running_task;
    } asyncio_thread_state;
};

#if defined(__APPLE__) && TARGET_OS_OSX
static uintptr_t
return_section_address(
    const char* section,
    mach_port_t proc_ref,
    uintptr_t base,
    void* map
) {
    struct mach_header_64* hdr = (struct mach_header_64*)map;
    int ncmds = hdr->ncmds;

    int cmd_cnt = 0;
    struct segment_command_64* cmd = map + sizeof(struct mach_header_64);

    mach_vm_size_t size = 0;
    mach_msg_type_number_t count = sizeof(vm_region_basic_info_data_64_t);
    mach_vm_address_t address = (mach_vm_address_t)base;
    vm_region_basic_info_data_64_t r_info;
    mach_port_t object_name;
    uintptr_t vmaddr = 0;

    for (int i = 0; cmd_cnt < 2 && i < ncmds; i++) {
        if (cmd->cmd == LC_SEGMENT_64 && strcmp(cmd->segname, "__TEXT") == 0) {
            vmaddr = cmd->vmaddr;
        }
        if (cmd->cmd == LC_SEGMENT_64 && strcmp(cmd->segname, "__DATA") == 0) {
            while (cmd->filesize != size) {
                address += size;
                kern_return_t ret = mach_vm_region(
                    proc_ref,
                    &address,
                    &size,
                    VM_REGION_BASIC_INFO_64,
                    (vm_region_info_t)&r_info,  // cppcheck-suppress [uninitvar]
                    &count,
                    &object_name
                );
                if (ret != KERN_SUCCESS) {
                    PyErr_SetString(
                        PyExc_RuntimeError, "Cannot get any more VM maps.\n");
                    return 0;
                }
            }

            int nsects = cmd->nsects;
            struct section_64* sec = (struct section_64*)(
                (void*)cmd + sizeof(struct segment_command_64)
            );
            for (int j = 0; j < nsects; j++) {
                if (strcmp(sec[j].sectname, section) == 0) {
                    return base + sec[j].addr - vmaddr;
                }
            }
            cmd_cnt++;
        }

        cmd = (struct segment_command_64*)((void*)cmd + cmd->cmdsize);
    }

    // We should not be here, but if we are there, we should say about this
    PyErr_SetString(
        PyExc_RuntimeError, "Cannot find section address.\n");
    return 0;
}

static uintptr_t
search_section_in_file(
    const char* secname,
    char* path,
    uintptr_t base,
    mach_vm_size_t size,
    mach_port_t proc_ref
) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        PyErr_Format(PyExc_RuntimeError, "Cannot open binary %s\n", path);
        return 0;
    }

    struct stat fs;
    if (fstat(fd, &fs) == -1) {
        PyErr_Format(
            PyExc_RuntimeError, "Cannot get size of binary %s\n", path);
        close(fd);
        return 0;
    }

    void* map = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        PyErr_Format(PyExc_RuntimeError, "Cannot map binary %s\n", path);
        close(fd);
        return 0;
    }

    uintptr_t result = 0;

    struct mach_header_64* hdr = (struct mach_header_64*)map;
    switch (hdr->magic) {
        case MH_MAGIC:
        case MH_CIGAM:
        case FAT_MAGIC:
        case FAT_CIGAM:
            PyErr_SetString(
                PyExc_RuntimeError,
                "32-bit Mach-O binaries are not supported");
            break;
        case MH_MAGIC_64:
        case MH_CIGAM_64:
            result = return_section_address(secname, proc_ref, base, map);
            break;
        default:
            PyErr_SetString(PyExc_RuntimeError, "Unknown Mach-O magic");
            break;
    }

    munmap(map, fs.st_size);
    if (close(fd) != 0) {
        // This might hide one of the above exceptions, maybe we
        // should chain them?
        PyErr_SetFromErrno(PyExc_OSError);
    }
    return result;
}

static mach_port_t
pid_to_task(pid_t pid)
{
    mach_port_t task;
    kern_return_t result;

    result = task_for_pid(mach_task_self(), pid, &task);
    if (result != KERN_SUCCESS) {
        PyErr_Format(PyExc_PermissionError, "Cannot get task for PID %d", pid);
        return 0;
    }
    return task;
}

static uintptr_t
search_map_for_section(pid_t pid, const char* secname, const char* substr) {
    mach_vm_address_t address = 0;
    mach_vm_size_t size = 0;
    mach_msg_type_number_t count = sizeof(vm_region_basic_info_data_64_t);
    vm_region_basic_info_data_64_t region_info;
    mach_port_t object_name;

    mach_port_t proc_ref = pid_to_task(pid);
    if (proc_ref == 0) {
        return 0;
    }

    int match_found = 0;
    char map_filename[MAXPATHLEN + 1];
    while (mach_vm_region(
                   proc_ref,
                   &address,
                   &size,
                   VM_REGION_BASIC_INFO_64,
                   (vm_region_info_t)&region_info,
                   &count,
                   &object_name) == KERN_SUCCESS)
    {
        if ((region_info.protection & VM_PROT_READ) == 0
            || (region_info.protection & VM_PROT_EXECUTE) == 0) {
            address += size;
            continue;
        }

        int path_len = proc_regionfilename(
            pid, address, map_filename, MAXPATHLEN);
        if (path_len == 0) {
            address += size;
            continue;
        }

        char* filename = strrchr(map_filename, '/');
        if (filename != NULL) {
            filename++;  // Move past the '/'
        } else {
            filename = map_filename;  // No path, use the whole string
        }

        if (!match_found && strncmp(filename, substr, strlen(substr)) == 0) {
            match_found = 1;
            return search_section_in_file(
                secname, map_filename, address, size, proc_ref);
        }

        address += size;
    }

    PyErr_SetString(PyExc_RuntimeError,
        "mach_vm_region failed to find the section");
    return 0;
}

#elif defined(__linux__)
static uintptr_t
find_map_start_address(pid_t pid, char* result_filename, const char* map)
{
    char maps_file_path[64];
    sprintf(maps_file_path, "/proc/%d/maps", pid);

    FILE* maps_file = fopen(maps_file_path, "r");
    if (maps_file == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return 0;
    }

    int match_found = 0;

    char line[256];
    char map_filename[PATH_MAX];
    uintptr_t result_address = 0;
    while (fgets(line, sizeof(line), maps_file) != NULL) {
        unsigned long start_address = 0;
        sscanf(
            line, "%lx-%*x %*s %*s %*s %*s %s",
            &start_address, map_filename
        );
        char* filename = strrchr(map_filename, '/');
        if (filename != NULL) {
            filename++;  // Move past the '/'
        } else {
            filename = map_filename;  // No path, use the whole string
        }

        if (!match_found && strncmp(filename, map, strlen(map)) == 0) {
            match_found = 1;
            result_address = start_address;
            strcpy(result_filename, map_filename);
            break;
        }
    }

    fclose(maps_file);

    if (!match_found) {
        map_filename[0] = '\0';
        PyErr_Format(PyExc_RuntimeError,
            "Cannot find map start address for map: %s", map);
    }

    return result_address;
}

static uintptr_t
search_map_for_section(pid_t pid, const char* secname, const char* map)
{
    char elf_file[256];
    uintptr_t start_address = find_map_start_address(pid, elf_file, map);

    if (start_address == 0) {
        return 0;
    }

    uintptr_t result = 0;
    void* file_memory = NULL;

    int fd = open(elf_file, O_RDONLY);
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto exit;
    }

    struct stat file_stats;
    if (fstat(fd, &file_stats) != 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto exit;
    }

    file_memory = mmap(NULL, file_stats.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_memory == MAP_FAILED) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto exit;
    }

    Elf_Ehdr* elf_header = (Elf_Ehdr*)file_memory;

    Elf_Shdr* section_header_table =
        (Elf_Shdr*)(file_memory + elf_header->e_shoff);

    Elf_Shdr* shstrtab_section = &section_header_table[elf_header->e_shstrndx];
    char* shstrtab = (char*)(file_memory + shstrtab_section->sh_offset);

    Elf_Shdr* section = NULL;
    for (int i = 0; i < elf_header->e_shnum; i++) {
        const char* this_sec_name = (
            shstrtab +
            section_header_table[i].sh_name +
            1  // "+1" accounts for the leading "."
        );

        if (strcmp(secname, this_sec_name) == 0) {
            section = &section_header_table[i];
            break;
        }
    }

    Elf_Phdr* program_header_table =
        (Elf_Phdr*)(file_memory + elf_header->e_phoff);

    // Find the first PT_LOAD segment
    Elf_Phdr* first_load_segment = NULL;
    for (int i = 0; i < elf_header->e_phnum; i++) {
        if (program_header_table[i].p_type == PT_LOAD) {
            first_load_segment = &program_header_table[i];
            break;
        }
    }

    if (section != NULL && first_load_segment != NULL) {
        uintptr_t elf_load_addr =
            first_load_segment->p_vaddr - (
                first_load_segment->p_vaddr % first_load_segment->p_align
            );
        result = start_address + (uintptr_t)section->sh_addr - elf_load_addr;
    }
    else {
        PyErr_Format(PyExc_KeyError,
                     "cannot find map for section %s", secname);
    }

exit:
    if (close(fd) != 0) {
        PyErr_SetFromErrno(PyExc_OSError);
    }
    if (file_memory != NULL) {
        munmap(file_memory, file_stats.st_size);
    }
    return result;
}
#else
static uintptr_t
search_map_for_section(pid_t pid, const char* secname, const char* map)
{
    PyErr_SetString(PyExc_NotImplementedError,
        "Not supported on this platform");
    return 0;
}
#endif

static uintptr_t
get_py_runtime(pid_t pid)
{
    uintptr_t address = search_map_for_section(pid, "PyRuntime", "libpython");
    if (address == 0) {
        PyErr_Clear();
        address = search_map_for_section(pid, "PyRuntime", "python");
    }
    return address;
}

static uintptr_t
get_async_debug(pid_t pid)
{
    uintptr_t result = search_map_for_section(pid, "AsyncioDebug",
        "_asyncio.cpython");
    if (result == 0 && !PyErr_Occurred()) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot find AsyncioDebug section");
    }
    return result;
}


static ssize_t
read_memory(pid_t pid, uintptr_t remote_address, size_t len, void* dst)
{
    ssize_t total_bytes_read = 0;
#if defined(__linux__) && HAVE_PROCESS_VM_READV
    struct iovec local[1];
    struct iovec remote[1];
    ssize_t result = 0;
    ssize_t read = 0;

    do {
        local[0].iov_base = dst + result;
        local[0].iov_len = len - result;
        remote[0].iov_base = (void*)(remote_address + result);
        remote[0].iov_len = len - result;

        read = process_vm_readv(pid, local, 1, remote, 1, 0);
        if (read < 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }

        result += read;
    } while ((size_t)read != local[0].iov_len);
    total_bytes_read = result;
#elif defined(__APPLE__) && TARGET_OS_OSX
    ssize_t result = -1;
    kern_return_t kr = mach_vm_read_overwrite(
            pid_to_task(pid),
            (mach_vm_address_t)remote_address,
            len,
            (mach_vm_address_t)dst,
            (mach_vm_size_t*)&result);

    if (kr != KERN_SUCCESS) {
        switch (kr) {
            case KERN_PROTECTION_FAILURE:
                PyErr_SetString(
                    PyExc_PermissionError,
                    "Not enough permissions to read memory");
                break;
            case KERN_INVALID_ARGUMENT:
                PyErr_SetString(
                    PyExc_PermissionError,
                    "Invalid argument to mach_vm_read_overwrite");
                break;
            default:
                PyErr_SetString(
                    PyExc_RuntimeError,
                    "Unknown error reading memory");
        }
        return -1;
    }
    total_bytes_read = len;
#else
    PyErr_SetString(
        PyExc_RuntimeError,
        "Memory reading is not supported on this platform");
    return -1;
#endif
    return total_bytes_read;
}

static int
read_string(
    pid_t pid,
    _Py_DebugOffsets* debug_offsets,
    uintptr_t address,
    char* buffer,
    Py_ssize_t size
) {
    Py_ssize_t len;
    ssize_t bytes_read = read_memory(
        pid,
        address + debug_offsets->unicode_object.length,
        sizeof(Py_ssize_t),
        &len
    );
    if (bytes_read < 0) {
        return -1;
    }
    if (len >= size) {
        PyErr_SetString(PyExc_RuntimeError, "Buffer too small");
        return -1;
    }
    size_t offset = debug_offsets->unicode_object.asciiobject_size;
    bytes_read = read_memory(pid, address + offset, len, buffer);
    if (bytes_read < 0) {
        return -1;
    }
    buffer[len] = '\0';
    return 0;
}


static inline int
read_ptr(pid_t pid, uintptr_t address, uintptr_t *ptr_addr)
{
    int bytes_read = read_memory(pid, address, sizeof(void*), ptr_addr);
    if (bytes_read < 0) {
        return -1;
    }
    return 0;
}

static inline int
read_ssize_t(pid_t pid, uintptr_t address, Py_ssize_t *size)
{
    int bytes_read = read_memory(pid, address, sizeof(Py_ssize_t), size);
    if (bytes_read < 0) {
        return -1;
    }
    return 0;
}

static int
read_py_ptr(pid_t pid, uintptr_t address, uintptr_t *ptr_addr)
{
    if (read_ptr(pid, address, ptr_addr)) {
        return -1;
    }
    *ptr_addr &= ~Py_TAG_BITS;
    return 0;
}

static int
read_char(pid_t pid, uintptr_t address, char *result)
{
    int bytes_read = read_memory(pid, address, sizeof(char), result);
    if (bytes_read < 0) {
        return -1;
    }
    return 0;
}

static int
read_int(pid_t pid, uintptr_t address, int *result)
{
    int bytes_read = read_memory(pid, address, sizeof(int), result);
    if (bytes_read < 0) {
        return -1;
    }
    return 0;
}

static int
read_unsigned_long(pid_t pid, uintptr_t address, unsigned long *result)
{
    int bytes_read = read_memory(pid, address, sizeof(unsigned long), result);
    if (bytes_read < 0) {
        return -1;
    }
    return 0;
}

static int
read_pyobj(pid_t pid, uintptr_t address, PyObject *ptr_addr)
{
    int bytes_read = read_memory(pid, address, sizeof(PyObject), ptr_addr);
    if (bytes_read < 0) {
        return -1;
    }
    return 0;
}

static PyObject *
read_py_str(
    pid_t pid,
    _Py_DebugOffsets* debug_offsets,
    uintptr_t address,
    ssize_t max_len
) {
    assert(max_len > 0);

    PyObject *result = NULL;

    char *buf = (char *)PyMem_RawMalloc(max_len);
    if (buf == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    if (read_string(pid, debug_offsets, address, buf, max_len)) {
        goto err;
    }

    result = PyUnicode_FromString(buf);
    if (result == NULL) {
        goto err;
    }

    PyMem_RawFree(buf);
    assert(result != NULL);
    return result;

err:
    PyMem_RawFree(buf);
    return NULL;
}

static long
read_py_long(pid_t pid, _Py_DebugOffsets* offsets, uintptr_t address)
{
    unsigned int shift = PYLONG_BITS_IN_DIGIT;

    ssize_t size;
    uintptr_t lv_tag;

    int bytes_read = read_memory(
        pid, address + offsets->long_object.lv_tag,
        sizeof(uintptr_t),
        &lv_tag);
    if (bytes_read < 0) {
        return -1;
    }

    int negative = (lv_tag & 3) == 2;
    size = lv_tag >> 3;

    if (size == 0) {
        return 0;
    }

    digit *digits = (digit *)PyMem_RawMalloc(size * sizeof(digit));
    if (!digits) {
        PyErr_NoMemory();
        return -1;
    }

    bytes_read = read_memory(
        pid,
        address + offsets->long_object.ob_digit,
        sizeof(digit) * size,
        digits
    );
    if (bytes_read < 0) {
        goto error;
    }

    long value = 0;

    // In theory this can overflow, but because of llvm/llvm-project#16778
    // we can't use __builtin_mul_overflow because it fails to link with
    // __muloti4 on aarch64. In practice this is fine because all we're
    // testing here are task numbers that would fit in a single byte.
    for (ssize_t i = 0; i < size; ++i) {
        long long factor = digits[i] * (1UL << (ssize_t)(shift * i));
        value += factor;
    }
    PyMem_RawFree(digits);
    if (negative) {
        value *= -1;
    }
    return value;
error:
    PyMem_RawFree(digits);
    return -1;
}

static PyObject *
parse_task_name(
    int pid,
    _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address
) {
    uintptr_t task_name_addr;
    int err = read_py_ptr(
        pid,
        task_address + async_offsets->asyncio_task_object.task_name,
        &task_name_addr);
    if (err) {
        return NULL;
    }

    // The task name can be a long or a string so we need to check the type

    PyObject task_name_obj;
    err = read_pyobj(
        pid,
        task_name_addr,
        &task_name_obj);
    if (err) {
        return NULL;
    }

    unsigned long flags;
    err = read_unsigned_long(
        pid,
        (uintptr_t)task_name_obj.ob_type + offsets->type_object.tp_flags,
        &flags);
    if (err) {
        return NULL;
    }

    if ((flags & Py_TPFLAGS_LONG_SUBCLASS)) {
        long res = read_py_long(pid, offsets, task_name_addr);
        if (res == -1) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to get task name");
            return NULL;
        }
        return PyUnicode_FromFormat("Task-%d", res);
    }

    if(!(flags & Py_TPFLAGS_UNICODE_SUBCLASS)) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid task name object");
        return NULL;
    }

    return read_py_str(
        pid,
        offsets,
        task_name_addr,
        255
    );
}

static int
parse_coro_chain(
    int pid,
    struct _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t coro_address,
    PyObject *render_to
) {
    assert((void*)coro_address != NULL);

    uintptr_t gen_type_addr;
    int err = read_ptr(
        pid,
        coro_address + sizeof(void*),
        &gen_type_addr);
    if (err) {
        return -1;
    }

    uintptr_t gen_name_addr;
    err = read_py_ptr(
        pid,
        coro_address + offsets->gen_object.gi_name,
        &gen_name_addr);
    if (err) {
        return -1;
    }

    PyObject *name = read_py_str(
        pid,
        offsets,
        gen_name_addr,
        255
    );
    if (name == NULL) {
        return -1;
    }

    if (PyList_Append(render_to, name)) {
        Py_DECREF(name);
        return -1;
    }
    Py_DECREF(name);

    int gi_frame_state;
    err = read_int(
        pid,
        coro_address + offsets->gen_object.gi_frame_state,
        &gi_frame_state);
    if (err) {
        return -1;
    }

    if (gi_frame_state == FRAME_SUSPENDED_YIELD_FROM) {
        char owner;
        err = read_char(
            pid,
            coro_address + offsets->gen_object.gi_iframe +
                offsets->interpreter_frame.owner,
            &owner
        );
        if (err) {
            return -1;
        }
        if (owner != FRAME_OWNED_BY_GENERATOR) {
            PyErr_SetString(
                PyExc_RuntimeError,
                "generator doesn't own its frame \\_o_/");
            return -1;
        }

        uintptr_t stackpointer_addr;
        err = read_py_ptr(
            pid,
            coro_address + offsets->gen_object.gi_iframe +
                offsets->interpreter_frame.stackpointer,
            &stackpointer_addr);
        if (err) {
            return -1;
        }

        if ((void*)stackpointer_addr != NULL) {
            uintptr_t gi_await_addr;
            err = read_py_ptr(
                pid,
                stackpointer_addr - sizeof(void*),
                &gi_await_addr);
            if (err) {
                return -1;
            }

            if ((void*)gi_await_addr != NULL) {
                uintptr_t gi_await_addr_type_addr;
                int err = read_ptr(
                    pid,
                    gi_await_addr + sizeof(void*),
                    &gi_await_addr_type_addr);
                if (err) {
                    return -1;
                }

                if (gen_type_addr == gi_await_addr_type_addr) {
                    /* This needs an explanation. We always start with parsing
                       native coroutine / generator frames. Ultimately they
                       are awaiting on something. That something can be
                       a native coroutine frame or... an iterator.
                       If it's the latter -- we can't continue building
                       our chain. So the condition to bail out of this is
                       to do that when the type of the current coroutine
                       doesn't match the type of whatever it points to
                       in its cr_await.
                    */
                    err = parse_coro_chain(
                        pid,
                        offsets,
                        async_offsets,
                        gi_await_addr,
                        render_to
                    );
                    if (err) {
                        return -1;
                    }
                }
            }
        }

    }

    return 0;
}


static int
parse_task_awaited_by(
    int pid,
    struct _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address,
    PyObject *awaited_by
);


static int
parse_task(
    int pid,
    struct _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address,
    PyObject *render_to
) {
    char is_task;
    int err = read_char(
        pid,
        task_address + async_offsets->asyncio_task_object.task_is_task,
        &is_task);
    if (err) {
        return -1;
    }

    uintptr_t refcnt;
    read_ptr(pid, task_address + sizeof(Py_ssize_t), &refcnt);

    PyObject* result = PyList_New(0);
    if (result == NULL) {
        return -1;
    }

    PyObject *call_stack = PyList_New(0);
    if (call_stack == NULL) {
        goto err;
    }
    if (PyList_Append(result, call_stack)) {
        Py_DECREF(call_stack);
        goto err;
    }
    /* we can operate on a borrowed one to simplify cleanup */
    Py_DECREF(call_stack);

    if (is_task) {
        PyObject *tn = parse_task_name(
            pid, offsets, async_offsets, task_address);
        if (tn == NULL) {
            goto err;
        }
        if (PyList_Append(result, tn)) {
            Py_DECREF(tn);
            goto err;
        }
        Py_DECREF(tn);

        uintptr_t coro_addr;
        err = read_py_ptr(
            pid,
            task_address + async_offsets->asyncio_task_object.task_coro,
            &coro_addr);
        if (err) {
            goto err;
        }

        if ((void*)coro_addr != NULL) {
            err = parse_coro_chain(
                pid,
                offsets,
                async_offsets,
                coro_addr,
                call_stack
            );
            if (err) {
                goto err;
            }

            if (PyList_Reverse(call_stack)) {
                goto err;
            }
        }
    }

    if (PyList_Append(render_to, result)) {
        goto err;
    }

    PyObject *awaited_by = PyList_New(0);
    if (awaited_by == NULL) {
        goto err;
    }
    if (PyList_Append(result, awaited_by)) {
        Py_DECREF(awaited_by);
        goto err;
    }
    /* we can operate on a borrowed one to simplify cleanup */
    Py_DECREF(awaited_by);

    if (parse_task_awaited_by(pid, offsets, async_offsets,
                              task_address, awaited_by)
    ) {
        goto err;
    }
    Py_DECREF(result);

    return 0;

err:
    Py_DECREF(result);
    return -1;
}

static int
parse_tasks_in_set(
    int pid,
    struct _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t set_addr,
    PyObject *awaited_by
) {
    uintptr_t set_obj;
    if (read_py_ptr(
            pid,
            set_addr,
            &set_obj)
    ) {
        return -1;
    }

    Py_ssize_t num_els;
    if (read_ssize_t(
            pid,
            set_obj + offsets->set_object.used,
            &num_els)
    ) {
        return -1;
    }

    Py_ssize_t set_len;
    if (read_ssize_t(
            pid,
            set_obj + offsets->set_object.mask,
            &set_len)
    ) {
        return -1;
    }
    set_len++; // The set contains the `mask+1` element slots.

    uintptr_t table_ptr;
    if (read_ptr(
            pid,
            set_obj + offsets->set_object.table,
            &table_ptr)
    ) {
        return -1;
    }

    Py_ssize_t i = 0;
    Py_ssize_t els = 0;
    while (i < set_len) {
        uintptr_t key_addr;
        if (read_py_ptr(pid, table_ptr, &key_addr)) {
            return -1;
        }

        if ((void*)key_addr != NULL) {
            Py_ssize_t ref_cnt;
            if (read_ssize_t(pid, table_ptr, &ref_cnt)) {
                return -1;
            }

            if (ref_cnt) {
                // if 'ref_cnt=0' it's a set dummy marker

                if (parse_task(
                    pid,
                    offsets,
                    async_offsets,
                    key_addr,
                    awaited_by)
                ) {
                    return -1;
                }

                if (++els == num_els) {
                    break;
                }
            }
        }

        table_ptr += sizeof(void*) * 2;
        i++;
    }
    return 0;
}


static int
parse_task_awaited_by(
    int pid,
    struct _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address,
    PyObject *awaited_by
) {
    uintptr_t task_ab_addr;
    int err = read_py_ptr(
        pid,
        task_address + async_offsets->asyncio_task_object.task_awaited_by,
        &task_ab_addr);
    if (err) {
        return -1;
    }

    if ((void*)task_ab_addr == NULL) {
        return 0;
    }

    char awaited_by_is_a_set;
    err = read_char(
        pid,
        task_address + async_offsets->asyncio_task_object.task_awaited_by_is_set,
        &awaited_by_is_a_set);
    if (err) {
        return -1;
    }

    if (awaited_by_is_a_set) {
        if (parse_tasks_in_set(
            pid,
            offsets,
            async_offsets,
            task_address + async_offsets->asyncio_task_object.task_awaited_by,
            awaited_by)
         ) {
            return -1;
        }
    } else {
        uintptr_t sub_task;
        if (read_py_ptr(
                pid,
                task_address + async_offsets->asyncio_task_object.task_awaited_by,
                &sub_task)
        ) {
            return -1;
        }

        if (parse_task(
            pid,
            offsets,
            async_offsets,
            sub_task,
            awaited_by)
        ) {
            return -1;
        }
    }

    return 0;
}

static int
parse_code_object(
    int pid,
    PyObject* result,
    struct _Py_DebugOffsets* offsets,
    uintptr_t address,
    uintptr_t* previous_frame
) {
    uintptr_t address_of_function_name;
    int bytes_read = read_memory(
        pid,
        address + offsets->code_object.name,
        sizeof(void*),
        &address_of_function_name
    );
    if (bytes_read < 0) {
        return -1;
    }

    if ((void*)address_of_function_name == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "No function name found");
        return -1;
    }

    PyObject* py_function_name = read_py_str(
        pid, offsets, address_of_function_name, 256);
    if (py_function_name == NULL) {
        return -1;
    }

    if (PyList_Append(result, py_function_name) == -1) {
        Py_DECREF(py_function_name);
        return -1;
    }
    Py_DECREF(py_function_name);

    return 0;
}

static int
parse_frame_object(
    int pid,
    PyObject* result,
    struct _Py_DebugOffsets* offsets,
    uintptr_t address,
    uintptr_t* previous_frame
) {
    int err;

    ssize_t bytes_read = read_memory(
        pid,
        address + offsets->interpreter_frame.previous,
        sizeof(void*),
        previous_frame
    );
    if (bytes_read < 0) {
        return -1;
    }

    char owner;
    if (read_char(pid, address + offsets->interpreter_frame.owner, &owner)) {
        return -1;
    }

    if (owner >= FRAME_OWNED_BY_INTERPRETER) {
        return 0;
    }

    uintptr_t address_of_code_object;
    err = read_py_ptr(
        pid,
        address + offsets->interpreter_frame.executable,
        &address_of_code_object
    );
    if (err) {
        return -1;
    }

    if ((void*)address_of_code_object == NULL) {
        return 0;
    }

    return parse_code_object(
        pid, result, offsets, address_of_code_object, previous_frame);
}

static int
parse_async_frame_object(
    int pid,
    PyObject* result,
    struct _Py_DebugOffsets* offsets,
    uintptr_t address,
    uintptr_t* previous_frame,
    uintptr_t* code_object
) {
    int err;

    ssize_t bytes_read = read_memory(
        pid,
        address + offsets->interpreter_frame.previous,
        sizeof(void*),
        previous_frame
    );
    if (bytes_read < 0) {
        return -1;
    }

    char owner;
    bytes_read = read_memory(
        pid, address + offsets->interpreter_frame.owner, sizeof(char), &owner);
    if (bytes_read < 0) {
        return -1;
    }

    if (owner == FRAME_OWNED_BY_CSTACK || owner == FRAME_OWNED_BY_INTERPRETER) {
        return 0;  // C frame
    }

    if (owner != FRAME_OWNED_BY_GENERATOR
        && owner != FRAME_OWNED_BY_THREAD) {
        PyErr_Format(PyExc_RuntimeError, "Unhandled frame owner %d.\n", owner);
        return -1;
    }

    err = read_py_ptr(
        pid,
        address + offsets->interpreter_frame.executable,
        code_object
    );
    if (err) {
        return -1;
    }

    assert(code_object != NULL);
    if ((void*)*code_object == NULL) {
        return 0;
    }

    if (parse_code_object(
        pid, result, offsets, *code_object, previous_frame)) {
        return -1;
    }

    return 1;
}

static int
read_offsets(
    int pid,
    uintptr_t *runtime_start_address,
    _Py_DebugOffsets* debug_offsets
) {
    *runtime_start_address = get_py_runtime(pid);
    if ((void*)*runtime_start_address == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(
                PyExc_RuntimeError, "Failed to get .PyRuntime address");
        }
        return -1;
    }
    size_t size = sizeof(struct _Py_DebugOffsets);
    ssize_t bytes_read = read_memory(
        pid, *runtime_start_address, size, debug_offsets);
    if (bytes_read < 0) {
        return -1;
    }
    return 0;
}

static int
read_async_debug(
    int pid,
    struct _Py_AsyncioModuleDebugOffsets* async_debug
) {
    uintptr_t async_debug_addr = get_async_debug(pid);
    if (!async_debug_addr) {
        return -1;
    }
    size_t size = sizeof(struct _Py_AsyncioModuleDebugOffsets);
    ssize_t bytes_read = read_memory(
        pid, async_debug_addr, size, async_debug);
    if (bytes_read < 0) {
        return -1;
    }
    return 0;
}

static int
find_running_frame(
    int pid,
    uintptr_t runtime_start_address,
    _Py_DebugOffsets* local_debug_offsets,
    uintptr_t *frame
) {
    off_t interpreter_state_list_head =
        local_debug_offsets->runtime_state.interpreters_head;

    uintptr_t address_of_interpreter_state;
    int bytes_read = read_memory(
            pid,
            runtime_start_address + interpreter_state_list_head,
            sizeof(void*),
            &address_of_interpreter_state);
    if (bytes_read < 0) {
        return -1;
    }

    if (address_of_interpreter_state == 0) {
        PyErr_SetString(PyExc_RuntimeError, "No interpreter state found");
        return -1;
    }

    uintptr_t address_of_thread;
    bytes_read = read_memory(
            pid,
            address_of_interpreter_state +
                local_debug_offsets->interpreter_state.threads_head,
            sizeof(void*),
            &address_of_thread);
    if (bytes_read < 0) {
        return -1;
    }

    // No Python frames are available for us (can happen at tear-down).
    if ((void*)address_of_thread != NULL) {
        int err = read_ptr(
            pid,
            address_of_thread + local_debug_offsets->thread_state.current_frame,
            frame);
        if (err) {
            return -1;
        }
        return 0;
    }

    *frame = (uintptr_t)NULL;
    return 0;
}

static int
find_running_task(
    int pid,
    uintptr_t runtime_start_address,
    _Py_DebugOffsets *local_debug_offsets,
    struct _Py_AsyncioModuleDebugOffsets *async_offsets,
    uintptr_t *running_task_addr
) {
    *running_task_addr = (uintptr_t)NULL;

    off_t interpreter_state_list_head =
        local_debug_offsets->runtime_state.interpreters_head;

    uintptr_t address_of_interpreter_state;
    int bytes_read = read_memory(
            pid,
            runtime_start_address + interpreter_state_list_head,
            sizeof(void*),
            &address_of_interpreter_state);
    if (bytes_read < 0) {
        return -1;
    }

    if (address_of_interpreter_state == 0) {
        PyErr_SetString(PyExc_RuntimeError, "No interpreter state found");
        return -1;
    }

    uintptr_t address_of_thread;
    bytes_read = read_memory(
            pid,
            address_of_interpreter_state +
                local_debug_offsets->interpreter_state.threads_head,
            sizeof(void*),
            &address_of_thread);
    if (bytes_read < 0) {
        return -1;
    }

    uintptr_t address_of_running_loop;
    // No Python frames are available for us (can happen at tear-down).
    if ((void*)address_of_thread == NULL) {
        return 0;
    }

    bytes_read = read_py_ptr(
        pid,
        address_of_thread
        + async_offsets->asyncio_thread_state.asyncio_running_loop,
        &address_of_running_loop);
    if (bytes_read == -1) {
        return -1;
    }

    // no asyncio loop is now running
    if ((void*)address_of_running_loop == NULL) {
        return 0;
    }

    int err = read_ptr(
        pid,
        address_of_thread
        + async_offsets->asyncio_thread_state.asyncio_running_task,
        running_task_addr);
    if (err) {
        return -1;
    }

    return 0;
}

static PyObject*
get_stack_trace(PyObject* self, PyObject* args)
{
#if (!defined(__linux__) && !defined(__APPLE__)) || \
    (defined(__linux__) && !HAVE_PROCESS_VM_READV)
    PyErr_SetString(
        PyExc_RuntimeError,
        "get_stack_trace is not supported on this platform");
    return NULL;
#endif
    int pid;

    if (!PyArg_ParseTuple(args, "i", &pid)) {
        return NULL;
    }

    uintptr_t runtime_start_address = get_py_runtime(pid);
    if (runtime_start_address == 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(
                PyExc_RuntimeError, "Failed to get .PyRuntime address");
        }
        return NULL;
    }
    struct _Py_DebugOffsets local_debug_offsets;

    if (read_offsets(pid, &runtime_start_address, &local_debug_offsets)) {
        return NULL;
    }

    uintptr_t address_of_current_frame;
    if (find_running_frame(
        pid, runtime_start_address, &local_debug_offsets,
        &address_of_current_frame)
    ) {
        return NULL;
    }

    PyObject* result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }

    while ((void*)address_of_current_frame != NULL) {
        if (parse_frame_object(
                    pid,
                    result,
                    &local_debug_offsets,
                    address_of_current_frame,
                    &address_of_current_frame)
            < 0)
        {
            Py_DECREF(result);
            return NULL;
        }
    }

    return result;
}

static PyObject*
get_async_stack_trace(PyObject* self, PyObject* args)
{
#if (!defined(__linux__) && !defined(__APPLE__)) || \
    (defined(__linux__) && !HAVE_PROCESS_VM_READV)
    PyErr_SetString(
        PyExc_RuntimeError,
        "get_stack_trace is not supported on this platform");
    return NULL;
#endif
    int pid;

    if (!PyArg_ParseTuple(args, "i", &pid)) {
        return NULL;
    }

    uintptr_t runtime_start_address = get_py_runtime(pid);
    if (runtime_start_address == 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(
                PyExc_RuntimeError, "Failed to get .PyRuntime address");
        }
        return NULL;
    }
    struct _Py_DebugOffsets local_debug_offsets;

    if (read_offsets(pid, &runtime_start_address, &local_debug_offsets)) {
        return NULL;
    }

    struct _Py_AsyncioModuleDebugOffsets local_async_debug;
    if (read_async_debug(pid, &local_async_debug)) {
        return NULL;
    }

    PyObject* result = PyList_New(1);
    if (result == NULL) {
        return NULL;
    }
    PyObject* calls = PyList_New(0);
    if (calls == NULL) {
        Py_DECREF(result);
        return NULL;
    }
    if (PyList_SetItem(result, 0, calls)) { /* steals ref to 'calls' */
        Py_DECREF(result);
        Py_DECREF(calls);
        return NULL;
    }

    uintptr_t running_task_addr = (uintptr_t)NULL;
    if (find_running_task(
        pid, runtime_start_address, &local_debug_offsets, &local_async_debug,
        &running_task_addr)
    ) {
        goto result_err;
    }

    if ((void*)running_task_addr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "No running task found");
        goto result_err;
    }

    uintptr_t running_coro_addr;
    if (read_py_ptr(
        pid,
        running_task_addr + local_async_debug.asyncio_task_object.task_coro,
        &running_coro_addr
    )) {
        goto result_err;
    }

    if ((void*)running_coro_addr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Running task coro is NULL");
        goto result_err;
    }

    // note: genobject's gi_iframe is an embedded struct so the address to
    // the offset leads directly to its first field: f_executable
    uintptr_t address_of_running_task_code_obj;
    if (read_py_ptr(
        pid,
        running_coro_addr + local_debug_offsets.gen_object.gi_iframe,
        &address_of_running_task_code_obj
    )) {
        goto result_err;
    }

    if ((void*)address_of_running_task_code_obj == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Running task code object is NULL");
        goto result_err;
    }

    uintptr_t address_of_current_frame;
    if (find_running_frame(
        pid, runtime_start_address, &local_debug_offsets,
        &address_of_current_frame)
    ) {
        goto result_err;
    }

    uintptr_t address_of_code_object;
    while ((void*)address_of_current_frame != NULL) {
        int res = parse_async_frame_object(
            pid,
            calls,
            &local_debug_offsets,
            address_of_current_frame,
            &address_of_current_frame,
            &address_of_code_object
        );

        if (res < 0) {
            goto result_err;
        }

        if (address_of_code_object == address_of_running_task_code_obj) {
            break;
        }
    }

    PyObject *tn = parse_task_name(
        pid, &local_debug_offsets, &local_async_debug, running_task_addr);
    if (tn == NULL) {
        goto result_err;
    }
    if (PyList_Append(result, tn)) {
        Py_DECREF(tn);
        goto result_err;
    }
    Py_DECREF(tn);

    PyObject* awaited_by = PyList_New(0);
    if (awaited_by == NULL) {
        goto result_err;
    }
    if (PyList_Append(result, awaited_by)) {
        Py_DECREF(awaited_by);
        goto result_err;
    }
    Py_DECREF(awaited_by);

    if (parse_task_awaited_by(
        pid, &local_debug_offsets, &local_async_debug,
        running_task_addr, awaited_by)
    ) {
        goto result_err;
    }

    return result;

result_err:
    Py_DECREF(result);
    return NULL;
}


static PyMethodDef methods[] = {
    {"get_stack_trace", get_stack_trace, METH_VARARGS,
        "Get the Python stack from a given PID"},
    {"get_async_stack_trace", get_async_stack_trace, METH_VARARGS,
        "Get the asyncio stack from a given PID"},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_testexternalinspection",
    .m_size = -1,
    .m_methods = methods,
};

PyMODINIT_FUNC
PyInit__testexternalinspection(void)
{
    PyObject* mod = PyModule_Create(&module);
    if (mod == NULL) {
        return NULL;
    }
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(mod, Py_MOD_GIL_NOT_USED);
#endif
    int rc = PyModule_AddIntConstant(
        mod, "PROCESS_VM_READV_SUPPORTED", HAVE_PROCESS_VM_READV);
    if (rc < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    return mod;
}
