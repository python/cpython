/*
IMPORTANT: This header file is full of static functions that are not exported.

The reason is that we don't want to export these functions to the Python API
and they can be used both for the interpreter and some shared libraries. The
reason we don't want to export them is to avoid having them participating in
return-oriented programming attacks.

If you need to add a new function ensure that is declared 'static'.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __clang__
    #define UNUSED __attribute__((unused))
#elif defined(__GNUC__)
    #define UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
    #define UNUSED __pragma(warning(suppress: 4505))
#else
    #define UNUSED
#endif

#if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  error "this header requires Py_BUILD_CORE or Py_BUILD_CORE_MODULE define"
#endif

#include "pyconfig.h"
#include "internal/pycore_ceval.h"

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

#if defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX
#  include <libproc.h>
#  include <mach-o/fat.h>
#  include <mach-o/loader.h>
#  include <mach-o/nlist.h>
#  include <mach/error.h>
#  include <mach/mach.h>
#  include <mach/mach_vm.h>
#  include <mach/machine.h>
#  include <mach/task_info.h>
#  include <sys/mman.h>
#  include <sys/proc.h>
#  include <sys/sysctl.h>
#endif

#ifdef MS_WINDOWS
    // Windows includes and definitions
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef MS_WINDOWS
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifndef HAVE_PROCESS_VM_READV
#    define HAVE_PROCESS_VM_READV 0
#endif

#define _set_debug_exception_cause(exception, format, ...) \
    do { \
        if (!PyErr_ExceptionMatches(PyExc_PermissionError)) { \
            PyThreadState *tstate = _PyThreadState_GET(); \
            if (!_PyErr_Occurred(tstate)) { \
                _PyErr_Format(tstate, exception, format, ##__VA_ARGS__); \
            } else { \
                _PyErr_FormatFromCause(exception, format, ##__VA_ARGS__); \
            } \
        } \
    } while (0)

static inline size_t
get_page_size(void) {
    size_t page_size = 0;
    if (page_size == 0) {
#ifdef MS_WINDOWS
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        page_size = si.dwPageSize;
#else
        page_size = (size_t)getpagesize();
#endif
    }
    return page_size;
}

typedef struct page_cache_entry {
    uintptr_t page_addr; // page-aligned base address
    char *data;
    int valid;
    struct page_cache_entry *next;
} page_cache_entry_t;

#define MAX_PAGES 1024

// Define a platform-independent process handle structure
typedef struct {
    pid_t pid;
#if defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX
    mach_port_t task;
#elif defined(MS_WINDOWS)
    HANDLE hProcess;
#elif defined(__linux__)
    int memfd;
#endif
    page_cache_entry_t pages[MAX_PAGES];
    Py_ssize_t page_size;
} proc_handle_t;

static void
_Py_RemoteDebug_FreePageCache(proc_handle_t *handle)
{
    for (int i = 0; i < MAX_PAGES; i++) {
        PyMem_RawFree(handle->pages[i].data);
        handle->pages[i].data = NULL;
        handle->pages[i].valid = 0;
    }
}

UNUSED static void
_Py_RemoteDebug_ClearCache(proc_handle_t *handle)
{
    for (int i = 0; i < MAX_PAGES; i++) {
        handle->pages[i].valid = 0;
    }
}

#if defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX
static mach_port_t pid_to_task(pid_t pid);
#endif

// Initialize the process handle
static int
_Py_RemoteDebug_InitProcHandle(proc_handle_t *handle, pid_t pid) {
    handle->pid = pid;
#if defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX
    handle->task = pid_to_task(handle->pid);
    if (handle->task == 0) {
        _set_debug_exception_cause(PyExc_RuntimeError, "Failed to initialize macOS process handle");
        return -1;
    }
#elif defined(MS_WINDOWS)
    handle->hProcess = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        FALSE, pid);
    if (handle->hProcess == NULL) {
        PyErr_SetFromWindowsErr(0);
        _set_debug_exception_cause(PyExc_RuntimeError, "Failed to initialize Windows process handle");
        return -1;
    }
#elif defined(__linux__)
    handle->memfd = -1;
#endif
    handle->page_size = get_page_size();
    for (int i = 0; i < MAX_PAGES; i++) {
        handle->pages[i].data = NULL;
        handle->pages[i].valid = 0;
    }
    return 0;
}

// Clean up the process handle
static void
_Py_RemoteDebug_CleanupProcHandle(proc_handle_t *handle) {
#ifdef MS_WINDOWS
    if (handle->hProcess != NULL) {
        CloseHandle(handle->hProcess);
        handle->hProcess = NULL;
    }
#elif defined(__linux__)
    if (handle->memfd != -1) {
        close(handle->memfd);
        handle->memfd = -1;
    }
#endif
    handle->pid = 0;
    _Py_RemoteDebug_FreePageCache(handle);
}

#if defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX

static uintptr_t
return_section_address64(
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
                    PyErr_Format(PyExc_RuntimeError,
                        "mach_vm_region failed while parsing 64-bit Mach-O binary "
                        "at base address 0x%lx (kern_return_t: %d)",
                        base, ret);
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

    return 0;
}

static uintptr_t
return_section_address32(
    const char* section,
    mach_port_t proc_ref,
    uintptr_t base,
    void* map
) {
    struct mach_header* hdr = (struct mach_header*)map;
    int ncmds = hdr->ncmds;

    int cmd_cnt = 0;
    struct segment_command* cmd = map + sizeof(struct mach_header);

    mach_vm_size_t size = 0;
    mach_msg_type_number_t count = sizeof(vm_region_basic_info_data_t);
    mach_vm_address_t address = (mach_vm_address_t)base;
    vm_region_basic_info_data_t r_info;
    mach_port_t object_name;
    uintptr_t vmaddr = 0;

    for (int i = 0; cmd_cnt < 2 && i < ncmds; i++) {
        if (cmd->cmd == LC_SEGMENT && strcmp(cmd->segname, "__TEXT") == 0) {
            vmaddr = cmd->vmaddr;
        }
        if (cmd->cmd == LC_SEGMENT && strcmp(cmd->segname, "__DATA") == 0) {
            while (cmd->filesize != size) {
                address += size;
                kern_return_t ret = mach_vm_region(
                    proc_ref,
                    &address,
                    &size,
                    VM_REGION_BASIC_INFO,
                    (vm_region_info_t)&r_info,  // cppcheck-suppress [uninitvar]
                    &count,
                    &object_name
                );
                if (ret != KERN_SUCCESS) {
                    PyErr_Format(PyExc_RuntimeError,
                        "mach_vm_region failed while parsing 32-bit Mach-O binary "
                        "at base address 0x%lx (kern_return_t: %d)",
                        base, ret);
                    return 0;
                }
            }

            int nsects = cmd->nsects;
            struct section* sec = (struct section*)(
                (void*)cmd + sizeof(struct segment_command)
                );
            for (int j = 0; j < nsects; j++) {
                if (strcmp(sec[j].sectname, section) == 0) {
                    return base + sec[j].addr - vmaddr;
                }
            }
            cmd_cnt++;
        }

        cmd = (struct segment_command*)((void*)cmd + cmd->cmdsize);
    }

    return 0;
}

static uintptr_t
return_section_address_fat(
    const char* section,
    mach_port_t proc_ref,
    uintptr_t base,
    void* map
) {
    struct fat_header* fat_hdr = (struct fat_header*)map;

    // Determine host CPU type for architecture selection
    cpu_type_t cpu;
    int is_abi64;
    size_t cpu_size = sizeof(cpu), abi64_size = sizeof(is_abi64);

    if (sysctlbyname("hw.cputype", &cpu, &cpu_size, NULL, 0) != 0) {
        PyErr_Format(PyExc_OSError,
            "Failed to determine CPU type via sysctlbyname "
            "for fat binary analysis at 0x%lx: %s",
            base, strerror(errno));
        return 0;
    }
    if (sysctlbyname("hw.cpu64bit_capable", &is_abi64, &abi64_size, NULL, 0) != 0) {
        PyErr_Format(PyExc_OSError,
            "Failed to determine CPU ABI capability via sysctlbyname "
            "for fat binary analysis at 0x%lx: %s",
            base, strerror(errno));
        return 0;
    }

    cpu |= is_abi64 * CPU_ARCH_ABI64;

    // Check endianness
    int swap = fat_hdr->magic == FAT_CIGAM;
    struct fat_arch* arch = (struct fat_arch*)(map + sizeof(struct fat_header));

    // Get number of architectures in fat binary
    uint32_t nfat_arch = swap ? __builtin_bswap32(fat_hdr->nfat_arch) : fat_hdr->nfat_arch;

    // Search for matching architecture
    for (uint32_t i = 0; i < nfat_arch; i++) {
        cpu_type_t arch_cpu = swap ? __builtin_bswap32(arch[i].cputype) : arch[i].cputype;

        if (arch_cpu == cpu) {
            // Found matching architecture, now process it
            uint32_t offset = swap ? __builtin_bswap32(arch[i].offset) : arch[i].offset;
            struct mach_header_64* hdr = (struct mach_header_64*)(map + offset);

            // Determine which type of Mach-O it is and process accordingly
            switch (hdr->magic) {
                case MH_MAGIC:
                case MH_CIGAM:
                    return return_section_address32(section, proc_ref, base, (void*)hdr);

                case MH_MAGIC_64:
                case MH_CIGAM_64:
                    return return_section_address64(section, proc_ref, base, (void*)hdr);

                default:
                    PyErr_Format(PyExc_RuntimeError,
                        "Unknown Mach-O magic number 0x%x in fat binary architecture %u at base 0x%lx",
                        hdr->magic, i, base);
                    return 0;
            }
        }
    }

    PyErr_Format(PyExc_RuntimeError,
        "No matching architecture found for CPU type 0x%x "
        "in fat binary at base 0x%lx (%u architectures examined)",
        cpu, base, nfat_arch);
    return 0;
}

static uintptr_t
search_section_in_file(const char* secname, char* path, uintptr_t base, mach_vm_size_t size, mach_port_t proc_ref)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        PyErr_Format(PyExc_OSError,
            "Cannot open binary file '%s' for section '%s' search: %s",
            path, secname, strerror(errno));
        return 0;
    }

    struct stat fs;
    if (fstat(fd, &fs) == -1) {
        PyErr_Format(PyExc_OSError,
            "Cannot get file size for binary '%s' during section '%s' search: %s",
            path, secname, strerror(errno));
        close(fd);
        return 0;
    }

    void* map = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        PyErr_Format(PyExc_OSError,
            "Cannot memory map binary file '%s' (size: %lld bytes) for section '%s' search: %s",
            path, (long long)fs.st_size, secname, strerror(errno));
        close(fd);
        return 0;
    }

    uintptr_t result = 0;
    uint32_t magic = *(uint32_t*)map;

    switch (magic) {
    case MH_MAGIC:
    case MH_CIGAM:
        result = return_section_address32(secname, proc_ref, base, map);
        break;
    case MH_MAGIC_64:
    case MH_CIGAM_64:
        result = return_section_address64(secname, proc_ref, base, map);
        break;
    case FAT_MAGIC:
    case FAT_CIGAM:
        result = return_section_address_fat(secname, proc_ref, base, map);
        break;
    default:
        PyErr_Format(PyExc_RuntimeError,
            "Unrecognized Mach-O magic number 0x%x in binary file '%s' for section '%s' search",
            magic, path, secname);
        break;
    }

    if (munmap(map, fs.st_size) != 0) {
        PyErr_Format(PyExc_OSError,
            "Failed to unmap binary file '%s' (size: %lld bytes): %s",
            path, (long long)fs.st_size, strerror(errno));
        result = 0;
    }
    if (close(fd) != 0) {
        PyErr_Format(PyExc_OSError,
            "Failed to close binary file '%s': %s",
            path, strerror(errno));
        result = 0;
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
        PyErr_Format(PyExc_PermissionError,
            "Cannot get task port for PID %d (kern_return_t: %d). "
            "This typically requires running as root or having the 'com.apple.system-task-ports' entitlement.",
            pid, result);
        return 0;
    }
    return task;
}

static uintptr_t
search_map_for_section(proc_handle_t *handle, const char* secname, const char* substr) {
    mach_vm_address_t address = 0;
    mach_vm_size_t size = 0;
    mach_msg_type_number_t count = sizeof(vm_region_basic_info_data_64_t);
    vm_region_basic_info_data_64_t region_info;
    mach_port_t object_name;

    mach_port_t proc_ref = pid_to_task(handle->pid);
    if (proc_ref == 0) {
        if (!PyErr_Occurred()) {
            PyErr_Format(PyExc_PermissionError,
                "Cannot get task port for PID %d during section search",
                handle->pid);
        }
        return 0;
    }

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
            handle->pid, address, map_filename, MAXPATHLEN);
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

        if (strncmp(filename, substr, strlen(substr)) == 0) {
            uintptr_t result = search_section_in_file(
                secname, map_filename, address, size, proc_ref);
            if (result != 0) {
                return result;
            }
        }

        address += size;
    }

    return 0;
}

#endif // (__APPLE__ && defined(TARGET_OS_OSX) && TARGET_OS_OSX)

#if defined(__linux__) && HAVE_PROCESS_VM_READV
static uintptr_t
search_elf_file_for_section(
        proc_handle_t *handle,
        const char* secname,
        uintptr_t start_address,
        const char *elf_file)
{
    if (start_address == 0) {
        return 0;
    }

    uintptr_t result = 0;
    void* file_memory = NULL;

    int fd = open(elf_file, O_RDONLY);
    if (fd < 0) {
        PyErr_Format(PyExc_OSError,
            "Cannot open ELF file '%s' for section '%s' search: %s",
            elf_file, secname, strerror(errno));
        goto exit;
    }

    struct stat file_stats;
    if (fstat(fd, &file_stats) != 0) {
        PyErr_Format(PyExc_OSError,
            "Cannot get file size for ELF file '%s' during section '%s' search: %s",
            elf_file, secname, strerror(errno));
        goto exit;
    }

    file_memory = mmap(NULL, file_stats.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_memory == MAP_FAILED) {
        PyErr_Format(PyExc_OSError,
            "Cannot memory map ELF file '%s' (size: %lld bytes) for section '%s' search: %s",
            elf_file, (long long)file_stats.st_size, secname, strerror(errno));
        goto exit;
    }

    Elf_Ehdr* elf_header = (Elf_Ehdr*)file_memory;

    // Validate ELF header
    if (elf_header->e_shstrndx >= elf_header->e_shnum) {
        PyErr_Format(PyExc_RuntimeError,
            "Invalid ELF file '%s': string table index %u >= section count %u",
            elf_file, elf_header->e_shstrndx, elf_header->e_shnum);
        goto exit;
    }

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

    if (section == NULL) {
        goto exit;
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

    if (first_load_segment == NULL) {
        PyErr_Format(PyExc_RuntimeError,
            "No PT_LOAD segment found in ELF file '%s' (%u program headers examined)",
            elf_file, elf_header->e_phnum);
        goto exit;
    }

    uintptr_t elf_load_addr = first_load_segment->p_vaddr
        - (first_load_segment->p_vaddr % first_load_segment->p_align);
    result = start_address + (uintptr_t)section->sh_addr - elf_load_addr;

exit:
    if (file_memory != NULL) {
        munmap(file_memory, file_stats.st_size);
    }
    if (fd >= 0 && close(fd) != 0) {
        PyErr_Format(PyExc_OSError,
            "Failed to close ELF file '%s': %s",
            elf_file, strerror(errno));
        result = 0;
    }
    return result;
}

static uintptr_t
search_linux_map_for_section(proc_handle_t *handle, const char* secname, const char* substr)
{
    char maps_file_path[64];
    sprintf(maps_file_path, "/proc/%d/maps", handle->pid);

    FILE* maps_file = fopen(maps_file_path, "r");
    if (maps_file == NULL) {
        PyErr_Format(PyExc_OSError,
            "Cannot open process memory map file '%s' for PID %d section search: %s",
            maps_file_path, handle->pid, strerror(errno));
        return 0;
    }

    size_t linelen = 0;
    size_t linesz = PATH_MAX;
    char *line = PyMem_Malloc(linesz);
    if (!line) {
        fclose(maps_file);
        _set_debug_exception_cause(PyExc_MemoryError,
            "Cannot allocate memory for reading process map file '%s'",
            maps_file_path);
        return 0;
    }

    uintptr_t retval = 0;

    while (fgets(line + linelen, linesz - linelen, maps_file) != NULL) {
        linelen = strlen(line);
        if (line[linelen - 1] != '\n') {
            // Read a partial line: realloc and keep reading where we left off.
            // Note that even the last line will be terminated by a newline.
            linesz *= 2;
            char *biggerline = PyMem_Realloc(line, linesz);
            if (!biggerline) {
                PyMem_Free(line);
                fclose(maps_file);
                _set_debug_exception_cause(PyExc_MemoryError,
                    "Cannot reallocate memory while reading process map file '%s' (attempted size: %zu)",
                    maps_file_path, linesz);
                return 0;
            }
            line = biggerline;
            continue;
        }

        // Read a full line: strip the newline
        line[linelen - 1] = '\0';
        // and prepare to read the next line into the start of the buffer.
        linelen = 0;

        unsigned long start = 0;
        unsigned long path_pos = 0;
        sscanf(line, "%lx-%*x %*s %*s %*s %*s %ln", &start, &path_pos);

        if (!path_pos) {
            // Line didn't match our format string.  This shouldn't be
            // possible, but let's be defensive and skip the line.
            continue;
        }

        const char *path = line + path_pos;
        const char *filename = strrchr(path, '/');
        if (filename) {
            filename++;  // Move past the '/'
        } else {
            filename = path;  // No directories, or an empty string
        }

        if (strstr(filename, substr)) {
            retval = search_elf_file_for_section(handle, secname, start, path);
            if (retval) {
                break;
            }
        }
    }

    PyMem_Free(line);
    if (fclose(maps_file) != 0) {
        PyErr_Format(PyExc_OSError,
            "Failed to close process map file '%s': %s",
            maps_file_path, strerror(errno));
        retval = 0;
    }

    return retval;
}


#endif // __linux__

#ifdef MS_WINDOWS

static int is_process_alive(HANDLE hProcess) {
    DWORD exitCode;
    if (GetExitCodeProcess(hProcess, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return 0;
}

static void* analyze_pe(const wchar_t* mod_path, BYTE* remote_base, const char* secname) {
    HANDLE hFile = CreateFileW(mod_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        PyErr_SetFromWindowsErr(0);
        DWORD error = GetLastError();
        PyErr_Format(PyExc_OSError,
            "Cannot open PE file for section '%s' analysis (error %lu)",
            secname, error);
        return NULL;
    }

    HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, 0);
    if (!hMap) {
        PyErr_SetFromWindowsErr(0);
        DWORD error = GetLastError();
        PyErr_Format(PyExc_OSError,
            "Cannot create file mapping for PE file section '%s' analysis (error %lu)",
            secname, error);
        CloseHandle(hFile);
        return NULL;
    }

    BYTE* mapView = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!mapView) {
        PyErr_SetFromWindowsErr(0);
        DWORD error = GetLastError();
        PyErr_Format(PyExc_OSError,
            "Cannot map view of PE file for section '%s' analysis (error %lu)",
            secname, error);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return NULL;
    }

    IMAGE_DOS_HEADER* pDOSHeader = (IMAGE_DOS_HEADER*)mapView;
    if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        PyErr_Format(PyExc_RuntimeError,
            "Invalid DOS signature (0x%x) in PE file for section '%s' analysis (expected 0x%x)",
            pDOSHeader->e_magic, secname, IMAGE_DOS_SIGNATURE);
        UnmapViewOfFile(mapView);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return NULL;
    }

    IMAGE_NT_HEADERS* pNTHeaders = (IMAGE_NT_HEADERS*)(mapView + pDOSHeader->e_lfanew);
    if (pNTHeaders->Signature != IMAGE_NT_SIGNATURE) {
        PyErr_Format(PyExc_RuntimeError,
            "Invalid NT signature (0x%lx) in PE file for section '%s' analysis (expected 0x%lx)",
            pNTHeaders->Signature, secname, IMAGE_NT_SIGNATURE);
        UnmapViewOfFile(mapView);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return NULL;
    }

    IMAGE_SECTION_HEADER* pSection_header = (IMAGE_SECTION_HEADER*)(mapView + pDOSHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS));
    void* runtime_addr = NULL;

    for (int i = 0; i < pNTHeaders->FileHeader.NumberOfSections; i++) {
        const char* name = (const char*)pSection_header[i].Name;
        if (strncmp(name, secname, IMAGE_SIZEOF_SHORT_NAME) == 0) {
            runtime_addr = remote_base + pSection_header[i].VirtualAddress;
            break;
        }
    }

    UnmapViewOfFile(mapView);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return runtime_addr;
}


static uintptr_t
search_windows_map_for_section(proc_handle_t* handle, const char* secname, const wchar_t* substr) {
    HANDLE hProcSnap;
    do {
        hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, handle->pid);
    } while (hProcSnap == INVALID_HANDLE_VALUE && GetLastError() == ERROR_BAD_LENGTH);

    if (hProcSnap == INVALID_HANDLE_VALUE) {
        PyErr_SetFromWindowsErr(0);
        DWORD error = GetLastError();
        PyErr_Format(PyExc_PermissionError,
            "Unable to create module snapshot for PID %d section '%s' "
            "search (error %lu). Check permissions or PID validity",
            handle->pid, secname, error);
        return 0;
    }

    MODULEENTRY32W moduleEntry;
    moduleEntry.dwSize = sizeof(moduleEntry);
    void* runtime_addr = NULL;

    for (BOOL hasModule = Module32FirstW(hProcSnap, &moduleEntry); hasModule; hasModule = Module32NextW(hProcSnap, &moduleEntry)) {
        // Look for either python executable or DLL
        if (wcsstr(moduleEntry.szModule, substr)) {
            runtime_addr = analyze_pe(moduleEntry.szExePath, moduleEntry.modBaseAddr, secname);
            if (runtime_addr != NULL) {
                break;
            }
        }
    }

    CloseHandle(hProcSnap);

    return (uintptr_t)runtime_addr;
}

#endif // MS_WINDOWS

// Get the PyRuntime section address for any platform
static uintptr_t
_Py_RemoteDebug_GetPyRuntimeAddress(proc_handle_t* handle)
{
    uintptr_t address;

#ifdef MS_WINDOWS
    // On Windows, search for 'python' in executable or DLL
    address = search_windows_map_for_section(handle, "PyRuntime", L"python");
    if (address == 0) {
        // Error out: 'python' substring covers both executable and DLL
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_Format(PyExc_RuntimeError,
            "Failed to find the PyRuntime section in process %d on Windows platform",
            handle->pid);
        _PyErr_ChainExceptions1(exc);
    }
#elif defined(__linux__)
    // On Linux, search for 'python' in executable or DLL
    address = search_linux_map_for_section(handle, "PyRuntime", "python");
    if (address == 0) {
        // Error out: 'python' substring covers both executable and DLL
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_Format(PyExc_RuntimeError,
            "Failed to find the PyRuntime section in process %d on Linux platform",
            handle->pid);
        _PyErr_ChainExceptions1(exc);
    }
#elif defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX
    // On macOS, try libpython first, then fall back to python
    const char* candidates[] = {"libpython", "python", "Python", NULL};
    for (const char** candidate = candidates; *candidate; candidate++) {
        PyErr_Clear();
        address = search_map_for_section(handle, "PyRuntime", *candidate);
        if (address != 0) {
            break;
        }
    }
    if (address == 0) {
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_Format(PyExc_RuntimeError,
            "Failed to find the PyRuntime section in process %d "
            "on macOS platform (tried both libpython and python)",
            handle->pid);
        _PyErr_ChainExceptions1(exc);
    }
#else
    _set_debug_exception_cause(PyExc_RuntimeError,
        "Reading the PyRuntime section is not supported on this platform");
    return 0;
#endif

    return address;
}

#if defined(__linux__) && HAVE_PROCESS_VM_READV

static int
open_proc_mem_fd(proc_handle_t *handle)
{
    char mem_file_path[64];
    sprintf(mem_file_path, "/proc/%d/mem", handle->pid);

    handle->memfd = open(mem_file_path, O_RDWR);
    if (handle->memfd == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        _set_debug_exception_cause(PyExc_OSError,
            "failed to open file %s: %s", mem_file_path, strerror(errno));
        return -1;
    }
    return 0;
}

// Why is pwritev not guarded? Except on Android API level 23 (no longer
// supported), HAVE_PROCESS_VM_READV is sufficient.
static int
read_remote_memory_fallback(proc_handle_t *handle, uintptr_t remote_address, size_t len, void* dst)
{
    if (handle->memfd == -1) {
        if (open_proc_mem_fd(handle) < 0) {
            return -1;
        }
    }

    struct iovec local[1];
    Py_ssize_t result = 0;
    Py_ssize_t read_bytes = 0;

    do {
        local[0].iov_base = (char*)dst + result;
        local[0].iov_len = len - result;
        off_t offset = remote_address + result;

        read_bytes = preadv(handle->memfd, local, 1, offset);
        if (read_bytes < 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            _set_debug_exception_cause(PyExc_OSError,
                "preadv failed for PID %d at address 0x%lx "
                "(size %zu, partial read %zd bytes): %s",
                handle->pid, remote_address + result, len - result, result, strerror(errno));
            return -1;
        }

        result += read_bytes;
    } while ((size_t)read_bytes != local[0].iov_len);
    return 0;
}

#endif // __linux__

// Platform-independent memory read function
static int
_Py_RemoteDebug_ReadRemoteMemory(proc_handle_t *handle, uintptr_t remote_address, size_t len, void* dst)
{
#ifdef MS_WINDOWS
    SIZE_T read_bytes = 0;
    SIZE_T result = 0;
    do {
        if (!ReadProcessMemory(handle->hProcess, (LPCVOID)(remote_address + result), (char*)dst + result, len - result, &read_bytes)) {
            // Check if the process is still alive: we need to be able to tell our caller
            // that the process is dead and not just that the read failed.
            if (!is_process_alive(handle->hProcess)) {
                _set_errno(ESRCH);
                PyErr_SetFromErrno(PyExc_OSError);
                return -1;
            }
            PyErr_SetFromWindowsErr(0);
            DWORD error = GetLastError();
            _set_debug_exception_cause(PyExc_OSError,
                "ReadProcessMemory failed for PID %d at address 0x%lx "
                "(size %zu, partial read %zu bytes): Windows error %lu",
                handle->pid, remote_address + result, len - result, result, error);
            return -1;
        }
        result += read_bytes;
    } while (result < len);
    return 0;
#elif defined(__linux__) && HAVE_PROCESS_VM_READV
    if (handle->memfd != -1) {
        return read_remote_memory_fallback(handle, remote_address, len, dst);
    }
    struct iovec local[1];
    struct iovec remote[1];
    Py_ssize_t result = 0;
    Py_ssize_t read_bytes = 0;

    do {
        local[0].iov_base = (char*)dst + result;
        local[0].iov_len = len - result;
        remote[0].iov_base = (void*)(remote_address + result);
        remote[0].iov_len = len - result;

        read_bytes = process_vm_readv(handle->pid, local, 1, remote, 1, 0);
        if (read_bytes < 0) {
            if (errno == ENOSYS) {
                return read_remote_memory_fallback(handle, remote_address, len, dst);
            }
            PyErr_SetFromErrno(PyExc_OSError);
            if (errno == ESRCH) {
                return -1;
            }
            _set_debug_exception_cause(PyExc_OSError,
                "process_vm_readv failed for PID %d at address 0x%lx "
                "(size %zu, partial read %zd bytes): %s",
                handle->pid, remote_address + result, len - result, result, strerror(errno));
            return -1;
        }

        result += read_bytes;
    } while ((size_t)read_bytes != local[0].iov_len);
    return 0;
#elif defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX
    Py_ssize_t result = -1;
    kern_return_t kr = mach_vm_read_overwrite(
        handle->task,
        (mach_vm_address_t)remote_address,
        len,
        (mach_vm_address_t)dst,
        (mach_vm_size_t*)&result);

    if (kr != KERN_SUCCESS) {
        switch (err_get_code(kr)) {
        case KERN_PROTECTION_FAILURE:
            PyErr_Format(PyExc_PermissionError,
                "Memory protection failure reading from PID %d at address "
                "0x%lx (size %zu): insufficient permissions",
                handle->pid, remote_address, len);
            break;
        case KERN_INVALID_ARGUMENT: {
            // Perform a task_info check to see if the invalid argument is due
            // to the process being terminated
            task_basic_info_data_t task_basic_info;
            mach_msg_type_number_t task_info_count = TASK_BASIC_INFO_COUNT;
            kern_return_t task_valid_check = task_info(handle->task, TASK_BASIC_INFO,
                                                        (task_info_t)&task_basic_info,
                                                        &task_info_count);
            if (task_valid_check == KERN_INVALID_ARGUMENT) {
                PyErr_Format(PyExc_ProcessLookupError,
                    "Process %d is no longer accessible (process terminated)",
                    handle->pid);
            } else {
                PyErr_Format(PyExc_ValueError,
                    "Invalid argument to mach_vm_read_overwrite for PID %d at "
                    "address 0x%lx (size %zu) - check memory permissions",
                    handle->pid, remote_address, len);
            }
            break;
        }
        case KERN_NO_SPACE:
        case KERN_MEMORY_ERROR:
            PyErr_Format(PyExc_ProcessLookupError,
                "Process %d memory space no longer available (process terminated)",
                handle->pid);
            break;
        default:
            PyErr_Format(PyExc_RuntimeError,
                "mach_vm_read_overwrite failed for PID %d at address 0x%lx "
                "(size %zu): kern_return_t %d",
                handle->pid, remote_address, len, kr);
        }
        return -1;
    }
    return 0;
#else
    Py_UNREACHABLE();
#endif
}

UNUSED static int
_Py_RemoteDebug_PagedReadRemoteMemory(proc_handle_t *handle,
                                      uintptr_t addr,
                                      size_t size,
                                      void *out)
{
    size_t page_size = handle->page_size;
    uintptr_t page_base = addr & ~(page_size - 1);
    size_t offset_in_page = addr - page_base;

    if (offset_in_page + size > page_size) {
        return _Py_RemoteDebug_ReadRemoteMemory(handle, addr, size, out);
    }

    // Search for valid cached page
    for (int i = 0; i < MAX_PAGES; i++) {
        page_cache_entry_t *entry = &handle->pages[i];
        if (entry->valid && entry->page_addr == page_base) {
            memcpy(out, entry->data + offset_in_page, size);
            return 0;
        }
    }

    // Find reusable slot
    for (int i = 0; i < MAX_PAGES; i++) {
        page_cache_entry_t *entry = &handle->pages[i];
        if (!entry->valid) {
            if (entry->data == NULL) {
                entry->data = PyMem_RawMalloc(page_size);
                if (entry->data == NULL) {
                    _set_debug_exception_cause(PyExc_MemoryError,
                        "Cannot allocate %zu bytes for page cache entry "
                        "during read from PID %d at address 0x%lx",
                        page_size, handle->pid, addr);
                    return -1;
                }
            }

            if (_Py_RemoteDebug_ReadRemoteMemory(handle, page_base, page_size, entry->data) < 0) {
                // Try to just copy the exact ammount as a fallback
                PyErr_Clear();
                goto fallback;
            }

            entry->page_addr = page_base;
            entry->valid = 1;
            memcpy(out, entry->data + offset_in_page, size);
            return 0;
        }
    }

fallback:
    // Cache full — fallback to uncached read
    return _Py_RemoteDebug_ReadRemoteMemory(handle, addr, size, out);
}

static int
_Py_RemoteDebug_ReadDebugOffsets(
    proc_handle_t *handle,
    uintptr_t *runtime_start_address,
    _Py_DebugOffsets* debug_offsets
) {
    *runtime_start_address = _Py_RemoteDebug_GetPyRuntimeAddress(handle);
    if (!*runtime_start_address) {
        if (!PyErr_Occurred()) {
            PyErr_Format(PyExc_RuntimeError,
                "Failed to locate PyRuntime address for PID %d",
                handle->pid);
        }
        _set_debug_exception_cause(PyExc_RuntimeError, "PyRuntime address lookup failed during debug offsets initialization");
        return -1;
    }
    size_t size = sizeof(struct _Py_DebugOffsets);
    if (0 != _Py_RemoteDebug_ReadRemoteMemory(handle, *runtime_start_address, size, debug_offsets)) {
        _set_debug_exception_cause(PyExc_RuntimeError, "Failed to read debug offsets structure from remote process");
        return -1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
