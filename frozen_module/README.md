# Frozen Modules patch for cPython 3.6

## Getting started

1. Get a clean checkout of Python 3.6 and build it.
2. Use either `patch` or `git apply` to apply the patchfile (`frozenmodules.patch`) onto the cPython checkout.
3. Run `setup.py install` (or `setup.py develop`) to install the helper module.
4. Run `python -m frozen_module freeze-startup-modules -o <path_to_cpython_checkout>/Python/frozenmodules.c <path_to_cpython_checkout>/python`. This generates a C file with all the startup modules baked in.
5. Build cPython again.
