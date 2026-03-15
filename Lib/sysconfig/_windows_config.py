
"""
_windows_config.py  (rewritten)

Parse Windows-style pyconfig.h and resolve conditional macros in a way suitable
for use by sysconfig._init_non_posix.

This implementation:
 - improves parsing of inline comments (/* ... */ and // ...)
 - strips surrounding quotes from string macros
 - leaves preprocessor helper macros alone (doesn't try to "evaluate" them)
 - does NOT force MS_WINDOWS_APP/MS_WINDOWS_SYSTEM/MS_WINDOWS_GAMES/MS_WINDOWS_DESKTOP to 1
 - determines pointer/size widths with ctypes when available
 - selects PYD_PLATFORM_TAG from detected MS_WIN64 (amd64) vs win32 default
 - provides get_windows_config_vars(pyconfig_path=None) as public entry point

Intended usage:
    from _windows_config import get_windows_config_vars
    vars = get_windows_config_vars('/path/to/pyconfig.h')  # or None to auto-discover

"""
from __future__ import annotations

import os
import re
import sys
import ctypes
from typing import Dict, Optional, IO

# Macros that should not be treated as configuration results
_WINDOWS_SHOULDNOT_ANALYZE = {
    "_W64",
    "_CRT_SECURE_NO_DEPRECATE",
    "_CRT_NONSTDC_NO_DEPRECATE",
    "Py_BUILD_CORE",
    "_Py_PASTE_VERSION",
    "_Py_STRINGIZE",
    "_Py_STRINGIZE1",
    "_MSC_VER",
    "_WIN32",
    "_WIN64",
    "WINVER",
    "_WIN32_WINNT",
    "NTDDI_VERSION",
    # Preprocessor helpers
    "Py_LL",
    "Py_SOCKET_FD_CAN_BE_GE_FD_SETSIZE",
    "_Py_COMPILER",
    "Py_NTDDI",
}

# Macros that are typically conditional on build/arch and need resolution.
# Note: we removed the MS_WINDOWS_* family from forced conditionals, they should
# only be present if defined in pyconfig.h.
_CONDITION_MACROS_ON_WINDOWS = {
    "USE_SOCKET",
    "HAVE_WINDOWS_CONSOLE_IO",
    "Py_DEBUG",
    "Py_GIL_DISABLED",
    "Py_WINVER",
    "HAVE_PY_SSIZE_T",
    "MS_WIN64",
    "PY_SUPPORT_TIER",
    "PYD_PLATFORM_TAG",
    "PY_SSIZE_T_MAX",
    "PY_LONG_LONG",
    "PY_LLONG_MIN",
    "PY_LLONG_MAX",
    "PY_ULLONG_MAX",
    "Py_ENABLE_SHARED",
    "MS_COREDLL",
    "PLATFORM",
    "SIZEOF_VOID_P",
    "HAVE_LARGEFILE_SUPPORT",
    "SIZEOF_OFF_T",
    "SIZEOF_FPOS_T",
    "SIZEOF_HKEY",
    "SIZEOF_SIZE_T",
    "ALIGNOF_SIZE_T",
    "ALIGNOF_MAX_ALIGN_T",
    "SIZEOF_TIME_T",
    "SIZEOF_SHORT",
    "SIZEOF_INT",
    "SIZEOF_LONG",
    "ALIGNOF_LONG",
    "SIZEOF_LONG_LONG",
    "SIZEOF_DOUBLE",
    "SIZEOF_FLOAT",
    "HAVE_UINTPTR_T",
    "HAVE_INTPTR_T",
    "HAVE_WCSFTIME",
}

# Regex matching "#define NAME VALUE" where VALUE may be empty (no explicit value)
_define_rx = re.compile(r"^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)\s*(.*)$")
_undef_rx = re.compile(r"^\s*/\*\s*#\s*undef\s+([A-Za-z_][A-Za-z0-9_]*)\s*\*/\s*$")

def _strip_trailing_comments(value: str) -> str:
    """Remove '/* ... */' and '//' style trailing comments from a value string."""
    # remove block comment if present
    idx = value.find("/*")
    if idx != -1:
        value = value[:idx]
    # remove // comments
    idx2 = value.find("//")
    if idx2 != -1:
        value = value[:idx2]
    return value.strip()

def parse_config_h(fp: IO[str], vars: Optional[Dict[str, object]] = None) -> Dict[str, object]:
    """Parse a pyconfig.h-like file.

    Similar semantics to sysconfig.parse_config_h but more tolerant to trailing
    comments and unquoted string values. Returns a dict of macro -> value where
    values are ints where reasonable, otherwise strings.
    """
    if vars is None:
        vars = {}
    for raw in fp:
        line = raw.rstrip("\n")
        # skip empty lines and obvious non-def lines
        if not line.strip():
            continue
        m = _define_rx.match(line)
        if m:
            name, val = m.group(1), m.group(2).strip()
            if name in _WINDOWS_SHOULDNOT_ANALYZE or name in _CONDITION_MACROS_ON_WINDOWS:
                continue
            # clean comments and whitespace
            val = _strip_trailing_comments(val)
            if val == "":
                # '#define NAME' with no value -> treat as 1
                vars[name] = 1
                continue
            # remove surrounding quotes if present
            if (val.startswith('"') and val.endswith('"')) or (val.startswith("'") and val.endswith("'")):
                vars[name] = val[1:-1]
                continue
            # if value contains spaces (like macro expressions), keep raw string
            if " " in val and not val.isnumeric():
                vars[name] = val
                continue
            # try integer conversion: hex or decimal
            try:
                if val.startswith(("0x", "0X")):
                    vars[name] = int(val, 16)
                else:
                    vars[name] = int(val)
            except Exception:
                # fallback to raw token (e.g., SIZEOF_INT or LLONG_MAX)
                vars[name] = val
            continue
        # match /* #undef NAME */ style
        um = _undef_rx.match(line)
        if um:
            vars[um.group(1)] = 0
    return vars

def _read_pyconfig_path(default_paths=None) -> Optional[str]:
    candidates = []
    if default_paths:
        candidates.extend(default_paths)
    candidates.append("/mnt/data/pyconfig.h")  # environment helper when testing here
    # try to ask sysconfig for default filename but avoid import cycles if possible
    try:
        import importlib
        sc = importlib.import_module("sysconfig")
        try:
            candidates.append(sc.get_config_h_filename())
        except Exception:
            pass
    except Exception:
        pass
    for c in candidates:
        if not c:
            continue
        if os.path.isabs(c) and os.path.exists(c):
            return c
    return None

def _resolve_conditionals(parsed: Dict[str, object]) -> Dict[str, object]:
    """Resolve conditional macros using runtime/ABI information."""
    vars = dict(parsed)  # copy

    # Determine 64-bit vs 32-bit
    is_64 = sys.maxsize > 2**32
    # Respect pyconfig.h if it explicitly defines MS_WIN64 / MS_WIN32;
    # otherwise set from runtime detection.
    if 'MS_WIN64' not in vars and 'MS_WIN32' not in vars:
        vars['MS_WIN64'] = 1 if is_64 else 0
        vars['MS_WIN32'] = 0 if is_64 else 1
    else:
        # normalize to integers if present
        try:
            vars['MS_WIN64'] = int(vars.get('MS_WIN64', 0))
        except Exception:
            vars['MS_WIN64'] = 1 if is_64 else 0
        try:
            vars['MS_WIN32'] = int(vars.get('MS_WIN32', 0))
        except Exception:
            vars['MS_WIN32'] = 0 if is_64 else 1

    # SIZEOF_VOID_P and SIZEOF_SIZE_T via ctypes where possible
    try:
        vars['SIZEOF_VOID_P'] = ctypes.sizeof(ctypes.c_void_p)
    except Exception:
        vars.setdefault('SIZEOF_VOID_P', 8 if is_64 else 4)
    try:
        vars['SIZEOF_SIZE_T'] = ctypes.sizeof(ctypes.c_size_t)
    except Exception:
        vars.setdefault('SIZEOF_SIZE_T', vars['SIZEOF_VOID_P'])

    # SIZEOF_LONG and SIZEOF_LONG_LONG
    try:
        vars['SIZEOF_LONG'] = ctypes.sizeof(ctypes.c_long)
    except Exception:
        vars.setdefault('SIZEOF_LONG', 4)
    try:
        vars['SIZEOF_LONG_LONG'] = ctypes.sizeof(ctypes.c_longlong)
    except Exception:
        vars.setdefault('SIZEOF_LONG_LONG', 8)

    # ALIGNMENT defaults
    vars.setdefault('ALIGNOF_SIZE_T', vars.get('SIZEOF_SIZE_T', 8))
    vars.setdefault('ALIGNOF_MAX_ALIGN_T', vars.get('ALIGNOF_MAX_ALIGN_T', 8))
    vars.setdefault('ALIGNOF_LONG', vars.get('ALIGNOF_LONG', vars.get('SIZEOF_LONG', 4)))

    # SIZEOF_TIME_T: prefer existing; otherwise assume 8 on modern Windows
    if 'SIZEOF_TIME_T' not in vars:
        vars['SIZEOF_TIME_T'] = 8

    # SIZEOF_HKEY / OFF_T / FPOS_T defaults per architecture
    if vars.get('MS_WIN64'):
        vars.setdefault('SIZEOF_HKEY', 8)
        vars.setdefault('SIZEOF_OFF_T', 4)
        vars.setdefault('SIZEOF_FPOS_T', 8)
        vars.setdefault('HAVE_LARGEFILE_SUPPORT', 1)
    else:
        vars.setdefault('SIZEOF_HKEY', 4)
        vars.setdefault('SIZEOF_OFF_T', 4)
        vars.setdefault('SIZEOF_FPOS_T', 8)
        vars.setdefault('HAVE_LARGEFILE_SUPPORT', 1)

    # Py_DEBUG detection: rely on sys.gettotalrefcount heuristic if not explicit
    if 'Py_DEBUG' not in vars:
        vars['Py_DEBUG'] = 1 if hasattr(sys, 'gettotalrefcount') else 0
    else:
        try:
            vars['Py_DEBUG'] = int(vars['Py_DEBUG'])
        except Exception:
            vars['Py_DEBUG'] = 1 if vars['Py_DEBUG'] else 0

    # Py_GIL_DISABLED normalization (0 or 1)
    if 'Py_GIL_DISABLED' in vars:
        try:
            vars['Py_GIL_DISABLED'] = 1 if int(vars['Py_GIL_DISABLED']) != 0 else 0
        except Exception:
            # if it's a textual token, leave as-is (some pyconfig.hs use non-zero tokens)
            pass
    else:
        vars['Py_GIL_DISABLED'] = 0

    # Py_ENABLE_SHARED default if not present
    if 'Py_ENABLE_SHARED' not in vars:
        vars['Py_ENABLE_SHARED'] = 1

    # HAVE_PY_SSIZE_T and PY_SSIZE_T_MAX
    vars.setdefault('HAVE_PY_SSIZE_T', 1)
    try:
        bit = ctypes.sizeof(ctypes.c_void_p) * 8
        vars['PY_SSIZE_T_MAX'] = (1 << (bit - 1)) - 1
    except Exception:
        vars.setdefault('PY_SSIZE_T_MAX', (1 << 31) - 1)

    # HAVE_UINTPTR_T / HAVE_INTPTR_T best-effort
    vars.setdefault('HAVE_UINTPTR_T', 1)
    vars.setdefault('HAVE_INTPTR_T', 1)

    # HAVE_WCSFTIME default if not present
    vars.setdefault('HAVE_WCSFTIME', 1)

    # Normalize booleans to 0/1
    for k in list(vars.keys()):
        v = vars[k]
        if isinstance(v, bool):
            vars[k] = 1 if v else 0

    # PYD_PLATFORM_TAG fallback based on detected arch if not defined or obviously wrong
    if 'PYD_PLATFORM_TAG' not in vars or not vars['PYD_PLATFORM_TAG']:
        vars['PYD_PLATFORM_TAG'] = 'win_amd64' if vars.get('MS_WIN64') else 'win32'
    else:
        # strip possible comment remnants (if value not integer)
        try:
            if isinstance(vars['PYD_PLATFORM_TAG'], str):
                vars['PYD_PLATFORM_TAG'] = vars['PYD_PLATFORM_TAG'].strip().strip('"').strip("'")
        except Exception:
            pass

    return vars

def _post_process_for_sysconfig(vars: Dict[str, object]) -> Dict[str, object]:
    """Add or normalize a few values that sysconfig expects on Windows."""
    res = dict(vars)
    res.setdefault('EXE', '.exe')
    res.setdefault('VERSION', ''.join(map(str, sys.version_info[:2])))
    # BINDIR: use sys.executable dir as best guess (may differ in PCbuild)
    try:
        res.setdefault('BINDIR', os.path.dirname(os.path.realpath(sys.executable)))
    except Exception:
        res.setdefault('BINDIR', os.getcwd())

    installed_base = res.get('installed_base') or getattr(sys, 'base_prefix', sys.prefix)
    res.setdefault('LIBDIR', os.path.realpath(os.path.join(installed_base, 'libs')))
    res.setdefault('LIBDEST', res.get('LIBDIR', installed_base))
    res.setdefault('LDLIBRARY', res.get('LIBRARY', ''))
    res.setdefault('ABIFLAGS', res.get('ABIFLAGS', ''))
    # ABIFLAGS: emulate 't' for Py_GIL_DISABLED and '_d' for debug
    abi_thread = 't' if res.get('Py_GIL_DISABLED') else ''
    dflag = '_d' if res.get('Py_DEBUG') else ''
    res['ABIFLAGS'] = res.get('ABIFLAGS', abi_thread + dflag)

    # TZPATH heuristic
    check_tzpath = os.path.join(res.get('prefix', sys.prefix), 'share', 'zoneinfo')
    res.setdefault('TZPATH', check_tzpath if os.path.exists(check_tzpath) else '')

    return res

def load_pyconfig(pyconfig_path: Optional[str] = None) -> Dict[str, object]:
    path = pyconfig_path or _read_pyconfig_path()
    parsed: Dict[str, object] = {}
    if path and os.path.exists(path):
        with open(path, 'r', encoding='utf-8', errors='replace') as fp:
            parsed = parse_config_h(fp, {})
    else:
        parsed = {}
    # remove macros we decided not to analyze
    for bad in list(_WINDOWS_SHOULDNOT_ANALYZE):
        if bad in parsed:
            parsed.pop(bad, None)
    resolved = _resolve_conditionals(parsed)
    final = _post_process_for_sysconfig(resolved)
    return final

def get_windows_config_vars(pyconfig_path: Optional[str] = None) -> Dict[str, object]:
    """Public API for sysconfig to call when initializing Windows-specific vars."""
    return load_pyconfig(pyconfig_path)

if __name__ == '__main__':
    p = _read_pyconfig_path()
    print('Using pyconfig.h:', p)
    vars = get_windows_config_vars(p)
    keys = sorted(vars.keys())
    for k in keys:
        print(f'{k} = {vars[k]!r}')
