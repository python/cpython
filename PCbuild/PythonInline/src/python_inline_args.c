/* Python Inline Argument Parsing Implementation */
#include "python_inline_args.h"
#include "Python.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>

void 
python_inline_show_usage(void)
{
    printf("Usage: python_inline [OPTIONS] -c \"SCRIPT\" [ARGS...]\n");
    //printf("       python_inline [OPTIONS] --script=\"SCRIPT\" [ARGS...]\n");
    //printf("       python_inline [OPTIONS] --file=FILENAME [ARGS...]\n");
    printf("\n");
    printf("Execute Python code inline or from a file with optional external packages.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -c \"SCRIPT\"          Execute the Python script string\n");
    printf("  -h, --help           Show this help message and exit\n");
    printf("  -v, --version        Show Python version and exit\n");
    printf("  -q, --quiet          Don't print version and copyright messages\n");
    //printf("  -i, --interactive    Inspect interactively after running script\n");
    printf("\n");
    printf("Package Management:\n");
    printf("  --packages=FILE      Load package configuration from file\n");
    printf("  --list-packages      List configured packages and exit\n");
    //printf("  --add-package=NAME   Add a package to the configuration (runtime)\n");
    printf("\n");
    printf("Arguments after the script are passed to the Python script as sys.argv[1:].\n");
    printf("\n");
    printf("Examples:\n");
    printf("  python_inline -c \"print('Hello, World!')\"\n");
    //printf("  python_inline --script=\"import sys; print(sys.version)\"\n");
    //printf("  python_inline --file=script.py arg1 arg2\n");
    //printf("  python_inline --packages=requirements.txt --file=script.py\n");
    //printf("  python_inline --add-package=requests -c \"import requests; print('OK')\"\n");
    printf("  python_inline --list-packages\n");
    printf("\n");
    printf("Package Configuration File Format:\n");
    printf("  # Lines starting with # are comments\n");
    printf("  requests==2.31.0\n");
    printf("  numpy\n");
    printf("  pandas>=1.5.0\n");
}

void 
python_inline_show_version(void)
{
    printf("Python %s on %s\n", Py_GetVersion(), Py_GetPlatform());
}

int 
python_inline_parse_args(int argc, wchar_t **argv, PythonInlineConfig *config)
{
    if (!config) {
        return PYTHON_INLINE_ERROR_ARGS;
    }
    
    python_inline_config_init(config);
    
    int script_start_index = argc; /* Index where script arguments start */
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"-h") == 0 || wcscmp(argv[i], L"--help") == 0) {
            config->show_help = 1;
            return PYTHON_INLINE_SUCCESS;
        }
        else if (wcscmp(argv[i], L"-v") == 0 || wcscmp(argv[i], L"--version") == 0) {
            config->show_version = 1;
            return PYTHON_INLINE_SUCCESS;
        }
        else if (wcscmp(argv[i], L"--list-packages") == 0) {
            config->list_packages = 1;
            return PYTHON_INLINE_SUCCESS;
        }
        else if (wcscmp(argv[i], L"-q") == 0 || wcscmp(argv[i], L"--quiet") == 0) {
            config->quiet = 1;
        }
        else if (wcscmp(argv[i], L"-i") == 0 || wcscmp(argv[i], L"--interactive") == 0) {
            config->interactive = 1;
        }
        else if (wcscmp(argv[i], L"-c") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -c option requires a script argument\n");
                return PYTHON_INLINE_ERROR_ARGS;
            }
            config->script = argv[i + 1];
            script_start_index = i + 2;
            i++; /* Skip the script argument */
            break; /* Remaining arguments are for the script */
        }
        else if (wcsncmp(argv[i], L"--script=", 9) == 0) {
            config->script = argv[i] + 9;
            script_start_index = i + 1;
            break; /* Remaining arguments are for the script */
        }
        else if (wcsncmp(argv[i], L"--file=", 7) == 0) {
            config->filename = argv[i] + 7;
            script_start_index = i + 1;
            break; /* Remaining arguments are for the script */
        }
        else if (wcsncmp(argv[i], L"--packages=", 11) == 0) {
            config->packages_config = argv[i] + 11;
        }
        else if (wcsncmp(argv[i], L"--add-package=", 14) == 0) {
            wchar_t *package_spec = argv[i] + 14;
            wchar_t *version_sep = wcsstr(package_spec, L"==");
            
            if (version_sep) {
                /* Temporarily null-terminate the package name */
                *version_sep = L'\0';
                python_inline_add_package(config, package_spec, version_sep + 2);
                *version_sep = L'='; /* Restore the string */
            } else {
                python_inline_add_package(config, package_spec, NULL);
            }
        }
        else {
            fprintf(stderr, "Error: Unknown option '%ls'\n", argv[i]);
            return PYTHON_INLINE_ERROR_ARGS;
        }
    }
    
    /* Load packages from config file if specified */
    if (config->packages_config) {
        int result = python_inline_load_packages_config(config, config->packages_config);
        if (result != PYTHON_INLINE_SUCCESS && result != PYTHON_INLINE_ERROR_PACKAGES) {
            return result; /* Fatal error */
        }
        /* Continue even if package config loading failed (just a warning) */
    }
    
    /* Check if we have a script or filename to execute (unless showing help/version/packages) */
    if (config->script == NULL && config->filename == NULL && 
        !config->show_help && !config->show_version && !config->list_packages) {
        fprintf(stderr, "Error: No script specified. Use -c, --script=, or --file=\n");
        return PYTHON_INLINE_ERROR_NO_SCRIPT;
    }
    
    /* Set up script arguments */
    if (script_start_index < argc) {
        config->script_argc = argc - script_start_index + 1;
        config->script_argv = argv + script_start_index - 1;
    } else {
        /* Just set program name */
        config->script_argc = 1;
        config->script_argv = argv; /* argv[0] is program name */
    }
    
    return PYTHON_INLINE_SUCCESS;
}
