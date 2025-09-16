/* Python Inline Execution Engine */
#ifndef PYTHON_INLINE_EXECUTOR_H
#define PYTHON_INLINE_EXECUTOR_H

#include "python_inline_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Execute inline script with given configuration */
int python_inline_execute_script(const PythonInlineConfig *config);

/* Execute file script with given configuration */
int python_inline_execute_file(const PythonInlineConfig *config);

#ifdef __cplusplus
}
#endif

#endif /* PYTHON_INLINE_EXECUTOR_H */
