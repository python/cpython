/* Python Inline Execution Engine Implementation */
#include "python_inline_executor.h"
#include "Python.h"

static int
configure_and_initialize_python(const PythonInlineConfig *config, PyConfig *py_config, 
                                const wchar_t *script_or_file, int is_file)
{
    PyStatus status;
    
    PyConfig_InitPythonConfig(py_config);
    
    /* Set basic configuration */
    py_config->interactive = config->interactive;
    py_config->quiet = config->quiet;
    
    /* Set the script or file to run */
    if (is_file) {
        status = PyConfig_SetString(py_config, &py_config->run_filename, script_or_file);
    } else {
        status = PyConfig_SetString(py_config, &py_config->run_command, script_or_file);
    }
    
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(py_config);
        Py_ExitStatusException(status);
        return PYTHON_INLINE_ERROR_PYTHON;
    }
    
    /* Set argv for the script */
    if (config->script_argc > 0 && config->script_argv) {
        status = PyConfig_SetArgv(py_config, config->script_argc, config->script_argv);
        if (PyStatus_Exception(status)) {
            PyConfig_Clear(py_config);
            Py_ExitStatusException(status);
            return PYTHON_INLINE_ERROR_PYTHON;
        }
    }
    
    /* Configure site-packages path if we have any site-packages path */
    if (config->site_packages_path) {
        /* Enable site module and configure site-packages */
        py_config->site_import = 1;
        
        /* Add site-packages to Python path */
        status = PyConfig_SetString(py_config, &py_config->pythonpath_env, config->site_packages_path);
        if (PyStatus_Exception(status)) {
            wprintf(L"Warning: Could not set site-packages path: %ls\n", config->site_packages_path);
            /* Continue anyway */
        }
    }
    
    /* Initialize Python with the configuration */
    status = Py_InitializeFromConfig(py_config);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(py_config);
        Py_ExitStatusException(status);
        return PYTHON_INLINE_ERROR_PYTHON;
    }
    
    return PYTHON_INLINE_SUCCESS;
}

static int
setup_python_paths(const PythonInlineConfig *config)
{
    if (!config->site_packages_path) {
        return PYTHON_INLINE_SUCCESS; /* Nothing to do */
    }
    
    PyObject *sys_module = PyImport_ImportModule("sys");
    if (!sys_module) {
        wprintf(L"Error: Could not import sys module\n");
        return PYTHON_INLINE_ERROR_PYTHON;
    }
    
    PyObject *sys_path = PyObject_GetAttrString(sys_module, "path");
    if (!sys_path) {
        Py_DECREF(sys_module);
        wprintf(L"Error: Could not get sys.path\n");
        return PYTHON_INLINE_ERROR_PYTHON;
    }
    
    /* Add site-packages to sys.path if configured */
    if (config->site_packages_path) {
        PyObject *site_packages_str = PyUnicode_FromWideChar(config->site_packages_path, -1);
        if (site_packages_str) {
            /* Insert at the beginning of sys.path for priority */
            if (PyList_Insert(sys_path, 0, site_packages_str) < 0) {
                wprintf(L"Warning: Could not add site-packages to sys.path\n");
            } /*else {
                if (!config->quiet) {
                    wprintf(L"Added to Python path: %ls\n", config->site_packages_path);
                }
            }*/
            Py_DECREF(site_packages_str);
        }
    }
    
    Py_DECREF(sys_path);
    Py_DECREF(sys_module);
    
    return PYTHON_INLINE_SUCCESS;
}

int 
python_inline_execute_script(const PythonInlineConfig *config)
{
    if (!config || !config->script) {
        return PYTHON_INLINE_ERROR_ARGS;
    }
    
    PyConfig py_config;
    int init_result = configure_and_initialize_python(config, &py_config, 
                                                     config->script, 0);
    PyConfig_Clear(&py_config);
    
    if (init_result != PYTHON_INLINE_SUCCESS) {
        return init_result;
    }
    
    /* Setup Python paths for packages */
    int path_result = setup_python_paths(config);
    if (path_result != PYTHON_INLINE_SUCCESS) {
        wprintf(L"Warning: Failed to setup package paths\n");
        /* Continue anyway */
    }
    
    /* Show package information if not quiet */
    if (!config->quiet && config->site_packages_path) {
        //wprintf(L"Site-packages path configured: %ls\n", config->site_packages_path);
    }
    
    /* Run the main Python logic */
    int exitcode = Py_RunMain();
    return exitcode;
}

int 
python_inline_execute_file(const PythonInlineConfig *config)
{
    if (!config || !config->filename) {
        return PYTHON_INLINE_ERROR_ARGS;
    }
    
    PyConfig py_config;
    int init_result = configure_and_initialize_python(config, &py_config, 
                                                     config->filename, 1);
    PyConfig_Clear(&py_config);
    
    if (init_result != PYTHON_INLINE_SUCCESS) {
        return init_result;
    }
    
    /* Setup Python paths for packages */
    int path_result = setup_python_paths(config);
    if (path_result != PYTHON_INLINE_SUCCESS) {
        wprintf(L"Warning: Failed to setup package paths\n");
        /* Continue anyway */
    }
    
    /* Show package information if not quiet */
    if (!config->quiet && config->site_packages_path) {
        //wprintf(L"Site-packages path configured: %ls\n", config->site_packages_path);
    }
    
    /* Run the main Python logic */
    int exitcode = Py_RunMain();
    return exitcode;
}
