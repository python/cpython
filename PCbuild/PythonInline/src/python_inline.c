/* 
 * Python Inline Runner - Main Entry Point
 * 
 * A custom Python entry point that can execute inline scripts and files
 * with proper argument handling, platform compatibility, and external package support.
 */

#include "Python.h"
#include "python_inline_config.h"
#include "python_inline_args.h"
#include "python_inline_executor.h"
#include "python_inline_platform.h"

#include <stdio.h>

#ifdef MS_WINDOWS
int
wmain(int argc, wchar_t **argv)
#else
int
main(int argc, char **argv_bytes)
#endif
{
    PythonInlineConfig config;
    int exitcode = PYTHON_INLINE_SUCCESS;
    
#ifndef MS_WINDOWS
    /* Convert char** to wchar_t** for non-Windows platforms */
    wchar_t **argv;
    int conversion_result = python_inline_convert_args(argc, argv_bytes, &argv);
    if (conversion_result != PYTHON_INLINE_SUCCESS) {
        return conversion_result;
    }
#endif

    /* Parse command line arguments */
    int parse_result = python_inline_parse_args(argc, argv, &config);
    if (parse_result != PYTHON_INLINE_SUCCESS) {
        if (parse_result == PYTHON_INLINE_ERROR_ARGS) {
            python_inline_show_usage();
        }
        exitcode = parse_result;
        goto cleanup;
    }
    
    /* Handle help and version requests */
    if (config.show_help) {
        python_inline_show_usage();
        exitcode = PYTHON_INLINE_SUCCESS;
        goto cleanup;
    }
    
    if (config.show_version) {
        python_inline_show_version();
        exitcode = PYTHON_INLINE_SUCCESS;
        goto cleanup;
    }
    
    /* Handle package listing */
    if (config.list_packages) {
        /* Setup site-packages path for listing */
        python_inline_setup_site_packages(&config);
        python_inline_list_packages(&config);
        exitcode = PYTHON_INLINE_SUCCESS;
        goto cleanup;
    }
    
    /* Always setup site-packages path to support bundled packages */
    int setup_result = python_inline_setup_site_packages(&config);
    if (setup_result != PYTHON_INLINE_SUCCESS) {
        fprintf(stderr, "Warning: Failed to setup site-packages path\n");
        /* Continue anyway */
    }
    
    /* Execute the script or file */
    if (config.script != NULL) {
        exitcode = python_inline_execute_script(&config);
    } else {
        /* This should not happen due to argument validation */
        fprintf(stderr, "Internal error: No script specified\n");
        exitcode = PYTHON_INLINE_ERROR_NO_SCRIPT;
    }

cleanup:
    /* Clean up configuration */
    if (config.site_packages_path) {
        free(config.site_packages_path);
    }
    python_inline_config_clear(&config);

#ifndef MS_WINDOWS
    /* Clean up converted arguments for non-Windows platforms */
    python_inline_free_converted_args(argc, argv);
#endif

    return exitcode;
}
