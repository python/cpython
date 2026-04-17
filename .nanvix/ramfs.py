# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

"""Ramfs image generation for Nanvix CPython.

Replaces ramfs.mk. Provides sysroot trimming and ramfs image building
via mkramfs.elf.
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

import sys as _sys
_sys.path.insert(0, str(Path(__file__).resolve().parent))
from _loader import load_sibling

config = load_sibling("config", __file__)


def trim_sysroot(
    staging: Path,
    *,
    keep_tests: bool = False,
) -> None:
    """Strip dev-only artifacts from a staged sysroot for ramfs packaging.

    Args:
        staging: Root directory containing a ``sysroot/`` subdirectory.
        keep_tests: When True, retain ``lib/python3.12/test/`` (needed
            when building a ramfs for the test pipeline).
    """
    sysroot = staging / "sysroot"
    if not sysroot.is_dir():
        raise FileNotFoundError(f"{sysroot} does not exist")

    print("Trimming sysroot for ramfs...")

    # Remove heavyweight stdlib packages not needed at runtime.
    for reldir in config.SYSROOT_TRIM_DIRS:
        p = sysroot / reldir
        if p.is_dir():
            shutil.rmtree(p)
        elif p.is_file():
            p.unlink()

    # Optionally remove tests.
    if not keep_tests:
        test_dir = sysroot / "lib" / config.PYTHON_LIB_DIR / "test"
        if test_dir.is_dir():
            shutil.rmtree(test_dir)

    # Remove static library.
    lib_a = sysroot / "lib" / f"libpython{config.PYTHON_VERSION}.a"
    if lib_a.is_file():
        lib_a.unlink()

    # Remove dev/config binaries from bin/.
    bin_dir = sysroot / "bin"
    if bin_dir.is_dir():
        for pattern in config.SYSROOT_TRIM_BIN_PATTERNS:
            for match in bin_dir.glob(pattern):
                match.unlink()
        # Remove all ELF binaries.
        for elf in bin_dir.glob("*.elf"):
            elf.unlink()
        # Remove host-native Windows binaries (nanvixd.exe, mkramfs.exe, …).
        # These are host tools, not guest binaries, and waste VM memory.
        for exe in bin_dir.glob("*.exe"):
            exe.unlink()
        # Remove bin/ if empty.
        try:
            bin_dir.rmdir()
        except OSError:
            pass

    # Remove __pycache__ directories.
    for cache_dir in sysroot.rglob("__pycache__"):
        if cache_dir.is_dir():
            shutil.rmtree(cache_dir)


def build_image(
    staging: Path,
    nanvix_home: Path,
    output: Path | None = None,
) -> Path:
    """Build a ramfs image from a trimmed sysroot.

    Args:
        staging: Root directory containing a ``sysroot/`` subdirectory
            (should be trimmed first via :func:`trim_sysroot`).
        nanvix_home: Path to the Nanvix sysroot (contains
            ``bin/mkramfs.elf`` or ``bin/mkramfs.exe``).
        output: Output path for the ramfs image.

    Returns:
        Path to the generated ramfs image.

    Raises:
        ValueError: If *output* is not provided.
    """
    if output is None:
        raise ValueError("output path is required for build_image()")

    mkramfs_name = config.mkramfs_binary()
    mkramfs = nanvix_home / "bin" / mkramfs_name
    if not mkramfs.is_file():
        raise FileNotFoundError(
            f"{mkramfs_name} not found at {mkramfs}. "
            "Run `./z setup` to download required binaries."
        )

    sysroot = staging / "sysroot"
    if not sysroot.is_dir():
        raise FileNotFoundError(f"{sysroot} does not exist")

    subprocess.run(
        [str(mkramfs), "-o", str(output), str(sysroot)],
        check=True,
    )

    size = output.stat().st_size
    human = _human_size(size)
    print(f"Built ramfs image: {output} ({human})")
    return output


def trim_and_build(
    staging: Path,
    nanvix_home: Path,
    output: Path | None = None,
    *,
    keep_tests: bool = False,
) -> Path:
    """Convenience: trim sysroot then build ramfs image."""
    trim_sysroot(staging, keep_tests=keep_tests)
    return build_image(staging, nanvix_home, output)


def _human_size(nbytes: int) -> str:
    size = float(nbytes)
    for unit in ("B", "KB", "MB", "GB"):
        if size < 1024:
            return f"{size:.1f}{unit}"
        size /= 1024
    return f"{size:.1f}TB"
