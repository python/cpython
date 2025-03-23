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
#include <internal/pycore_runtime.h>
#include <internal/pycore_ceval.h>

#ifndef HAVE_PROCESS_VM_READV
#    define HAVE_PROCESS_VM_READV 0
#endif

/*[clinic input]
module _pdb
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7fb1cf2618bcf972]*/

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
search_section_in_file(const char* secname, char* path, uintptr_t base, mach_vm_size_t size, mach_port_t proc_ref)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        PyErr_Format(PyExc_RuntimeError, "Cannot open binary %s\n", path);
        return 0;
    }

    struct stat fs;
    if (fstat(fd, &fs) == -1) {
        PyErr_Format(PyExc_RuntimeError, "Cannot get size of binary %s\n", path);
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
            PyErr_SetString(PyExc_RuntimeError, "32-bit Mach-O binaries are not supported");
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
        PyErr_SetString(PyExc_PermissionError, "Cannot get task for PID");
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

#endif

#ifdef __linux__
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
        sscanf(line, "%lx-%*x %*s %*s %*s %*s %s", &start_address, map_filename);
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

    Elf_Shdr* section_header_table = (Elf_Shdr*)(file_memory + elf_header->e_shoff);

    Elf_Shdr* shstrtab_section = &section_header_table[elf_header->e_shstrndx];
    char* shstrtab = (char*)(file_memory + shstrtab_section->sh_offset);

    Elf_Shdr* section = NULL;
    for (int i = 0; i < elf_header->e_shnum; i++) {
        char* this_sec_name = shstrtab + section_header_table[i].sh_name;
        // Move 1 character to account for the leading "."
        this_sec_name += 1;
        if (strcmp(secname, this_sec_name) == 0) {
            section = &section_header_table[i];
            break;
        }
    }

    Elf_Phdr* program_header_table = (Elf_Phdr*)(file_memory + elf_header->e_phoff);
    // Find the first PT_LOAD segment
    Elf_Phdr* first_load_segment = NULL;
    for (int i = 0; i < elf_header->e_phnum; i++) {
        if (program_header_table[i].p_type == PT_LOAD) {
            first_load_segment = &program_header_table[i];
            break;
        }
    }

    if (section != NULL && first_load_segment != NULL) {
        uintptr_t elf_load_addr = first_load_segment->p_vaddr
                                  - (first_load_segment->p_vaddr % first_load_segment->p_align);
        result = start_address + (uintptr_t)section->sh_addr - elf_load_addr;
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

#endif

static uintptr_t
get_py_runtime(pid_t pid)
{
    uintptr_t address = search_map_for_section(pid, "PyRuntime", "libpython");
    if (address == 0) {
        // TODO: Differentiate between not found and error
        PyErr_Clear();
        address = search_map_for_section(pid, "PyRuntime", "python");
    }
    return address;
}

static ssize_t
read_memory(pid_t pid, uintptr_t remote_address, size_t len, void* dst)
{
    ssize_t total_bytes = 0;
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
    total_bytes = result;
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
                PyErr_SetString(PyExc_PermissionError, "Not enough permissions to read memory");
                break;
            case KERN_INVALID_ARGUMENT:
                PyErr_SetString(PyExc_PermissionError, "Invalid argument to mach_vm_read_overwrite");
                break;
            default:
                PyErr_SetString(PyExc_RuntimeError, "Unknown error reading memory");
        }
        return -1;
    }
    total_bytes = len;
#else
    PyErr_SetString(
        PyExc_RuntimeError,
        "Memory reading is not supported on this platform");
    return -1;
#endif
    return total_bytes;
}

ssize_t
write_memory(pid_t pid, uintptr_t remote_address, size_t len, const void* src)
{
    ssize_t total_bytes_written = 0;
#if defined(__linux__) && HAVE_PROCESS_VM_READV
    struct iovec local[1];
    struct iovec remote[1];
    ssize_t result = 0;
    ssize_t written = 0;

    do {
        local[0].iov_base = (void*)((char*)src + result);
        local[0].iov_len = len - result;
        remote[0].iov_base = (void*)((char*)remote_address + result);
        remote[0].iov_len = len - result;

        written = process_vm_writev(pid, local, 1, remote, 1, 0);
        if (written < 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }

        result += written;
    } while ((size_t)written != local[0].iov_len);
    total_bytes_written = result;
#elif defined(__APPLE__) && TARGET_OS_OSX
    kern_return_t kr = mach_vm_write(
            pid_to_task(pid),
            (mach_vm_address_t)remote_address,
            (vm_offset_t)src,
            (mach_msg_type_number_t)len);

    if (kr != KERN_SUCCESS) {
        switch (kr) {
            case KERN_PROTECTION_FAILURE:
                PyErr_SetString(PyExc_PermissionError, "Not enough permissions to write memory");
                break;
            case KERN_INVALID_ARGUMENT:
                PyErr_SetString(PyExc_PermissionError, "Invalid argument to mach_vm_write");
                break;
            default:
                PyErr_SetString(PyExc_RuntimeError, "Unknown error writing memory");
        }
        return -1;
    }
    total_bytes_written = len;
#else
    PyErr_Format(PyExc_RuntimeError, "Writing memory is not supported on this platform");
    return -1;
#endif
    return total_bytes_written;
}

static int
read_offsets(
    int pid,
    uintptr_t *runtime_start_address,
    _Py_DebugOffsets* debug_offsets
) {
    *runtime_start_address = get_py_runtime(pid);
    if (!*runtime_start_address) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(
                PyExc_RuntimeError, "Failed to get .PyRuntime address");
        }
        return -1;
    }
    size_t size = sizeof(struct _Py_DebugOffsets);
    ssize_t bytes = read_memory(
        pid, *runtime_start_address, size, debug_offsets);
    if (bytes == -1) {
        return -1;
    }
    return 0;
}

int
_PySysRemoteDebug_SendExec(int pid, int tid, const char *debugger_script_path)
{
#if (!defined(__linux__) && !defined(__APPLE__)) || (defined(__linux__) && !HAVE_PROCESS_VM_READV)
    PyErr_SetString(PyExc_RuntimeError, "get_stack_trace is not supported on this platform");
    return -1;
#endif
    if (debugger_script_path != NULL && strlen(debugger_script_path) > PATH_MAX) {
        PyErr_SetString(PyExc_ValueError, "Debugger script path is too long");
        return -1;
    }

    uintptr_t runtime_start_address = get_py_runtime(pid);
    if (runtime_start_address == 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to get .PyRuntime address");
        }
        return -1;
    }
    struct _Py_DebugOffsets local_debug_offsets;

    if (read_offsets(pid, &runtime_start_address, &local_debug_offsets)) {
        return -1;
    }

    off_t interpreter_state_list_head = local_debug_offsets.runtime_state.interpreters_head;

    uintptr_t address_of_interpreter_state;
    int bytes = read_memory(
            pid,
            runtime_start_address + interpreter_state_list_head,
            sizeof(void*),
            &address_of_interpreter_state);
    if (bytes == -1) {
        return -1;
    }

    if (address_of_interpreter_state == 0) {
        PyErr_SetString(PyExc_RuntimeError, "No interpreter state found");
        return -1;
    }

    int is_remote_debugging_enabled = 0;
    bytes = read_memory(
            pid,
            address_of_interpreter_state + local_debug_offsets.debugger_support.remote_debugging_enabled,
            sizeof(int),
            &is_remote_debugging_enabled);
    if (bytes == -1) {
        return -1;
    }
    if (is_remote_debugging_enabled == 0) {
        PyErr_SetString(PyExc_RuntimeError, "Remote debugging is not enabled in the remote process");
        return -1;
    }

    uintptr_t address_of_thread;
    bytes = read_memory(
            pid,
            address_of_interpreter_state + local_debug_offsets.interpreter_state.threads_head,
            sizeof(void*),
            &address_of_thread);
    if (bytes == -1) {
        return -1;
    }

    pid_t this_tid = 0;
    if (tid != 0) {
        while (address_of_thread != 0) {
            bytes = read_memory(
                    pid,
                    address_of_thread + local_debug_offsets.thread_state.native_thread_id,
                    sizeof(pid_t),
                    &this_tid);
            if (bytes == -1) {
                return -1;
            }
            if (this_tid == tid) {
                break;
            }
            bytes = read_memory(
                    pid,
                    address_of_thread + local_debug_offsets.thread_state.next,
                    sizeof(void*),
                    &address_of_thread);
            if (bytes == -1) {
                return -1;
            }
        }
    }

    if (address_of_thread == 0) {
        PyErr_SetString(PyExc_RuntimeError, "No thread state found");
        return -1;
    }

    uintptr_t eval_breaker;
    bytes = read_memory(
            pid,
            address_of_thread + local_debug_offsets.debugger_support.eval_breaker,
            sizeof(uintptr_t),
            &eval_breaker);
    if (bytes == -1) {
        return -1;
    }
    eval_breaker |= _PY_EVAL_PLEASE_STOP_BIT;

    // Ensure our path is not too long
    if (local_debug_offsets.debugger_support.debugger_script_path_size <= strlen(debugger_script_path)) {
        PyErr_SetString(PyExc_ValueError, "Debugger script path is too long");
        return -1;
    }

    if (debugger_script_path != NULL) {
        uintptr_t debugger_script_path_addr = (
                address_of_thread +
                local_debug_offsets.debugger_support.remote_debugger_support +
                local_debug_offsets.debugger_support.debugger_script_path);
        bytes = write_memory(
                pid,
                debugger_script_path_addr,
                strlen(debugger_script_path) + 1,
                debugger_script_path);
        if (bytes == -1) {
            return -1;
        }
    }

    int pending_call = 1;
    uintptr_t debugger_pending_call_addr = (
            address_of_thread +
            local_debug_offsets.debugger_support.remote_debugger_support +
            local_debug_offsets.debugger_support.debugger_pending_call);
    bytes = write_memory(
            pid,
            debugger_pending_call_addr,
            sizeof(int),
            &pending_call);

    if (bytes == -1) {
        return -1;
    }

    bytes = write_memory(
            pid,
            address_of_thread + local_debug_offsets.debugger_support.eval_breaker,
            sizeof(uintptr_t),
            &eval_breaker);

    if (bytes == -1) {
        return -1;
    }

    return 0;
}