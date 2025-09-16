/* Python Inline Argument Parsing */
#ifndef PYTHON_INLINE_ARGS_H
#define PYTHON_INLINE_ARGS_H

#include "python_inline_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parse command line arguments and populate configuration */
int python_inline_parse_args(int argc, wchar_t **argv, PythonInlineConfig *config);

/* Show usage information */
void python_inline_show_usage(void);

/* Show version information */
void python_inline_show_version(void);

#ifdef __cplusplus
}
#endif

#endif /* PYTHON_INLINE_ARGS_H */
