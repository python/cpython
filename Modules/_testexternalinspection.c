#define _GNU_SOURCE

// Add logging macros at the top
#define LOG_ERROR(fmt, ...) fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) fprintf(stdout, "INFO: " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) fprintf(stdout, "DEBUG: " fmt "\n", ##__VA_ARGS__)

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
#include <internal/pycore_debug_offsets.h>
#include <internal/pycore_frame.h>
#include <internal/pycore_stackref.h>

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
    LOG_DEBUG("Entering return_section_address for section %s at base %p", section, (void*)base);
    
    struct mach_header_64* hdr = (struct mach_header_64*)map;
    int ncmds = hdr->ncmds;
    LOG_DEBUG("Found %d load commands", ncmds);

    int cmd_cnt = 0;
    struct segment_command_64* cmd = map + sizeof(struct mach_header_64);

    mach_vm_size_t size = 0;
    mach_msg_type_number_t count = sizeof(vm_region_basic_info_data_64_t);
    mach_vm_address_t address = (mach_vm_address_t)base;
    vm_region_basic_info_data_64_t r_info;
    mach_port_t object_name;
    uintptr_t vmaddr = 0;

    for (int i = 0; cmd_cnt < 2 && i < ncmds; i++) {
        LOG_DEBUG("Processing command %d", i);
        if (cmd->cmd == LC_SEGMENT_64 && strcmp(cmd->segname, "__TEXT") == 0) {
            LOG_INFO("Found __TEXT segment at command %d", i);
            vmaddr = cmd->vmaddr;
        }
        if (cmd->cmd == LC_SEGMENT_64 && strcmp(cmd->segname, "__DATA") == 0) {
            LOG_INFO("Found __DATA segment at command %d", i);
            while (cmd->filesize != size) {
                address += size;
                kern_return_t ret = mach_vm_region(
                    proc_ref,
                    &address,
                    &size,
                    VM_REGION_BASIC_INFO_64,
                    (vm_region_info_t)&r_info,
                    &count,
                    &object_name
                );
                if (ret != KERN_SUCCESS) {
                    LOG_ERROR("mach_vm_region failed");
                    PyErr_SetString(
                        PyExc_RuntimeError, "Cannot get any more VM maps.\n");
                    return 0;
                }
                LOG_DEBUG("VM region found at %p size %zu", (void*)address, (size_t)size);
            }

            int nsects = cmd->nsects;
            struct section_64* sec = (struct section_64*)(
                (void*)cmd + sizeof(struct segment_command_64)
            );
            LOG_DEBUG("Processing %d sections", nsects);
            
            for (int j = 0; j < nsects; j++) {
                if (strcmp(sec[j].sectname, section) == 0) {
                    LOG_INFO("Found target section %s", section);
                    uintptr_t result = base + sec[j].addr - vmaddr;
                    LOG_DEBUG("Returning address %p", (void*)result);
                    return result;
                }
            }
            cmd_cnt++;
        }

        cmd = (struct segment_command_64*)((void*)cmd + cmd->cmdsize);
    }
    LOG_INFO("Section %s not found", section);
    return 0;
}
#endif

#if defined(__APPLE__) && TARGET_OS_OSX
static uintptr_t
search_section_in_file(
    const char* secname,
    char* path,
    uintptr_t base,
    mach_vm_size_t size,
    mach_port_t proc_ref
) {
    LOG_DEBUG("Entering search_section_in_file for %s in %s", secname, path);
    
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        LOG_ERROR("Failed to open binary %s: %s", path, strerror(errno));
        PyErr_Format(PyExc_RuntimeError, "Cannot open binary %s\n", path);
        return 0;
    }

    struct stat fs;
    if (fstat(fd, &fs) == -1) {
        LOG_ERROR("Failed to stat binary %s: %s", path, strerror(errno));
        PyErr_Format(
            PyExc_RuntimeError, "Cannot get size of binary %s\n", path);
        close(fd);
        return 0;
    }
    LOG_DEBUG("Binary size: %lld bytes", (long long)fs.st_size);

    void* map = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        LOG_ERROR("Failed to mmap binary %s: %s", path, strerror(errno));
        PyErr_Format(PyExc_RuntimeError, "Cannot map binary %s\n", path);
        close(fd);
        return 0;
    }
    LOG_DEBUG("Successfully mapped binary at %p", map);

    uintptr_t result = 0;
    struct mach_header_64* hdr = (struct mach_header_64*)map;
    
    LOG_DEBUG("Binary magic: 0x%x", hdr->magic);
    switch (hdr->magic) {
        case MH_MAGIC:
        case MH_CIGAM:
        case FAT_MAGIC:
        case FAT_CIGAM:
            LOG_ERROR("32-bit Mach-O binaries are not supported");
            PyErr_SetString(
                PyExc_RuntimeError,
                "32-bit Mach-O binaries are not supported");
            break;
        case MH_MAGIC_64:
        case MH_CIGAM_64:
            LOG_INFO("Processing 64-bit Mach-O binary");
            result = return_section_address(secname, proc_ref, base, map);
            break;
        default:
            LOG_ERROR("Unknown Mach-O magic: 0x%x", hdr->magic);
            PyErr_SetString(PyExc_RuntimeError, "Unknown Mach-O magic");
            break;
    }

    LOG_DEBUG("Unmapping binary");
    munmap(map, fs.st_size);
    if (close(fd) != 0) {
        LOG_ERROR("Failed to close file descriptor: %s", strerror(errno));
        PyErr_SetFromErrno(PyExc_OSError);
    }

    LOG_DEBUG("Returning result %p", (void*)result);
    return result;
}
#endif

#if defined(__APPLE__) && TARGET_OS_OSX
static mach_port_t
pid_to_task(pid_t pid)
{
    LOG_DEBUG("Converting PID %d to task port", pid);
    
    mach_port_t task;
    kern_return_t result;

    result = task_for_pid(mach_task_self(), pid, &task);
    if (result != KERN_SUCCESS) {
        LOG_ERROR("task_for_pid failed for PID %d", pid);
        PyErr_Format(PyExc_PermissionError, "Cannot get task for PID %d", pid);
        return 0;
    }
    
    LOG_INFO("Successfully got task port for PID %d", pid);
    return task;
}
#endif

#if defined(__APPLE__) && TARGET_OS_OSX
static uintptr_t
search_map_for_section(pid_t pid, const char* secname, const char* substr)
{
    LOG_DEBUG("Searching for section %s with substring %s in PID %d", 
              secname, substr, pid);
    
    mach_vm_address_t address = 0;
    mach_vm_size_t size = 0;
    mach_msg_type_number_t count = sizeof(vm_region_basic_info_data_64_t);
    vm_region_basic_info_data_64_t region_info;
    mach_port_t object_name;

    mach_port_t proc_ref = pid_to_task(pid);
    if (proc_ref == 0) {
        LOG_ERROR("Failed to get task for PID %d", pid);
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
        LOG_DEBUG("Examining region at %p size %zu", (void*)address, (size_t)size);
        
        int path_len = proc_regionfilename(
            pid, address, map_filename, MAXPATHLEN);
        if (path_len == 0) {
            LOG_DEBUG("No filename for region at %p", (void*)address);
            address += size;
            continue;
        }

        if ((region_info.protection & VM_PROT_READ) == 0
            || (region_info.protection & VM_PROT_EXECUTE) == 0) {
            LOG_DEBUG("Region at %p lacks required permissions", (void*)address);
            address += size;
            continue;
        }

        char* filename = strrchr(map_filename, '/');
        if (filename != NULL) {
            filename++;
        } else {
            filename = map_filename;
        }

        LOG_DEBUG("Checking file: %s", filename);
        if (!match_found && strncmp(filename, substr, strlen(substr)) == 0) {
            LOG_INFO("Found matching file: %s", filename);
            match_found = 1;
            return search_section_in_file(
                secname, map_filename, address, size, proc_ref);
        }

        address += size;
    }
    
    LOG_INFO("No matching section found");
    return 0;
}

#elif defined(__linux__)
static uintptr_t
find_map_start_address(pid_t pid, char* result_filename, const char* map)
{
    LOG_DEBUG("Finding map start address for PID %d, map %s", pid, map);
    
    char maps_file_path[64];
    sprintf(maps_file_path, "/proc/%d/maps", pid);

    FILE* maps_file = fopen(maps_file_path, "r");
    if (maps_file == NULL) {
        LOG_ERROR("Failed to open maps file: %s", strerror(errno));
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
            filename++;
        } else {
            filename = map_filename;
        }

        LOG_DEBUG("Checking map entry: %s", filename);
        if (!match_found && strncmp(filename, map, strlen(map)) == 0) {
            LOG_INFO("Found matching map: %s at %lx", filename, start_address);
            match_found = 1;
            result_address = start_address;
            strcpy(result_filename, map_filename);
            break;
        }
    }

    fclose(maps_file);
    LOG_DEBUG("Search completed. Result address: %p", (void*)result_address);

    if (!match_found) {
        LOG_INFO("No matching map found");
        map_filename[0] = '\0';
    }

    return result_address;
}

static uintptr_t
search_map_for_section(pid_t pid, const char* secname, const char* map)
{
    LOG_DEBUG("Searching for section %s in map %s for PID %d", secname, map, pid);
    
    char elf_file[256];
    uintptr_t start_address = find_map_start_address(pid, elf_file, map);

    if (start_address == 0) {
        LOG_ERROR("Failed to find map start address");
        return 0;
    }
    LOG_INFO("Found start address: %p", (void*)start_address);

    uintptr_t result = 0;
    void* file_memory = NULL;

    int fd = open(elf_file, O_RDONLY);
    if (fd < 0) {
        LOG_ERROR("Failed to open ELF file %s: %s", elf_file, strerror(errno));
        PyErr_SetFromErrno(PyExc_OSError);
        goto exit;
    }

    struct stat file_stats;
    if (fstat(fd, &file_stats) != 0) {
        LOG_ERROR("Failed to stat file: %s", strerror(errno));
        PyErr_SetFromErrno(PyExc_OSError);
        goto exit;
    }
    LOG_DEBUG("File size: %lld bytes", (long long)file_stats.st_size);

    file_memory = mmap(NULL, file_stats.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_memory == MAP_FAILED) {
        LOG_ERROR("Failed to mmap file: %s", strerror(errno));
        PyErr_SetFromErrno(PyExc_OSError);
        goto exit;
    }
    LOG_DEBUG("Successfully mapped file at %p", file_memory);

    Elf_Ehdr* elf_header = (Elf_Ehdr*)file_memory;
    Elf_Shdr* section_header_table = (Elf_Shdr*)(file_memory + elf_header->e_shoff);
    Elf_Shdr* shstrtab_section = &section_header_table[elf_header->e_shstrndx];
    char* shstrtab = (char*)(file_memory + shstrtab_section->sh_offset);

    LOG_DEBUG("Processing %d sections", elf_header->e_shnum);
    Elf_Shdr* section = NULL;
    for (int i = 0; i < elf_header->e_shnum; i++) {
        const char* this_sec_name = (
            shstrtab +
            section_header_table[i].sh_name +
            1  // "+1" accounts for the leading "."
        );
        LOG_DEBUG("Checking section: %s", this_sec_name);

        if (strcmp(secname, this_sec_name) == 0) {
            LOG_INFO("Found target section: %s", secname);
            section = &section_header_table[i];
            break;
        }
    }

    Elf_Phdr* program_header_table = (Elf_Phdr*)(file_memory + elf_header->e_phoff);
    LOG_DEBUG("Processing %d program headers", elf_header->e_phnum);

    // Find the first PT_LOAD segment
    Elf_Phdr* first_load_segment = NULL;
    for (int i = 0; i < elf_header->e_phnum; i++) {
        if (program_header_table[i].p_type == PT_LOAD) {
            LOG_INFO("Found first PT_LOAD segment at index %d", i);
            first_load_segment = &program_header_table[i];
            break;
        }
    }

    if (section != NULL && first_load_segment != NULL) {
        uintptr_t elf_load_addr = first_load_segment->p_vaddr - (
            first_load_segment->p_vaddr % first_load_segment->p_align
        );
        result = start_address + (uintptr_t)section->sh_addr - elf_load_addr;
        LOG_INFO("Calculated result address: %p", (void*)result);
    }
    else {
        LOG_ERROR("Cannot find map for section %s", secname);
        PyErr_Format(PyExc_KeyError, "cannot find map for section %s", secname);
    }

exit:
    if (close(fd) != 0) {
        LOG_ERROR("Failed to close file descriptor: %s", strerror(errno));
        PyErr_SetFromErrno(PyExc_OSError);
    }
    if (file_memory != NULL) {
        munmap(file_memory, file_stats.st_size);
    }
    LOG_DEBUG("Returning result: %p", (void*)result);
    return result;
}
#else
static uintptr_t
search_map_for_section(pid_t pid, const char* secname, const char* map)
{
    LOG_DEBUG("search_map_for_section not implemented on this platform");
    return 0;
}
#endif

static uintptr_t
get_py_runtime(pid_t pid)
{
    LOG_DEBUG("Getting PyRuntime address for PID %d", pid);
    uintptr_t address = search_map_for_section(pid, "PyRuntime", "libpython");
    if (address == 0) {
        LOG_INFO("Trying alternate search for PyRuntime");
        address = search_map_for_section(pid, "PyRuntime", "python");
    }
    LOG_DEBUG("PyRuntime address: %p", (void*)address);
    return address;
}

static uintptr_t
get_async_debug(pid_t pid)
{
    LOG_DEBUG("Getting AsyncioDebug address for PID %d", pid);
    uintptr_t result = search_map_for_section(pid, "AsyncioDebug", "_asyncio.cpython");
    if (result == 0 && !PyErr_Occurred()) {
        LOG_ERROR("Cannot find AsyncioDebug section");
        PyErr_SetString(PyExc_RuntimeError, "Cannot find AsyncioDebug section");
    }
    LOG_DEBUG("AsyncioDebug address: %p", (void*)result);
    return result;
}

static ssize_t
read_memory(pid_t pid, uintptr_t remote_address, size_t len, void* dst)
{
    LOG_DEBUG("Reading memory at %p, length %zu for PID %d", 
              (void*)remote_address, len, pid);
    
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

        LOG_DEBUG("Attempting to read %zu bytes", len - result);
        read = process_vm_readv(pid, local, 1, remote, 1, 0);
        if (read < 0) {
            LOG_ERROR("process_vm_readv failed: %s", strerror(errno));
            PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }
        LOG_DEBUG("Read %zd bytes", read);

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
                LOG_ERROR("Memory protection failure");
                PyErr_SetString(PyExc_PermissionError, 
                              "Not enough permissions to read memory");
                break;
            case KERN_INVALID_ARGUMENT:
                LOG_ERROR("Invalid argument to mach_vm_read_overwrite");
                PyErr_SetString(PyExc_PermissionError, 
                              "Invalid argument to mach_vm_read_overwrite");
                break;
            default:
                LOG_ERROR("Unknown error reading memory: %d", kr);
                PyErr_SetString(PyExc_RuntimeError, "Unknown error reading memory");
        }
        return -1;
    }
    LOG_DEBUG("Successfully read %zd bytes", result);
    total_bytes_read = len;
#else
    LOG_ERROR("read_memory not supported on this platform");
    return -1;
#endif
    LOG_INFO("Total bytes read: %zd", total_bytes_read);
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
    LOG_DEBUG("Reading string at address %p", (void*)address);
    Py_ssize_t len;
    ssize_t bytes_read = read_memory(
        pid,
        address + debug_offsets->unicode_object.length,
        sizeof(Py_ssize_t),
        &len
    );
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read string length");
        return -1;
    }
    LOG_DEBUG("String length: %zd", len);
    
    if (len >= size) {
        LOG_ERROR("Buffer too small for string (need %zd, have %zd)", 
                 len, size);
        PyErr_SetString(PyExc_RuntimeError, "Buffer too small");
        return -1;
    }
    
    size_t offset = debug_offsets->unicode_object.asciiobject_size;
    bytes_read = read_memory(pid, address + offset, len, buffer);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read string content");
        return -1;
    }
    
    buffer[len] = '\0';
    LOG_DEBUG("Successfully read string: '%s'", buffer);
    return 0;
}

static inline int
read_ptr(pid_t pid, uintptr_t address, uintptr_t *ptr_addr)
{
    LOG_DEBUG("Reading pointer at address %p", (void*)address);
    int bytes_read = read_memory(pid, address, sizeof(void*), ptr_addr);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read pointer");
        return -1;
    }
    LOG_DEBUG("Read pointer value: %p", (void*)*ptr_addr);
    return 0;
}

static inline int
read_ssize_t(pid_t pid, uintptr_t address, Py_ssize_t *size)
{
    LOG_DEBUG("Reading Py_ssize_t at address %p", (void*)address);
    int bytes_read = read_memory(pid, address, sizeof(Py_ssize_t), size);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read Py_ssize_t");
        return -1;
    }
    LOG_DEBUG("Read Py_ssize_t value: %zd", *size);
    return 0;
}

static int
read_py_ptr(pid_t pid, uintptr_t address, uintptr_t *ptr_addr)
{
    LOG_DEBUG("Reading Python pointer at address %p", (void*)address);
    if (read_ptr(pid, address, ptr_addr)) {
        LOG_ERROR("Failed to read Python pointer");
        return -1;
    }
    *ptr_addr &= ~Py_TAG_BITS;
    LOG_DEBUG("Read Python pointer value (after mask): %p", (void*)*ptr_addr);
    return 0;
}

static int
read_char(pid_t pid, uintptr_t address, char *result)
{
    LOG_DEBUG("Reading char at address %p", (void*)address);
    int bytes_read = read_memory(pid, address, sizeof(char), result);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read char");
        return -1;
    }
    LOG_DEBUG("Read char value: %d", (int)*result);
    return 0;
}

static int
read_int(pid_t pid, uintptr_t address, int *result)
{
    LOG_DEBUG("Reading int at address %p", (void*)address);
    int bytes_read = read_memory(pid, address, sizeof(int), result);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read int");
        return -1;
    }
    LOG_DEBUG("Read int value: %d", *result);
    return 0;
}

static int
read_unsigned_long(pid_t pid, uintptr_t address, unsigned long *result)
{
    LOG_DEBUG("Reading unsigned long at address %p", (void*)address);
    int bytes_read = read_memory(pid, address, sizeof(unsigned long), result);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read unsigned long");
        return -1;
    }
    LOG_DEBUG("Read unsigned long value: %lu", *result);
    return 0;
}

static int
read_pyobj(pid_t pid, uintptr_t address, PyObject *ptr_addr)
{
    LOG_DEBUG("Reading PyObject at address %p", (void*)address);
    int bytes_read = read_memory(pid, address, sizeof(PyObject), ptr_addr);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read PyObject");
        return -1;
    }
    LOG_DEBUG("Successfully read PyObject");
    return 0;
}

static PyObject *
read_py_str(
    pid_t pid,
    _Py_DebugOffsets* debug_offsets,
    uintptr_t address,
    ssize_t max_len
) {
    LOG_DEBUG("Reading Python string at address %p (max_len: %zd)", 
              (void*)address, max_len);
    assert(max_len > 0);

    PyObject *result = NULL;

    char *buf = (char *)PyMem_RawMalloc(max_len);
    if (buf == NULL) {
        LOG_ERROR("Failed to allocate buffer for string");
        PyErr_NoMemory();
        return NULL;
    }
    if (read_string(pid, debug_offsets, address, buf, max_len)) {
        LOG_ERROR("Failed to read string content");
        goto err;
    }

    result = PyUnicode_FromString(buf);
    if (result == NULL) {
        LOG_ERROR("Failed to create Python string");
        goto err;
    }

    LOG_DEBUG("Successfully read Python string: %s", buf);
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
    LOG_DEBUG("Reading Python long at address %p", (void*)address);
    unsigned int shift = PYLONG_BITS_IN_DIGIT;

    ssize_t size;
    uintptr_t lv_tag;

    int bytes_read = read_memory(
        pid, address + offsets->long_object.lv_tag,
        sizeof(uintptr_t),
        &lv_tag);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read long tag");
        return -1;
    }

    int negative = (lv_tag & 3) == 2;
    size = lv_tag >> 3;

    LOG_DEBUG("Long size: %zd, negative: %d", size, negative);

    if (size == 0) {
        LOG_DEBUG("Long value is 0");
        return 0;
    }

    digit *digits = (digit *)PyMem_RawMalloc(size * sizeof(digit));
    if (!digits) {
        LOG_ERROR("Failed to allocate memory for digits");
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
        LOG_ERROR("Failed to read digits");
        goto error;
    }

    long value = 0;
    LOG_DEBUG("Converting digits to value");

    for (ssize_t i = 0; i < size; ++i) {
        long long factor = digits[i] * (1UL << (ssize_t)(shift * i));
        value += factor;
        LOG_DEBUG("Digit %zd: %u, factor: %lld, running total: %ld", 
                 i, digits[i], factor, value);
    }
    PyMem_RawFree(digits);
    if (negative) {
        value *= -1;
    }
    LOG_INFO("Final long value: %ld", value);
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
    LOG_DEBUG("Parsing task name at address %p", (void*)task_address);
    
    uintptr_t task_name_addr;
    int err = read_py_ptr(
        pid,
        task_address + async_offsets->asyncio_task_object.task_name,
        &task_name_addr);
    if (err) {
        LOG_ERROR("Failed to read task name pointer");
        return NULL;
    }

    PyObject task_name_obj;
    err = read_pyobj(
        pid,
        task_name_addr,
        &task_name_obj);
    if (err) {
        LOG_ERROR("Failed to read task name object");
        return NULL;
    }

    unsigned long flags;
    err = read_unsigned_long(
        pid,
        (uintptr_t)task_name_obj.ob_type + offsets->type_object.tp_flags,
        &flags);
    if (err) {
        LOG_ERROR("Failed to read type flags");
        return NULL;
    }
    LOG_DEBUG("Type flags: %lu", flags);

    if ((flags & Py_TPFLAGS_LONG_SUBCLASS)) {
        LOG_INFO("Task name is a long");
        long res = read_py_long(pid, offsets, task_name_addr);
        if (res == -1) {
            LOG_ERROR("Failed to read long task name");
            PyErr_SetString(PyExc_RuntimeError, "Failed to get task name");
            return NULL;
        }
        return PyUnicode_FromFormat("Task-%d", res);
    }

    if(!(flags & Py_TPFLAGS_UNICODE_SUBCLASS)) {
        LOG_ERROR("Invalid task name object type");
        PyErr_SetString(PyExc_RuntimeError, "Invalid task name object");
        return NULL;
    }

    LOG_INFO("Task name is a string");
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
    LOG_DEBUG("Parsing coroutine chain at address %p", (void*)coro_address);
    assert((void*)coro_address != NULL);

    uintptr_t gen_type_addr;
    int err = read_ptr(
        pid,
        coro_address + sizeof(void*),
        &gen_type_addr);
    if (err) {
        LOG_ERROR("Failed to read generator type address");
        return -1;
    }

    uintptr_t gen_name_addr;
    err = read_py_ptr(
        pid,
        coro_address + offsets->gen_object.gi_name,
        &gen_name_addr);
    if (err) {
        LOG_ERROR("Failed to read generator name address");
        return -1;
    }

    PyObject *name = read_py_str(
        pid,
        offsets,
        gen_name_addr,
        255
    );
    if (name == NULL) {
        LOG_ERROR("Failed to read generator name");
        return -1;
    }

    LOG_DEBUG("Adding name to chain");
    if (PyList_Append(render_to, name)) {
        LOG_ERROR("Failed to append name to list");
        return -1;
    }
    Py_DECREF(name);

    int gi_frame_state;
    err = read_int(
        pid,
        coro_address + offsets->gen_object.gi_frame_state,
        &gi_frame_state);

    if (gi_frame_state == FRAME_SUSPENDED_YIELD_FROM) {
        LOG_DEBUG("Frame is suspended in yield from");
        char owner;
        err = read_char(
            pid,
            coro_address + offsets->gen_object.gi_iframe +
                offsets->interpreter_frame.owner,
            &owner
        );
        if (err) {
            LOG_ERROR("Failed to read frame owner");
            return -1;
        }
        if (owner != FRAME_OWNED_BY_GENERATOR) {
            LOG_ERROR("Invalid frame owner: %d", owner);
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
            LOG_ERROR("Failed to read stack pointer");
            return -1;
        }

        if ((void*)stackpointer_addr != NULL) {
            uintptr_t gi_await_addr;
            err = read_py_ptr(
                pid,
                stackpointer_addr - sizeof(void*),
                &gi_await_addr);
            if (err) {
                LOG_ERROR("Failed to read gi_await address");
                return -1;
            }

            if ((void*)gi_await_addr != NULL) {
                uintptr_t gi_await_addr_type_addr;
                int err = read_ptr(
                    pid,
                    gi_await_addr + sizeof(void*),
                    &gi_await_addr_type_addr);
                if (err) {
                    LOG_ERROR("Failed to read gi_await type address");
                    return -1;
                }

                if (gen_type_addr == gi_await_addr_type_addr) {
                    LOG_DEBUG("Found matching coroutine, continuing chain");
                    err = parse_coro_chain(
                        pid,
                        offsets,
                        async_offsets,
                        gi_await_addr,
                        render_to
                    );
                    if (err) {
                        LOG_ERROR("Failed to parse next coroutine in chain");
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}

static int parse_task_awaited_by(
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
    LOG_DEBUG("Parsing task at address %p", (void*)task_address);

    char is_task;
    int err = read_char(
        pid,
        task_address + async_offsets->asyncio_task_object.task_is_task,
        &is_task);
    if (err) {
        LOG_ERROR("Failed to read is_task flag");
        return -1;
    }

    uintptr_t refcnt;
    read_ptr(pid, task_address + sizeof(Py_ssize_t), &refcnt);
    LOG_DEBUG("Task refcount: %lu", (unsigned long)refcnt);

    PyObject* result = PyList_New(0);
    if (result == NULL) {
        LOG_ERROR("Failed to create new list");
        return -1;
    }

    PyObject *call_stack = PyList_New(0);
    if (call_stack == NULL) {
        LOG_ERROR("Failed to create call stack list");
        goto err;
    }
    if (PyList_Append(result, call_stack)) {
        LOG_ERROR("Failed to append call stack to result");
        Py_DECREF(call_stack);
        goto err;
    }
    Py_DECREF(call_stack);

    if (is_task) {
        LOG_INFO("Processing task object");
        PyObject *tn = parse_task_name(
            pid, offsets, async_offsets, task_address);
        if (tn == NULL) {
            LOG_ERROR("Failed to parse task name");
            goto err;
        }
        if (PyList_Append(result, tn)) {
            LOG_ERROR("Failed to append task name to result");
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
            LOG_ERROR("Failed to read coroutine address");
            goto err;
        }

        if ((void*)coro_addr != NULL) {
            LOG_DEBUG("Parsing coroutine chain");
            err = parse_coro_chain(
                pid,
                offsets,
                async_offsets,
                coro_addr,
                call_stack
            );
            if (err) {
                LOG_ERROR("Failed to parse coroutine chain");
                goto err;
            }

            if (PyList_Reverse(call_stack)) {
                LOG_ERROR("Failed to reverse call stack");
                goto err;
            }
        }
    }

    LOG_DEBUG("Adding task to final result");
    if (PyList_Append(render_to, result)) {
        LOG_ERROR("Failed to append task to render_to list");
        goto err;
    }
    Py_DECREF(result);

    PyObject *awaited_by = PyList_New(0);
    if (awaited_by == NULL) {
        LOG_ERROR("Failed to create awaited_by list");
        goto err;
    }
    if (PyList_Append(result, awaited_by)) {
        LOG_ERROR("Failed to append awaited_by list");
        Py_DECREF(awaited_by);
        goto err;
    }
    Py_DECREF(awaited_by);

    if (parse_task_awaited_by(pid, offsets, async_offsets,
                              task_address, awaited_by)
    ) {
        LOG_ERROR("Failed to parse awaited_by tasks");
        goto err;
    }

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
    LOG_DEBUG("Parsing tasks in set at address %p", (void*)set_addr);

    uintptr_t set_obj;
    if (read_py_ptr(
            pid,
            set_addr,
            &set_obj)
    ) {
        LOG_ERROR("Failed to read set object");
        return -1;
    }

    Py_ssize_t num_els;
    if (read_ssize_t(
            pid,
            set_obj + offsets->set_object.used,
            &num_els)
    ) {
        LOG_ERROR("Failed to read number of elements");
        return -1;
    }
    LOG_DEBUG("Set contains %zd elements", num_els);

    Py_ssize_t set_len;
    if (read_ssize_t(
            pid,
            set_obj + offsets->set_object.mask,
            &set_len)
    ) {
        LOG_ERROR("Failed to read set length");
        return -1;
    }
    set_len++; // The set contains the `mask+1` element slots.
    LOG_DEBUG("Set length (including mask): %zd", set_len);

    uintptr_t table_ptr;
    if (read_ptr(
            pid,
            set_obj + offsets->set_object.table,
            &table_ptr)
    ) {
        LOG_ERROR("Failed to read set table pointer");
        return -1;
    }

    Py_ssize_t i = 0;
    Py_ssize_t els = 0;
    while (i < set_len) {
        uintptr_t key_addr;
        if (read_py_ptr(pid, table_ptr, &key_addr)) {
            LOG_ERROR("Failed to read key address");
            return -1;
        }

        if ((void*)key_addr != NULL) {
            Py_ssize_t ref_cnt;
            if (read_ssize_t(pid, table_ptr, &ref_cnt)) {
                LOG_ERROR("Failed to read reference count");
                return -1;
            }

            if (ref_cnt) {
                LOG_DEBUG("Processing set element %zd", els);
                if (parse_task(
                    pid,
                    offsets,
                    async_offsets,
                    key_addr,
                    awaited_by)
                ) {
                    LOG_ERROR("Failed to parse task in set");
                    return -1;
                }

                if (++els == num_els) {
                    LOG_DEBUG("Processed all elements in set");
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
    LOG_DEBUG("Parsing awaited_by for task at address %p", (void*)task_address);

    uintptr_t task_ab_addr;
    int err = read_py_ptr(
        pid,
        task_address + async_offsets->asyncio_task_object.task_awaited_by,
        &task_ab_addr);
    if (err) {
        LOG_ERROR("Failed to read task_awaited_by address");
        return -1;
    }

    if ((void*)task_ab_addr == NULL) {
        LOG_DEBUG("No awaited_by tasks");
        return 0;
    }

    char awaited_by_is_a_set;
    err = read_char(
        pid,
        task_address + async_offsets->asyncio_task_object.task_awaited_by_is_set,
        &awaited_by_is_a_set);
    if (err) {
        LOG_ERROR("Failed to read awaited_by_is_set flag");
        return -1;
    }

    if (awaited_by_is_a_set) {
        LOG_DEBUG("Processing awaited_by set");
        if (parse_tasks_in_set(
            pid,
            offsets,
            async_offsets,
            task_address + async_offsets->asyncio_task_object.task_awaited_by,
            awaited_by)
         ) {
            LOG_ERROR("Failed to parse tasks in awaited_by set");
            return -1;
        }
    } else {
        LOG_DEBUG("Processing single awaited_by task");
        uintptr_t sub_task;
        if (read_py_ptr(
                pid,
                task_address + async_offsets->asyncio_task_object.task_awaited_by,
                &sub_task)
        ) {
            LOG_ERROR("Failed to read sub-task address");
            return -1;
        }

        if (parse_task(
            pid,
            offsets,
            async_offsets,
            sub_task,
            awaited_by)
        ) {
            LOG_ERROR("Failed to parse sub-task");
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
    LOG_DEBUG("Parsing code object at address %p", (void*)address);

    uintptr_t address_of_function_name;
    int bytes_read = read_memory(
        pid,
        address + offsets->code_object.name,
        sizeof(void*),
        &address_of_function_name
    );
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read function name address");
        return -1;
    }

    if ((void*)address_of_function_name == NULL) {
        LOG_ERROR("No function name found");
        PyErr_SetString(PyExc_RuntimeError, "No function name found");
        return -1;
    }

    PyObject* py_function_name = read_py_str(
        pid, offsets, address_of_function_name, 256);
    if (py_function_name == NULL) {
        LOG_ERROR("Failed to read function name string");
        return -1;
    }

    if (PyList_Append(result, py_function_name) == -1) {
        LOG_ERROR("Failed to append function name to result");
        Py_DECREF(py_function_name);
        return -1;
    }
    LOG_DEBUG("Added function name to result");
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
    LOG_DEBUG("Parsing frame object at address %p", (void*)address);

    int err;

    ssize_t bytes_read = read_memory(
        pid,
        address + offsets->interpreter_frame.previous,
        sizeof(void*),
        previous_frame
    );
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read previous frame pointer");
        return -1;
    }

    char owner;
    if (read_char(pid, address + offsets->interpreter_frame.owner, &owner)) {
        LOG_ERROR("Failed to read frame owner");
        return -1;
    }
    LOG_DEBUG("Frame owner: %d", owner);

    if (owner >= FRAME_OWNED_BY_INTERPRETER) {
        LOG_DEBUG("Frame is owned by interpreter, skipping");
        return 0;
    }

    uintptr_t address_of_code_object;
    err = read_py_ptr(
        pid,
        address + offsets->interpreter_frame.executable,
        &address_of_code_object
    );
    if (err) {
        LOG_ERROR("Failed to read code object address");
        return -1;
    }

    if ((void*)address_of_code_object == NULL) {
        LOG_DEBUG("No code object, skipping frame");
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
    LOG_DEBUG("Parsing async frame object at address %p", (void*)address);

    int err;

    ssize_t bytes_read = read_memory(
        pid,
        address + offsets->interpreter_frame.previous,
        sizeof(void*),
        previous_frame
    );
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read previous frame pointer");
        return -1;
    }

    char owner;
    bytes_read = read_memory(
        pid, address + offsets->interpreter_frame.owner, sizeof(char), &owner);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read frame owner");
        return -1;
    }

    if (owner == FRAME_OWNED_BY_CSTACK || owner == FRAME_OWNED_BY_INTERPRETER) {
        LOG_DEBUG("Frame is owned by C stack or interpreter, skipping");
        return 0;  // C frame
    }

    if (owner != FRAME_OWNED_BY_GENERATOR
        && owner != FRAME_OWNED_BY_THREAD) {
        LOG_ERROR("Unhandled frame owner: %d", owner);
        PyErr_Format(PyExc_RuntimeError, "Unhandled frame owner %d.\n", owner);
        return -1;
    }

    err = read_py_ptr(
        pid,
        address + offsets->interpreter_frame.executable,
        code_object
    );
    if (err) {
        LOG_ERROR("Failed to read code object pointer");
        return -1;
    }

    assert(code_object != NULL);
    if ((void*)*code_object == NULL) {
        LOG_DEBUG("No code object, skipping frame");
        return 0;
    }

    if (parse_code_object(
        pid, result, offsets, *code_object, previous_frame)) {
        LOG_ERROR("Failed to parse code object");
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
    LOG_DEBUG("Reading debug offsets for PID %d", pid);
    
    *runtime_start_address = get_py_runtime(pid);
    if ((void*)*runtime_start_address == NULL) {
        if (!PyErr_Occurred()) {
            LOG_ERROR("Failed to get .PyRuntime address");
            PyErr_SetString(
                PyExc_RuntimeError, "Failed to get .PyRuntime address");
        }
        return -1;
    }
    LOG_INFO("PyRuntime address: %p", (void*)*runtime_start_address);

    size_t size = sizeof(struct _Py_DebugOffsets);
    ssize_t bytes_read = read_memory(
        pid, *runtime_start_address, size, debug_offsets);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read debug offsets");
        return -1;
    }
    LOG_DEBUG("Successfully read debug offsets");
    return 0;
}

static int
read_async_debug(
    int pid,
    struct _Py_AsyncioModuleDebugOffsets* async_debug
) {
    LOG_DEBUG("Reading async debug info for PID %d", pid);
    
    uintptr_t async_debug_addr = get_async_debug(pid);
    if (!async_debug_addr) {
        LOG_ERROR("Failed to get AsyncioDebug address");
        return -1;
    }
    LOG_INFO("AsyncioDebug address: %p", (void*)async_debug_addr);

    size_t size = sizeof(struct _Py_AsyncioModuleDebugOffsets);
    ssize_t bytes_read = read_memory(
        pid, async_debug_addr, size, async_debug);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read async debug info");
        return -1;
    }
    LOG_DEBUG("Successfully read async debug info");
    return 0;
}

static int
find_running_frame(
    int pid,
    uintptr_t runtime_start_address,
    _Py_DebugOffsets* local_debug_offsets,
    uintptr_t *frame
) {
    LOG_DEBUG("Finding running frame for PID %d", pid);
    
    off_t interpreter_state_list_head =
        local_debug_offsets->runtime_state.interpreters_head;

    uintptr_t address_of_interpreter_state;
    int bytes_read = read_memory(
            pid,
            runtime_start_address + interpreter_state_list_head,
            sizeof(void*),
            &address_of_interpreter_state);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read interpreter state address");
        return -1;
    }

    if (address_of_interpreter_state == 0) {
        LOG_ERROR("No interpreter state found");
        PyErr_SetString(PyExc_RuntimeError, "No interpreter state found");
        return -1;
    }
    LOG_INFO("Found interpreter state at %p", (void*)address_of_interpreter_state);

    uintptr_t address_of_thread;
    bytes_read = read_memory(
            pid,
            address_of_interpreter_state +
                local_debug_offsets->interpreter_state.threads_head,
            sizeof(void*),
            &address_of_thread);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read thread state address");
        return -1;
    }

    // No Python frames are available for us (can happen at tear-down).
    if ((void*)address_of_thread != NULL) {
        LOG_DEBUG("Reading current frame from thread state");
        int err = read_ptr(
            pid,
            address_of_thread + local_debug_offsets->thread_state.current_frame,
            frame);
        if (err) {
            LOG_ERROR("Failed to read current frame pointer");
            return -1;
        }
        LOG_INFO("Current frame: %p", (void*)*frame);
        return 0;
    }

    LOG_DEBUG("No thread state found, setting frame to NULL");
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
    LOG_DEBUG("Finding running task for PID %d", pid);
    
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
        LOG_ERROR("Failed to read interpreter state address");
        return -1;
    }

    if (address_of_interpreter_state == 0) {
        LOG_ERROR("No interpreter state found");
        PyErr_SetString(PyExc_RuntimeError, "No interpreter state found");
        return -1;
    }
    LOG_INFO("Found interpreter state at %p", (void*)address_of_interpreter_state);

    uintptr_t address_of_thread;
    bytes_read = read_memory(
            pid,
            address_of_interpreter_state +
                local_debug_offsets->interpreter_state.threads_head,
            sizeof(void*),
            &address_of_thread);
    if (bytes_read < 0) {
        LOG_ERROR("Failed to read thread state address");
        return -1;
    }

    uintptr_t address_of_running_loop;
    // No Python frames are available for us (can happen at tear-down).
    if ((void*)address_of_thread == NULL) {
        LOG_DEBUG("No thread state found");
        return 0;
    }

    bytes_read = read_py_ptr(
        pid,
        address_of_thread
        + async_offsets->asyncio_thread_state.asyncio_running_loop,
        &address_of_running_loop);
    if (bytes_read == -1) {
        LOG_ERROR("Failed to read running loop address");
        return -1;
    }

    // no asyncio loop is now running
    if ((void*)address_of_running_loop == NULL) {
        LOG_DEBUG("No running asyncio loop found");
        return 0;
    }

    int err = read_ptr(
        pid,
        address_of_thread
        + async_offsets->asyncio_thread_state.asyncio_running_task,
        running_task_addr);
    if (err) {
        LOG_ERROR("Failed to read running task address");
        return -1;
    }
    LOG_INFO("Running task address: %p", (void*)*running_task_addr);

    return 0;
}

static PyObject*
get_stack_trace(PyObject* self, PyObject* args)
{
    LOG_DEBUG("Entering get_stack_trace");
    
#if (!defined(__linux__) && !defined(__APPLE__)) || \
    (defined(__linux__) && !HAVE_PROCESS_VM_READV)
    LOG_ERROR("get_stack_trace not supported on this platform");
    PyErr_SetString(
        PyExc_RuntimeError,
        "get_stack_trace is not supported on this platform");
    return NULL;
#endif
    int pid;

    if (!PyArg_ParseTuple(args, "i", &pid)) {
        LOG_ERROR("Failed to parse PID argument");
        return NULL;
    }
    LOG_INFO("Getting stack trace for PID %d", pid);

    uintptr_t runtime_start_address = get_py_runtime(pid);
    struct _Py_DebugOffsets local_debug_offsets;

    if (read_offsets(pid, &runtime_start_address, &local_debug_offsets)) {
        LOG_ERROR("Failed to read debug offsets");
        return NULL;
    }

    uintptr_t address_of_current_frame;
    if (find_running_frame(
        pid, runtime_start_address, &local_debug_offsets,
        &address_of_current_frame)
    ) {
        LOG_ERROR("Failed to find running frame");
        return NULL;
    }

    PyObject* result = PyList_New(0);
    if (result == NULL) {
        LOG_ERROR("Failed to create result list");
        return NULL;
    }

    while ((void*)address_of_current_frame != NULL) {
        LOG_DEBUG("Processing frame at %p", (void*)address_of_current_frame);
        if (parse_frame_object(
                    pid,
                    result,
                    &local_debug_offsets,
                    address_of_current_frame,
                    &address_of_current_frame)
            < 0)
        {
            LOG_ERROR("Failed to parse frame object");
            Py_DECREF(result);
            return NULL;
        }
    }

    LOG_DEBUG("Successfully completed stack trace");
    return result;
}

static PyObject*
get_async_stack_trace(PyObject* self, PyObject* args)
{
    LOG_DEBUG("Entering get_async_stack_trace");
    
#if (!defined(__linux__) && !defined(__APPLE__)) || \
    (defined(__linux__) && !HAVE_PROCESS_VM_READV)
    LOG_ERROR("get_async_stack_trace not supported on this platform");
    PyErr_SetString(
        PyExc_RuntimeError,
        "get_stack_trace is not supported on this platform");
    return NULL;
#endif
    int pid;

    if (!PyArg_ParseTuple(args, "i", &pid)) {
        LOG_ERROR("Failed to parse PID argument");
        return NULL;
    }
    LOG_INFO("Getting async stack trace for PID %d", pid);

    uintptr_t runtime_start_address = get_py_runtime(pid);
    struct _Py_DebugOffsets local_debug_offsets;

    if (read_offsets(pid, &runtime_start_address, &local_debug_offsets)) {
        LOG_ERROR("Failed to read debug offsets");
        return NULL;
    }

    struct _Py_AsyncioModuleDebugOffsets local_async_debug;
    if (read_async_debug(pid, &local_async_debug)) {
        LOG_ERROR("Failed to read async debug info");
        return NULL;
    }

    PyObject* result = PyList_New(1);
    if (result == NULL) {
        LOG_ERROR("Failed to create result list");
        return NULL;
    }
    PyObject* calls = PyList_New(0);
    if (calls == NULL) {
        LOG_ERROR("Failed to create calls list");
        return NULL;
    }
    if (PyList_SetItem(result, 0, calls)) { /* steals ref to 'calls' */
        LOG_ERROR("Failed to set calls list in result");
        Py_DECREF(result);
        Py_DECREF(calls);
        return NULL;
    }

    uintptr_t running_task_addr = (uintptr_t)NULL;
    if (find_running_task(
        pid, runtime_start_address, &local_debug_offsets, &local_async_debug,
        &running_task_addr)
    ) {
        LOG_ERROR("Failed to find running task");
        goto result_err;
    }

    if ((void*)running_task_addr == NULL) {
        LOG_ERROR("No running task found");
        PyErr_SetString(PyExc_RuntimeError, "No running task found");
        goto result_err;
    }

    uintptr_t running_coro_addr;
    if (read_py_ptr(
        pid,
        running_task_addr + local_async_debug.asyncio_task_object.task_coro,
        &running_coro_addr
    )) {
        LOG_ERROR("Failed to read running coroutine address");
        goto result_err;
    }

    if ((void*)running_coro_addr == NULL) {
        LOG_ERROR("Running task coroutine is NULL");
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
        LOG_ERROR("Failed to read running task code object address");
        goto result_err;
    }

    if ((void*)address_of_running_task_code_obj == NULL) {
        LOG_ERROR("Running task code object is NULL");
        PyErr_SetString(PyExc_RuntimeError, "Running task code object is NULL");
        goto result_err;
    }

    uintptr_t address_of_current_frame;
    if (find_running_frame(
        pid, runtime_start_address, &local_debug_offsets,
        &address_of_current_frame)
    ) {
        LOG_ERROR("Failed to find running frame");
        goto result_err;
    }

    uintptr_t address_of_code_object;
    while ((void*)address_of_current_frame != NULL) {
        LOG_DEBUG("Processing frame at %p", (void*)address_of_current_frame);
        int res = parse_async_frame_object(
            pid,
            calls,
            &local_debug_offsets,
            address_of_current_frame,
            &address_of_current_frame,
            &address_of_code_object
        );

        if (res < 0) {
            LOG_ERROR("Failed to parse async frame object");
            goto result_err;
        }

        if (address_of_code_object == address_of_running_task_code_obj) {
            LOG_DEBUG("Found matching code object, breaking");
            break;
        }
    }

    PyObject *tn = parse_task_name(
        pid, &local_debug_offsets, &local_async_debug, running_task_addr);
    if (tn == NULL) {
        LOG_ERROR("Failed to parse task name");
        goto result_err;
    }
    if (PyList_Append(result, tn)) {
        LOG_ERROR("Failed to append task name to result");
        Py_DECREF(tn);
        goto result_err;
    }
    Py_DECREF(tn);

    PyObject* awaited_by = PyList_New(0);
    if (awaited_by == NULL) {
        LOG_ERROR("Failed to create awaited_by list");
        goto result_err;
    }
    if (PyList_Append(result, awaited_by)) {
        LOG_ERROR("Failed to append awaited_by list to result");
        Py_DECREF(awaited_by);
        goto result_err;
    }
    Py_DECREF(awaited_by);

    if (parse_task_awaited_by(
        pid, &local_debug_offsets, &local_async_debug,
        running_task_addr, awaited_by)
    ) {
        LOG_ERROR("Failed to parse awaited_by tasks");
        goto result_err;
    }

    LOG_DEBUG("Successfully completed async stack trace");
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
    LOG_DEBUG("Initializing _testexternalinspection module");
    
    PyObject* mod = PyModule_Create(&module);
    if (mod == NULL) {
        LOG_ERROR("Failed to create module");
        return NULL;
    }
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(mod, Py_MOD_GIL_NOT_USED);
#endif
    int rc = PyModule_AddIntConstant(
        mod, "PROCESS_VM_READV_SUPPORTED", HAVE_PROCESS_VM_READV);
    if (rc < 0) {
        LOG_ERROR("Failed to add PROCESS_VM_READV_SUPPORTED constant");
        Py_DECREF(mod);
        return NULL;
    }
    LOG_INFO("Successfully initialized module");
    return mod;
}