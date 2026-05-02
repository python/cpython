# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

"""Shared module loader for sibling .nanvix/ modules.

Because .nanvix/ is not a valid Python package name (leading dot),
modules must be loaded via importlib. This helper centralizes that
logic so it's not duplicated in every module.

Usage from sibling modules::

    import sys
    from pathlib import Path
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from _loader import load_sibling

    config = load_sibling("config", __file__)
"""

from __future__ import annotations

import importlib.util
from pathlib import Path
from typing import Any


def load_sibling(name: str, caller_file: str) -> Any:
    """Load a sibling module from the same directory as *caller_file*.

    Args:
        name: Module name without extension (e.g. ``"config"``).
        caller_file: ``__file__`` of the calling module.

    Returns:
        The loaded module object.

    Raises:
        ImportError: If the module cannot be found or loaded.
    """
    mod_path = Path(caller_file).resolve().parent / f"{name}.py"
    spec = importlib.util.spec_from_file_location(f"nanvix_{name}", mod_path)
    if spec is None or spec.loader is None:
        raise ImportError(f"Cannot load module from {mod_path}")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod
