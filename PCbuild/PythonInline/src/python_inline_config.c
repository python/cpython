/* Python Inline Configuration Implementation */
#include "python_inline_config.h"
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>

#ifdef MS_WINDOWS
#include <windows.h>
#include <shlwapi.h>
#endif

void 
python_inline_config_init(PythonInlineConfig *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(PythonInlineConfig));
    config->interactive = 0;
    config->quiet = 0;
    config->show_help = 0;
    config->show_version = 0;
    config->list_packages = 0;
    config->script = NULL;
    config->filename = NULL;
    config->packages_config = NULL;
    config->script_argc = 0;
    config->script_argv = NULL;
    config->package_count = 0;
    config->site_packages_path = NULL;
}

void 
python_inline_config_clear(PythonInlineConfig *config)
{
    if (!config) return;
    
    /* Note: We don't free script and filename as they point to argv elements */
    /* We also don't free script_argv as it points to portions of argv */
    
    python_inline_config_init(config);
}

int 
python_inline_add_package(PythonInlineConfig *config, const wchar_t *name, const wchar_t *version)
{
    if (!config || !name || config->package_count >= PYTHON_INLINE_MAX_PACKAGES) {
        return PYTHON_INLINE_ERROR_ARGS;
    }
    
    PythonInlinePackage *pkg = &config->packages[config->package_count];
    
    /* Copy package name */
    wcsncpy(pkg->name, name, PYTHON_INLINE_MAX_PACKAGE_NAME_LEN - 1);
    pkg->name[PYTHON_INLINE_MAX_PACKAGE_NAME_LEN - 1] = L'\0';
    
    /* Copy version if provided */
    if (version) {
        wcsncpy(pkg->version, version, 63);
        pkg->version[63] = L'\0';
    } else {
        pkg->version[0] = L'\0';
    }
    
    pkg->required = 1; /* Default to required */
    config->package_count++;
    
    return PYTHON_INLINE_SUCCESS;
}

int 
python_inline_load_packages_config(PythonInlineConfig *config, const wchar_t *config_path)
{
    if (!config || !config_path) {
        return PYTHON_INLINE_ERROR_ARGS;
    }
    
    FILE *file = NULL;
#ifdef MS_WINDOWS
    if (_wfopen_s(&file, config_path, L"r") != 0 || !file) {
#else
    /* Convert to narrow string for non-Windows */
    char narrow_path[1024];
    wcstombs(narrow_path, config_path, sizeof(narrow_path) - 1);
    narrow_path[sizeof(narrow_path) - 1] = '\0';
    file = fopen(narrow_path, "r");
    if (!file) {
#endif
        wprintf(L"Warning: Could not open packages config file: %ls\n", config_path);
        return PYTHON_INLINE_ERROR_PACKAGES;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), file) && config->package_count < PYTHON_INLINE_MAX_PACKAGES) {
        /* Remove newline */
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        /* Skip empty lines and comments */
        if (line[0] == '\0' || line[0] == '#') continue;
        
        /* Parse package specification: name[==version] */
        char *version_sep = strstr(line, "==");
        wchar_t name[PYTHON_INLINE_MAX_PACKAGE_NAME_LEN];
        wchar_t version[64] = L"";
        
        if (version_sep) {
            *version_sep = '\0';
            mbstowcs(name, line, PYTHON_INLINE_MAX_PACKAGE_NAME_LEN - 1);
            mbstowcs(version, version_sep + 2, 63);
        } else {
            mbstowcs(name, line, PYTHON_INLINE_MAX_PACKAGE_NAME_LEN - 1);
        }
        
        python_inline_add_package(config, name, version[0] ? version : NULL);
    }
    
    fclose(file);
    return PYTHON_INLINE_SUCCESS;
}

int 
python_inline_setup_site_packages(PythonInlineConfig *config)
{
    if (!config) return PYTHON_INLINE_ERROR_ARGS;
    
#ifdef MS_WINDOWS
    /* Get the executable directory */
    wchar_t exe_path[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, exe_path, MAX_PATH);
    if (len == 0) return PYTHON_INLINE_ERROR_PYTHON;
    
    /* Remove the executable name to get directory */
    PathRemoveFileSpecW(exe_path);
    
    /* Append site-packages subdirectory */
    PathAppendW(exe_path, L"site-packages");
    
    /* Allocate and store the path */
    size_t path_len = wcslen(exe_path) + 1;
    config->site_packages_path = malloc(path_len * sizeof(wchar_t));
    if (!config->site_packages_path) {
        return PYTHON_INLINE_ERROR_MEMORY;
    }
    
    wcscpy(config->site_packages_path, exe_path);
#else
    /* For non-Windows, use a relative path */
    const wchar_t *relative_path = L"./site-packages";
    size_t path_len = wcslen(relative_path) + 1;
    config->site_packages_path = malloc(path_len * sizeof(wchar_t));
    if (!config->site_packages_path) {
        return PYTHON_INLINE_ERROR_MEMORY;
    }
    
    wcscpy(config->site_packages_path, relative_path);
#endif
    
    return PYTHON_INLINE_SUCCESS;
}

void 
python_inline_list_packages(const PythonInlineConfig *config)
{
    if (!config) return;
    
    int installed_count = 0;
    
    // First, try to read installed packages from manifest
    if (config->site_packages_path) {
        wchar_t manifest_path[MAX_PATH];
        wcscpy(manifest_path, config->site_packages_path);
#ifdef MS_WINDOWS
        wcscat(manifest_path, L"\\python_inline_packages.txt");
#else
        wcscat(manifest_path, L"/python_inline_packages.txt");
#endif
        
        FILE *manifest = NULL;
#ifdef MS_WINDOWS
        if (_wfopen_s(&manifest, manifest_path, L"r") == 0 && manifest) {
#else
        // Convert to narrow string for non-Windows
        char narrow_manifest_path[1024];
        wcstombs(narrow_manifest_path, manifest_path, sizeof(narrow_manifest_path) - 1);
        narrow_manifest_path[sizeof(narrow_manifest_path) - 1] = '\0';
        manifest = fopen(narrow_manifest_path, "r");
        if (manifest) {
#endif
            wprintf(L"Explicitly bundled packages:\n");
            char line[512];
            
            while (fgets(line, sizeof(line), manifest)) {
                // Remove newline
                char *newline = strchr(line, '\n');
                if (newline) *newline = '\0';
                
                // Skip empty lines and comments
                if (line[0] == '\0' || line[0] == '#') continue;
                
                // Convert to wide string and print
                wchar_t wide_line[512];
                mbstowcs(wide_line, line, sizeof(wide_line) / sizeof(wchar_t) - 1);
                wide_line[sizeof(wide_line) / sizeof(wchar_t) - 1] = L'\0';
                wprintf(L"  %ls\n", wide_line);
                installed_count++;
            }
            fclose(manifest);
            
            if (installed_count == 0) {
                wprintf(L"  (no packages in manifest)\n");
            }
            wprintf(L"\n");
        }
        
        // Now scan for all installed packages by looking at .dist-info directories
        wprintf(L"All installed packages:\n");
        
#ifdef MS_WINDOWS
        wchar_t search_pattern[MAX_PATH];
        wcscpy(search_pattern, config->site_packages_path);
        wcscat(search_pattern, L"\\*.dist-info");
        
        WIN32_FIND_DATAW find_data;
        HANDLE find_handle = FindFirstFileW(search_pattern, &find_data);
        
        int total_packages = 0;
        if (find_handle != INVALID_HANDLE_VALUE) {
            do {
                if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // Extract package name and version from directory name
                    // Format is typically: package_name-version.dist-info
                    wchar_t *dist_info_pos = wcsstr(find_data.cFileName, L".dist-info");
                    if (dist_info_pos) {
                        *dist_info_pos = L'\0';  // Temporarily null-terminate
                        
                        // Find the last dash to separate name from version
                        wchar_t *last_dash = wcsrchr(find_data.cFileName, L'-');
                        if (last_dash) {
                            *last_dash = L'\0';
                            wchar_t *package_name = find_data.cFileName;
                            wchar_t *version = last_dash + 1;
                            wprintf(L"  %ls==%ls\n", package_name, version);
                        } else {
                            wprintf(L"  %ls\n", find_data.cFileName);
                        }
                        *dist_info_pos = L'.';  // Restore the string
                        if (last_dash) *last_dash = L'-';  // Restore the string
                        total_packages++;
                    }
                }
            } while (FindNextFileW(find_handle, &find_data));
            FindClose(find_handle);
        }
        
        if (total_packages == 0) {
            wprintf(L"  (no packages found in site-packages)\n");
        }
#else
        // For non-Windows, use a simpler approach
        wprintf(L"  (package scanning not implemented for this platform)\n");
#endif
        wprintf(L"\n");
    }
    
    // Then show runtime configured packages if any
    if (config->package_count > 0) {
        wprintf(L"Runtime configured packages (%d):\n", config->package_count);
        for (int i = 0; i < config->package_count; i++) {
            const PythonInlinePackage *pkg = &config->packages[i];
            if (pkg->version[0]) {
                wprintf(L"  %ls==%ls%ls\n", pkg->name, pkg->version, 
                       pkg->required ? L" (required)" : L" (optional)");
            } else {
                wprintf(L"  %ls%ls\n", pkg->name, 
                       pkg->required ? L" (required)" : L" (optional)");
            }
        }
        wprintf(L"\n");
    }
    
    if (config->site_packages_path) {
        wprintf(L"Site-packages path: %ls\n", config->site_packages_path);
    }
}
