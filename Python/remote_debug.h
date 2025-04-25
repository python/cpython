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

#if defined(__APPLE__) && TARGET_OS_OSX
#  include <libproc.h>
#  include <mach-o/fat.h>
#  include <mach-o/loader.h>
#  include <mach-o/nlist.h>
#  include <mach/mach.h>
#  include <mach/mach_vm.h>
#  include <mach/machine.h>
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

// Define a platform-independent process handle structure
typedef struct {
    pid_t pid;
#ifdef MS_WINDOWS
    HANDLE hProcess;
#endif
} proc_handle_t;

// Initialize the process handle
static int
_Py_RemoteDebug_InitProcHandle(proc_handle_t *handle, pid_t pid) {
    handle->pid = pid;
#ifdef MS_WINDOWS
    handle->hProcess = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        FALSE, pid);
    if (handle->hProcess == NULL) {
        PyErr_SetFromWindowsErr(0);
        return -1;
    }
#endif
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
#endif
    handle->pid = 0;
}

#if defined(__APPLE__) && TARGET_OS_OSX

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
                    PyErr_SetString(
                        PyExc_RuntimeError, "Cannot get any more VM maps.\n");
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

    // We should not be here, but if we are there, we should say about this
    PyErr_SetString(
        PyExc_RuntimeError, "Cannot find section address.\n");
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

    sysctlbyname("hw.cputype", &cpu, &cpu_size, NULL, 0);
    sysctlbyname("hw.cpu64bit_capable", &is_abi64, &abi64_size, NULL, 0);

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
                    PyErr_SetString(PyExc_RuntimeError, "Unknown Mach-O magic in fat binary.\n");
                    return 0;
            }
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "No matching architecture found in fat binary.\n");
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
        PyErr_SetString(PyExc_RuntimeError, "Unknown Mach-O magic");
        break;
    }

    munmap(map, fs.st_size);
    if (close(fd) != 0) {
        PyErr_SetFromErrno(PyExc_OSError);
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
        PyErr_Format(PyExc_PermissionError, "Cannot get task for PID %d", pid);
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
            PyErr_SetString(PyExc_PermissionError, "Cannot get task for PID");
        }
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

#endif // (__APPLE__ && TARGET_OS_OSX)

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
    if (file_memory != NULL) {
        munmap(file_memory, file_stats.st_size);
    }
    if (fd >= 0 && close(fd) != 0) {
        PyErr_SetFromErrno(PyExc_OSError);
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
        PyErr_SetFromErrno(PyExc_OSError);
        return 0;
    }

    size_t linelen = 0;
    size_t linesz = PATH_MAX;
    char *line = PyMem_Malloc(linesz);
    if (!line) {
        fclose(maps_file);
        PyErr_NoMemory();
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
                PyErr_NoMemory();
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
        PyErr_SetFromErrno(PyExc_OSError);
        retval = 0;
    }

    return retval;
}


#endif // __linux__

#ifdef MS_WINDOWS

static void* analyze_pe(const wchar_t* mod_path, BYTE* remote_base, const char* secname) {
    HANDLE hFile = CreateFileW(mod_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        PyErr_SetFromWindowsErr(0);
        return NULL;
    }
    HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, 0);
    if (!hMap) {
        PyErr_SetFromWindowsErr(0);
        CloseHandle(hFile);
        return NULL;
    }

    BYTE* mapView = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!mapView) {
        PyErr_SetFromWindowsErr(0);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return NULL;
    }

    IMAGE_DOS_HEADER* pDOSHeader = (IMAGE_DOS_HEADER*)mapView;
    if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid DOS signature.");
        UnmapViewOfFile(mapView);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return NULL;
    }

    IMAGE_NT_HEADERS* pNTHeaders = (IMAGE_NT_HEADERS*)(mapView + pDOSHeader->e_lfanew);
    if (pNTHeaders->Signature != IMAGE_NT_SIGNATURE) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid NT signature.");
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
        PyErr_SetString(PyExc_PermissionError, "Unable to create module snapshot. Check permissions or PID.");
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
        PyErr_SetString(PyExc_RuntimeError, "Failed to find the PyRuntime section in the process.");
        _PyErr_ChainExceptions1(exc);
    }
#elif defined(__linux__)
    // On Linux, search for 'python' in executable or DLL
    address = search_linux_map_for_section(handle, "PyRuntime", "python");
    if (address == 0) {
        // Error out: 'python' substring covers both executable and DLL
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_RuntimeError, "Failed to find the PyRuntime section in the process.");
        _PyErr_ChainExceptions1(exc);
    }
#elif defined(__APPLE__) && TARGET_OS_OSX
    // On macOS, try libpython first, then fall back to python
    address = search_map_for_section(handle, "PyRuntime", "libpython");
    if (address == 0) {
        // TODO: Differentiate between not found and error
        PyErr_Clear();
        address = search_map_for_section(handle, "PyRuntime", "python");
    }
#else
    Py_UNREACHABLE();
#endif

    return address;
}

// Platform-independent memory read function
static int
_Py_RemoteDebug_ReadRemoteMemory(proc_handle_t *handle, uintptr_t remote_address, size_t len, void* dst)
{
#ifdef MS_WINDOWS
    SIZE_T read_bytes = 0;
    SIZE_T result = 0;
    do {
        if (!ReadProcessMemory(handle->hProcess, (LPCVOID)(remote_address + result), (char*)dst + result, len - result, &read_bytes)) {
            PyErr_SetFromWindowsErr(0);
            return -1;
        }
        result += read_bytes;
    } while (result < len);
    return 0;
#elif defined(__linux__) && HAVE_PROCESS_VM_READV
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
            PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }

        result += read_bytes;
    } while ((size_t)read_bytes != local[0].iov_len);
    return 0;
#elif defined(__APPLE__) && TARGET_OS_OSX
    Py_ssize_t result = -1;
    kern_return_t kr = mach_vm_read_overwrite(
        pid_to_task(handle->pid),
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
    return 0;
#else
    Py_UNREACHABLE();
#endif
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
            PyErr_SetString(
                PyExc_RuntimeError, "Failed to get PyRuntime address");
        }
        return -1;
    }
    size_t size = sizeof(struct _Py_DebugOffsets);
    if (0 != _Py_RemoteDebug_ReadRemoteMemory(handle, *runtime_start_address, size, debug_offsets)) {
        return -1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
