# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

"""Build orchestration for Nanvix CPython cross-compilation.

Replaces the ``_run_make`` / ``_make_args`` pattern from z.py and the
build/install/clean targets from common.mk. Constructs and executes
``make -f Makefile.nanvix`` commands with correct variables.
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path
from typing import Any

import sys as _sys

_sys.path.insert(0, str(Path(__file__).resolve().parent))
from _loader import load_sibling

config = load_sibling("config", __file__)
docker_mod = load_sibling("docker", __file__)


def make_args(
    sysroot: str | Path,
    toolchain: str | Path,
    *targets: str,
    platform: str = config.DEFAULT_PLATFORM,
    process_mode: str = config.DEFAULT_PROCESS_MODE,
    memory_size: str = config.DEFAULT_MEMORY_SIZE,
    install_prefix: str = config.DEFAULT_INSTALL_PREFIX,
    release: bool = False,
) -> list[str]:
    """Build the make argument list for Makefile.nanvix.

    Args:
        sysroot: Path to the Nanvix sysroot.
        toolchain: Path to the Nanvix cross-compilation toolchain.

    Returns:
        Complete argument list starting with ``make``.
    """
    sysroot_p = Path(sysroot)
    toolchain_p = Path(toolchain)

    args = [
        "make",
        "-f",
        "Makefile.nanvix",
        f"CONFIG_NANVIX=y",
        f"NANVIX_HOME={sysroot_p}",
        f"NANVIX_TOOLCHAIN={toolchain_p}",
        f"PLATFORM={platform}",
        f"PROCESS_MODE={process_mode}",
        f"MEMORY_SIZE={memory_size}",
        f"INSTALL_PREFIX={install_prefix}",
        f"NANVIX_RELEASE={'yes' if release else 'no'}",
    ]
    args.extend(targets)
    return args


def run_make(
    args: list[str],
    *,
    cwd: Path | None = None,
    run_fn: Any = None,
) -> None:
    """Execute a make command.

    Args:
        args: Full make argument list (from :func:`make_args`).
        cwd: Working directory.
        run_fn: Callable to execute the command. Defaults to
            ``subprocess.run`` with check=True.
    """
    if run_fn:
        run_fn(*args, cwd=cwd)
    else:
        subprocess.run(args, cwd=cwd, check=True)


def build(
    sysroot: str | Path,
    toolchain: str | Path,
    repo_root: Path,
    *,
    platform: str = config.DEFAULT_PLATFORM,
    process_mode: str = config.DEFAULT_PROCESS_MODE,
    memory_size: str = config.DEFAULT_MEMORY_SIZE,
    install_prefix: str = config.DEFAULT_INSTALL_PREFIX,
    release: bool = False,
    run_fn: Any = None,
    docker: bool = False,
) -> None:
    """Cross-compile python.elf for Nanvix."""
    if config.IS_WINDOWS:
        # Build and install in one Docker invocation so the install tree
        # is cached for later use by ``./z test`` (no Docker during tests).
        install_cache = repo_root / ".nanvix" / "_install_cache"
        docker_mod.docker_build(
            repo_root,
            Path(sysroot),
            platform=platform,
            process_mode=process_mode,
            memory_size=memory_size,
            install_prefix=install_prefix,
            release=release,
            install_destdir=install_cache,
        )
        return
    effective_sysroot = config.DOCKER_SYSROOT_PATH if docker else sysroot
    effective_toolchain = config.DOCKER_TOOLCHAIN_PATH if docker else toolchain
    args = make_args(
        effective_sysroot,
        effective_toolchain,
        "build",
        platform=platform,
        process_mode=process_mode,
        memory_size=memory_size,
        install_prefix=install_prefix,
        release=release,
    )
    run_make(args, cwd=repo_root, run_fn=run_fn)


def install(
    sysroot: str | Path,
    toolchain: str | Path,
    repo_root: Path,
    destdir: Path,
    *,
    platform: str = config.DEFAULT_PLATFORM,
    process_mode: str = config.DEFAULT_PROCESS_MODE,
    memory_size: str = config.DEFAULT_MEMORY_SIZE,
    install_prefix: str = config.DEFAULT_INSTALL_PREFIX,
    release: bool = False,
    run_fn: Any = None,
    extra_make_flags: list[str] | None = None,
    docker: bool = False,
) -> None:
    """Install CPython into a staging directory."""
    if config.IS_WINDOWS:
        docker_mod.docker_install(
            repo_root,
            Path(sysroot),
            destdir,
            platform=platform,
            process_mode=process_mode,
            memory_size=memory_size,
            install_prefix=install_prefix,
            release=release,
        )
        return
    effective_sysroot = config.DOCKER_SYSROOT_PATH if docker else sysroot
    effective_toolchain = config.DOCKER_TOOLCHAIN_PATH if docker else toolchain
    # When running inside Docker, repo_root maps to /mnt/workspace.
    # Use a relative DESTDIR so it resolves correctly inside the container.
    try:
        rel_destdir = destdir.resolve().relative_to(repo_root.resolve())
    except ValueError:
        rel_destdir = destdir
    args = make_args(
        effective_sysroot,
        effective_toolchain,
        *(extra_make_flags or []),
        "install",
        f"DESTDIR={rel_destdir}",
        platform=platform,
        process_mode=process_mode,
        memory_size=memory_size,
        install_prefix=install_prefix,
        release=release,
    )
    run_make(args, cwd=repo_root, run_fn=run_fn)


def clean(repo_root: Path) -> None:
    """Remove build artifacts."""
    if config.IS_WINDOWS:
        for name in (".nanvix-configured", "python.elf", "python.exe"):
            p = repo_root / name
            if p.is_file():
                p.unlink()
                print(f"Removed {name}")
        for name in ("_test_staging", "staging", "_install_cache", "_ramfs_cache"):
            p = repo_root / ".nanvix" / name
            if p.is_dir():
                shutil.rmtree(p)
                print(f"Removed .nanvix/{name}/")
    else:
        subprocess.run(
            ["make", "-f", "Makefile.nanvix", "clean"],
            cwd=repo_root,
            check=False,
        )


# Git-clean exclusion list — files preserved during distclean.
_DISTCLEAN_EXCLUDES: list[str] = [
    "Makefile.nanvix",
    "NANVIX.md",
    ".github",
    ".nanvix/z.py",
    ".nanvix/_loader.py",
    ".nanvix/nanvix.toml",
    ".nanvix/.gitignore",
    ".nanvix/config.py",
    ".nanvix/build.py",
    ".nanvix/docker.py",
    ".nanvix/ramfs.py",
    ".nanvix/test.py",
    ".nanvix/package.py",
    ".nanvix/run-regrtest.py",
    ".nanvix/run-tests.py",
    "z",
    "z.sh",
    "z.ps1",
]


def distclean(repo_root: Path) -> None:
    """Deep clean: remove all build artifacts, caches, and untracked files."""
    clean(repo_root)
    if config.IS_WINDOWS:
        docker_mod.clean_volume(repo_root)
    cmd = ["git", "clean", "-fdx"]
    for exc in _DISTCLEAN_EXCLUDES:
        cmd.extend(["-e", exc])
    subprocess.run(cmd, cwd=repo_root, check=True)
