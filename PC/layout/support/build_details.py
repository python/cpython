"""
Generate the PEP 739 'build-details.json' document.
"""

import sys
from pathlib import Path

PEP739_SCHEMA_VERSION = '1.0'

ROOT_DIR = Path(
    __file__,  # PC/layout/support/build_details.py
    '..',  # PC/layout/support
    '..',  # PC/layout
    '..',  # PC
    '..',  # <src/install dir>
).resolve()
TOOLS_BUILD_DIR = ROOT_DIR / 'Tools' / 'build'

sys_path = sys.path[:]
try:
    sys.path.insert(0, str(TOOLS_BUILD_DIR))
    import generate_build_details
except ImportError:
    generate_build_details = None
finally:
    sys.path = sys_path
    del sys_path


def write_relative_build_details(out_path, base_path):
    if generate_build_details is None:
        return
    generate_build_details.write_build_details(
        schema_version=PEP739_SCHEMA_VERSION,
        base_path=base_path,
        location=out_path,
    )
