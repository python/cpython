from struct import unpack
from .constants import *
from .logging import *

def calculate_from_build_dir(root):
    candidates = [
        root / PYTHON_DLL_NAME,
        root / FREETHREADED_PYTHON_DLL_NAME,
        *root.glob("*.dll"),
        *root.glob("*.pyd"),
        # Check EXE last because it's easier to have cross-platform EXE
        *root.glob("*.exe"),
    ]

    ARCHS = {
        b"PE\0\0\x4c\x01": "win32",
        b"PE\0\0\x64\x86": "amd64",
        b"PE\0\0\x64\xAA": "arm64"
    }

    first_exc = None
    for pe in candidates:
        try:
            # Read the PE header to grab the machine type
            with open(pe, "rb") as f:
                f.seek(0x3C)
                offset = int.from_bytes(f.read(4), "little")
                f.seek(offset)
                arch = ARCHS[f.read(6)]
        except (FileNotFoundError, PermissionError, LookupError) as ex:
            log_debug("Failed to open {}: {}", pe, ex)
            continue
        log_info("Inferred architecture {} from {}", arch, pe)
        return arch
