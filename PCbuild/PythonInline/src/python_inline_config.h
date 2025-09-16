/* Python Inline Configuration and Types */
#ifndef PYTHON_INLINE_CONFIG_H
#define PYTHON_INLINE_CONFIG_H

#include "Python.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of allowed packages */
#define PYTHON_INLINE_MAX_PACKAGES 100
#define PYTHON_INLINE_MAX_PACKAGE_NAME_LEN 256

/* Package configuration structure */
typedef struct {
    wchar_t name[PYTHON_INLINE_MAX_PACKAGE_NAME_LEN];
    wchar_t version[64];  /* Optional version specification */
    int required;         /* 1 if package is required, 0 if optional */
} PythonInlinePackage;

/* Configuration structure for Python Inline execution */
typedef struct {
    int interactive;      /* Run in interactive mode after script */
    int quiet;           /* Suppress version and copyright messages */
    int show_help;       /* Show help message */
    int show_version;    /* Show version information */
    int list_packages;   /* List available packages */
    wchar_t *script;     /* Inline script to execute */
    wchar_t *filename;   /* Script file to execute */
    wchar_t *packages_config; /* Path to packages configuration file */
    int script_argc;     /* Number of arguments for the script */
    wchar_t **script_argv; /* Arguments to pass to the script */
    
    /* Package management */
    PythonInlinePackage packages[PYTHON_INLINE_MAX_PACKAGES];
    int package_count;   /* Number of configured packages */
    wchar_t *site_packages_path; /* Path to bundled site-packages */
} PythonInlineConfig;

/* Return codes */
#define PYTHON_INLINE_SUCCESS           0
#define PYTHON_INLINE_ERROR_ARGS        1
#define PYTHON_INLINE_ERROR_NO_SCRIPT   2
#define PYTHON_INLINE_ERROR_MEMORY      3
#define PYTHON_INLINE_ERROR_PYTHON      4
#define PYTHON_INLINE_ERROR_PACKAGES    5

/* Function declarations */
void python_inline_config_init(PythonInlineConfig *config);
void python_inline_config_clear(PythonInlineConfig *config);

/* Package management functions */
int python_inline_load_packages_config(PythonInlineConfig *config, const wchar_t *config_path);
int python_inline_add_package(PythonInlineConfig *config, const wchar_t *name, const wchar_t *version);
int python_inline_setup_site_packages(PythonInlineConfig *config);
void python_inline_list_packages(const PythonInlineConfig *config);

#ifdef __cplusplus
}
#endif

#endif /* PYTHON_INLINE_CONFIG_H */
