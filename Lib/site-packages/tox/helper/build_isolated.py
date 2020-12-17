"""PEP 517 build backend invocation script.

It accepts externally parsed build configuration from `[build-system]`
in `pyproject.toml` and invokes an API endpoint for building an sdist
tarball.
"""

import os
import sys


def _ensure_module_in_paths(module, paths):
    """Verify that the imported backend belongs in-tree."""
    if not paths:
        return

    module_path = os.path.normcase(os.path.abspath(module.__file__))
    normalized_paths = (os.path.normcase(os.path.abspath(path)) for path in paths)

    if any(os.path.commonprefix((module_path, path)) == path for path in normalized_paths):
        return

    raise SystemExit(
        "build-backend ({!r}) must exist in one of the paths "
        "specified by backend-path ({!r})".format(module, paths),
    )


dist_folder = sys.argv[1]
backend_spec = sys.argv[2]
backend_obj = sys.argv[3] if len(sys.argv) >= 4 else None
backend_paths = sys.argv[4].split(os.path.pathsep) if sys.argv[4] else []

sys.path[:0] = backend_paths

backend = __import__(backend_spec, fromlist=["_trash"])
_ensure_module_in_paths(backend, backend_paths)
if backend_obj:
    backend = getattr(backend, backend_obj)

basename = backend.build_sdist(dist_folder, {"--global-option": ["--formats=gztar"]})
print(basename)
