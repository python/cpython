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

#ifndef HAVE_PROCESS_VM_READV
#    define HAVE_PROCESS_VM_READV 0
#endif

#if defined(__APPLE__) && TARGET_OS_OSX
static void*
analyze_macho64(mach_port_t proc_ref, void* base, void* map)
{
    struct mach_header_64* hdr = (struct mach_header_64*)map;
    int ncmds = hdr->ncmds;

    int cmd_cnt = 0;
    struct segment_command_64* cmd = map + sizeof(struct mach_header_64);

    mach_vm_size_t size = 0;
    mach_msg_type_number_t count = sizeof(vm_region_basic_info_data_64_t);
    mach_vm_address_t address = (mach_vm_address_t)base;
    vm_region_basic_info_data_64_t region_info;
    mach_port_t object_name;

    for (int i = 0; cmd_cnt < 2 && i < ncmds; i++) {
        if (cmd->cmd == LC_SEGMENT_64 && strcmp(cmd->segname, "__DATA") == 0) {
            while (cmd->filesize != size) {
                address += size;
                if (mach_vm_region(
                            proc_ref,
                            &address,
                            &size,
                            VM_REGION_BASIC_INFO_64,
                            (vm_region_info_t)&region_info,  // cppcheck-suppress [uninitvar]
                            &count,
                            &object_name)
                    != KERN_SUCCESS)
                {
                    PyErr_SetString(PyExc_RuntimeError, "Cannot get any more VM maps.\n");
                    return NULL;
                }
            }
            base = (void*)address - cmd->vmaddr;

            int nsects = cmd->nsects;
            struct section_64* sec =
                    (struct section_64*)((void*)cmd + sizeof(struct segment_command_64));
            for (int j = 0; j < nsects; j++) {
                if (strcmp(sec[j].sectname, "PyRuntime") == 0) {
                    return base + sec[j].addr;
                }
            }
            cmd_cnt++;
        }

        cmd = (struct segment_command_64*)((void*)cmd + cmd->cmdsize);
    }
    return NULL;
}

static void*
analyze_macho(char* path, void* base, mach_vm_size_t size, mach_port_t proc_ref)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        PyErr_Format(PyExc_RuntimeError, "Cannot open binary %s\n", path);
        return NULL;
    }

    struct stat fs;
    if (fstat(fd, &fs) == -1) {
        PyErr_Format(PyExc_RuntimeError, "Cannot get size of binary %s\n", path);
        close(fd);
        return NULL;
    }

    void* map = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        PyErr_Format(PyExc_RuntimeError, "Cannot map binary %s\n", path);
        close(fd);
        return NULL;
    }

    void* result = NULL;

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
            result = analyze_macho64(proc_ref, base, map);
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

static void*
get_py_runtime_macos(pid_t pid)
{
    mach_vm_address_t address = 0;
    mach_vm_size_t size = 0;
    mach_msg_type_number_t count = sizeof(vm_region_basic_info_data_64_t);
    vm_region_basic_info_data_64_t region_info;
    mach_port_t object_name;

    mach_port_t proc_ref = pid_to_task(pid);
    if (proc_ref == 0) {
        PyErr_SetString(PyExc_PermissionError, "Cannot get task for PID");
        return NULL;
    }

    int match_found = 0;
    char map_filename[MAXPATHLEN + 1];
    void* result_address = NULL;
    while (mach_vm_region(
                   proc_ref,
                   &address,
                   &size,
                   VM_REGION_BASIC_INFO_64,
                   (vm_region_info_t)&region_info,
                   &count,
                   &object_name)
           == KERN_SUCCESS)
    {
        int path_len = proc_regionfilename(pid, address, map_filename, MAXPATHLEN);
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

        // Check if the filename starts with "python" or "libpython"
        if (!match_found && strncmp(filename, "python", 6) == 0) {
            match_found = 1;
            result_address = analyze_macho(map_filename, (void*)address, size, proc_ref);
        }
        if (strncmp(filename, "libpython", 9) == 0) {
            match_found = 1;
            result_address = analyze_macho(map_filename, (void*)address, size, proc_ref);
            break;
        }

        address += size;
    }
    return result_address;
}
#endif

#ifdef __linux__
void*
find_python_map_start_address(pid_t pid, char* result_filename)
{
    char maps_file_path[64];
    sprintf(maps_file_path, "/proc/%d/maps", pid);

    FILE* maps_file = fopen(maps_file_path, "r");
    if (maps_file == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    int match_found = 0;

    char line[256];
    char map_filename[PATH_MAX];
    void* result_address = 0;
    while (fgets(line, sizeof(line), maps_file) != NULL) {
        unsigned long start_address = 0;
        sscanf(line, "%lx-%*x %*s %*s %*s %*s %s", &start_address, map_filename);
        char* filename = strrchr(map_filename, '/');
        if (filename != NULL) {
            filename++;  // Move past the '/'
        } else {
            filename = map_filename;  // No path, use the whole string
        }

        // Check if the filename starts with "python" or "libpython"
        if (!match_found && strncmp(filename, "python", 6) == 0) {
            match_found = 1;
            result_address = (void*)start_address;
            strcpy(result_filename, map_filename);
        }
        if (strncmp(filename, "libpython", 9) == 0) {
            match_found = 1;
            result_address = (void*)start_address;
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

void*
get_py_runtime_linux(pid_t pid)
{
    char elf_file[256];
    void* start_address = (void*)find_python_map_start_address(pid, elf_file);

    if (start_address == 0) {
        PyErr_SetString(PyExc_RuntimeError, "No memory map associated with python or libpython found");
        return NULL;
    }

    void* result = NULL;
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

    Elf_Shdr* py_runtime_section = NULL;
    for (int i = 0; i < elf_header->e_shnum; i++) {
        if (strcmp(".PyRuntime", shstrtab + section_header_table[i].sh_name) == 0) {
            py_runtime_section = &section_header_table[i];
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

    if (py_runtime_section != NULL && first_load_segment != NULL) {
        uintptr_t elf_load_addr = first_load_segment->p_vaddr
                                  - (first_load_segment->p_vaddr % first_load_segment->p_align);
        result = start_address + py_runtime_section->sh_addr - elf_load_addr;
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

ssize_t
read_memory(pid_t pid, void* remote_address, size_t len, void* dst)
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
    total_bytes_read = len;
#else
    return -1;
#endif
    return total_bytes_read;
}

int
read_string(pid_t pid, _Py_DebugOffsets* debug_offsets, void* address, char* buffer, Py_ssize_t size)
{
    Py_ssize_t len;
    ssize_t bytes_read =
            read_memory(pid, address + debug_offsets->unicode_object.length, sizeof(Py_ssize_t), &len);
    if (bytes_read == -1) {
        return -1;
    }
    if (len >= size) {
        PyErr_SetString(PyExc_RuntimeError, "Buffer too small");
        return -1;
    }
    size_t offset = debug_offsets->unicode_object.asciiobject_size;
    bytes_read = read_memory(pid, address + offset, len, buffer);
    if (bytes_read == -1) {
        return -1;
    }
    buffer[len] = '\0';
    return 0;
}

void*
get_py_runtime(pid_t pid)
{
#if defined(__linux__)
    return get_py_runtime_linux(pid);
#elif defined(__APPLE__) && TARGET_OS_OSX
    return get_py_runtime_macos(pid);
#else
    return NULL;
#endif
}

static int
parse_code_object(
        int pid,
        PyObject* result,
        struct _Py_DebugOffsets* offsets,
        void* address,
        void** previous_frame)
{
    void* address_of_function_name;
    read_memory(
            pid,
            (void*)(address + offsets->code_object.name),
            sizeof(void*),
            &address_of_function_name);

    if (address_of_function_name == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "No function name found");
        return -1;
    }

    char function_name[256];
    if (read_string(pid, offsets, address_of_function_name, function_name, sizeof(function_name)) != 0) {
        return -1;
    }

    PyObject* py_function_name = PyUnicode_FromString(function_name);
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
        void* address,
        void** previous_frame)
{
    ssize_t bytes_read = read_memory(
            pid,
            (void*)(address + offsets->interpreter_frame.previous),
            sizeof(void*),
            previous_frame);
    if (bytes_read == -1) {
        return -1;
    }

    char owner;
    bytes_read =
            read_memory(pid, (void*)(address + offsets->interpreter_frame.owner), sizeof(char), &owner);
    if (bytes_read < 0) {
        return -1;
    }

    if (owner == FRAME_OWNED_BY_CSTACK) {
        return 0;
    }

    void* address_of_code_object;
    bytes_read = read_memory(
            pid,
            (void*)(address + offsets->interpreter_frame.executable),
            sizeof(void*),
            &address_of_code_object);
    if (bytes_read == -1) {
        return -1;
    }

    if (address_of_code_object == NULL) {
        return 0;
    }
    return parse_code_object(pid, result, offsets, address_of_code_object, previous_frame);
}

static PyObject*
get_stack_trace(PyObject* self, PyObject* args)
{
#if (!defined(__linux__) && !defined(__APPLE__)) || (defined(__linux__) && !HAVE_PROCESS_VM_READV)
    PyErr_SetString(PyExc_RuntimeError, "get_stack_trace is not supported on this platform");
    return NULL;
#endif
    int pid;

    if (!PyArg_ParseTuple(args, "i", &pid)) {
        return NULL;
    }

    void* runtime_start_address = get_py_runtime(pid);
    if (runtime_start_address == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to get .PyRuntime address");
        }
        return NULL;
    }
    size_t size = sizeof(struct _Py_DebugOffsets);
    struct _Py_DebugOffsets local_debug_offsets;

    ssize_t bytes_read = read_memory(pid, runtime_start_address, size, &local_debug_offsets);
    if (bytes_read == -1) {
        return NULL;
    }
    off_t interpreter_state_list_head = local_debug_offsets.runtime_state.interpreters_head;

    void* address_of_interpreter_state;
    bytes_read = read_memory(
            pid,
            (void*)(runtime_start_address + interpreter_state_list_head),
            sizeof(void*),
            &address_of_interpreter_state);
    if (bytes_read == -1) {
        return NULL;
    }

    if (address_of_interpreter_state == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "No interpreter state found");
        return NULL;
    }

    void* address_of_thread;
    bytes_read = read_memory(
            pid,
            (void*)(address_of_interpreter_state + local_debug_offsets.interpreter_state.threads_head),
            sizeof(void*),
            &address_of_thread);
    if (bytes_read == -1) {
        return NULL;
    }

    PyObject* result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }

    // No Python frames are available for us (can happen at tear-down).
    if (address_of_thread != NULL) {
        void* address_of_current_frame;
        (void)read_memory(
                pid,
                (void*)(address_of_thread + local_debug_offsets.thread_state.current_frame),
                sizeof(void*),
                &address_of_current_frame);
        while (address_of_current_frame != NULL) {
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
    }

    return result;
}

static PyMethodDef methods[] = {
        {"get_stack_trace", get_stack_trace, METH_VARARGS, "Get the Python stack from a given PID"},
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
    int rc = PyModule_AddIntConstant(mod, "PROCESS_VM_READV_SUPPORTED", HAVE_PROCESS_VM_READV);
    if (rc < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    return mod;
}
