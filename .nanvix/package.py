# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

"""Release packaging and verification for Nanvix CPython.

Replaces package-common.mk and verify-package.mk. Handles sysroot/buildroot
tarball creation, ramfs image inclusion, and tarball verification.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

import sys as _sys
_sys.path.insert(0, str(Path(__file__).resolve().parent))
from _loader import load_sibling

config = load_sibling("config", __file__)
build_mod = load_sibling("build", __file__)
ramfs_mod = load_sibling("ramfs", __file__)


def _artifact_base(
    platform: str = config.DEFAULT_PLATFORM,
    process_mode: str = config.DEFAULT_PROCESS_MODE,
    memory_size: str = config.DEFAULT_MEMORY_SIZE,
) -> str:
    """Return the base name for release tarballs."""
    return f"cpython-{platform}-{process_mode}-{memory_size}"


def package(
    sysroot: str | Path,
    toolchain: str | Path,
    repo_root: Path,
    *,
    platform: str = config.DEFAULT_PLATFORM,
    process_mode: str = config.DEFAULT_PROCESS_MODE,
    memory_size: str = config.DEFAULT_MEMORY_SIZE,
    install_prefix: str = config.DEFAULT_INSTALL_PREFIX,
    release: bool = True,
    run_fn=None,
    nanvix_home: str | Path | None = None,
) -> None:
    """Package CPython release tarballs.

    Creates two tarballs in ``dist/``:
    - ``cpython-<platform>-<mode>-<memory>.tar.bz2`` — runtime sysroot + binary + ramfs
    - ``cpython-<platform>-<mode>-<memory>-buildroot.tar.bz2`` — build dependencies

    Args:
        nanvix_home: Host-side path to the Nanvix sysroot for local
            file operations (mkramfs, etc.).  When Docker is active
            *sysroot* is a container-internal path; *nanvix_home*
            should be the original host path so that
            ``subprocess.run`` and ``Path.is_file`` work correctly.
    """
    nanvix_home = Path(nanvix_home) if nanvix_home else Path(sysroot)
    release_staging = repo_root / ".nanvix" / "release"
    dist_dir = repo_root / "dist"
    artifact = _artifact_base(platform, process_mode, memory_size)

    print("Packaging CPython release...")

    # Clean previous staging.
    if release_staging.exists():
        shutil.rmtree(release_staging)

    # Build.
    build_mod.build(
        sysroot, toolchain, repo_root,
        platform=platform,
        process_mode=process_mode,
        memory_size=memory_size,
        install_prefix=install_prefix,
        release=True,
        run_fn=run_fn,
    )

    # Install into staging.
    build_mod.install(
        sysroot, toolchain, repo_root, release_staging,
        platform=platform,
        process_mode=process_mode,
        memory_size=memory_size,
        install_prefix=install_prefix,
        release=True,
        run_fn=run_fn,
    )

    sysroot_installed = release_staging / "sysroot"
    if not sysroot_installed.is_dir():
        raise FileNotFoundError(f"Install did not produce {sysroot_installed}")

    # --- Buildroot: build dependencies ---
    buildroot_pkg = release_staging / "buildroot-pkg"
    buildroot_pkg.mkdir(parents=True)
    (buildroot_pkg / "lib").mkdir()
    (buildroot_pkg / "bin").mkdir()

    # Copy include directory.
    inc_src = sysroot_installed / "include"
    if inc_src.is_dir():
        shutil.copytree(inc_src, buildroot_pkg / "include")

    # Copy static libraries.
    lib_src = sysroot_installed / "lib"
    if lib_src.is_dir():
        for lib_file in lib_src.glob("*.a"):
            shutil.copy2(lib_file, buildroot_pkg / "lib" / lib_file.name)
        # Copy pkgconfig.
        pkgconfig = lib_src / "pkgconfig"
        if pkgconfig.is_dir():
            shutil.copytree(pkgconfig, buildroot_pkg / "lib" / "pkgconfig")
        # Copy config-3.12.
        config_dir = lib_src / config.PYTHON_LIB_DIR / f"config-{config.PYTHON_VERSION}"
        if config_dir.is_dir():
            dest = buildroot_pkg / "lib" / config.PYTHON_LIB_DIR / f"config-{config.PYTHON_VERSION}"
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copytree(config_dir, dest)

    # Copy dev binaries.
    for f in ["python3-config", f"python{config.PYTHON_VERSION}-config",
              "2to3", f"2to3-{config.PYTHON_VERSION}",
              "idle3", f"idle{config.PYTHON_VERSION}",
              "pydoc3", f"pydoc{config.PYTHON_VERSION}"]:
        src = sysroot_installed / "bin" / f
        if src.is_file():
            shutil.copy2(src, buildroot_pkg / "bin" / f)

    # Copy share directory.
    share_src = sysroot_installed / "share"
    if share_src.is_dir():
        shutil.copytree(share_src, buildroot_pkg / "share")

    # --- Sysroot: runtime stdlib (trimmed) ---
    ramfs_staging = release_staging / "sysroot-pkg-wrap"
    ramfs_sysroot = ramfs_staging / "sysroot" / "lib"
    ramfs_sysroot.mkdir(parents=True)

    py_lib = sysroot_installed / "lib" / config.PYTHON_LIB_DIR
    if py_lib.is_dir():
        shutil.copytree(py_lib, ramfs_sysroot / config.PYTHON_LIB_DIR)

    ramfs_mod.trim_sysroot(ramfs_staging)

    # --- Include python.elf binary ---
    bin_dir = release_staging / "bin"
    python_elf = repo_root / f"python{config.EXE}"
    if python_elf.is_file():
        bin_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(python_elf, bin_dir / "python.elf")
        size = (bin_dir / "python.elf").stat().st_size
        print(f"Included bin/python.elf ({size // 1024}K)")
    else:
        print("Warning: python.elf not found — binary will not be included in release")

    # --- Build ramfs image ---
    ramfs_img = release_staging / "cpython-ramfs.img"
    ramfs_mod.build_image(ramfs_staging, nanvix_home, ramfs_img)

    # --- Create release tarballs ---
    dist_dir.mkdir(parents=True, exist_ok=True)

    # Sysroot tarball.
    sysroot_tar = dist_dir / f"{artifact}.tar.bz2"
    sysroot_runtime = ramfs_staging / "sysroot"
    with tarfile.open(str(sysroot_tar), "w:bz2") as tf:
        tf.add(str(sysroot_runtime), arcname="sysroot")
        if bin_dir.is_dir():
            tf.add(str(bin_dir), arcname="bin")
        if ramfs_img.is_file():
            tf.add(str(ramfs_img), arcname="cpython-ramfs.img")

    # Buildroot tarball.
    buildroot_tar = dist_dir / f"{artifact}-buildroot.tar.bz2"
    with tarfile.open(str(buildroot_tar), "w:bz2") as tf:
        tf.add(str(buildroot_pkg), arcname="sysroot")

    # Cleanup staging.
    shutil.rmtree(release_staging)

    print("Release tarballs created in dist/")
    for f in sorted(dist_dir.glob(f"{artifact}*.tar.bz2")):
        size = f.stat().st_size
        print(f"  {f.name} ({size // 1024}K)")


def verify(
    repo_root: Path,
    *,
    platform: str = config.DEFAULT_PLATFORM,
    process_mode: str = config.DEFAULT_PROCESS_MODE,
    memory_size: str = config.DEFAULT_MEMORY_SIZE,
) -> None:
    """Verify release tarballs.

    Checks that tarballs exist, are not corrupt, and contain the
    expected contents.
    """
    artifact = _artifact_base(platform, process_mode, memory_size)
    dist_dir = repo_root / "dist"

    print("Verifying release tarballs...")

    sysroot_tar = dist_dir / f"{artifact}.tar.bz2"
    buildroot_tar = dist_dir / f"{artifact}-buildroot.tar.bz2"

    if not sysroot_tar.is_file():
        raise FileNotFoundError(f"Sysroot tarball not found: {sysroot_tar}")
    if not buildroot_tar.is_file():
        raise FileNotFoundError(f"Buildroot tarball not found: {buildroot_tar}")

    # Verify integrity.
    with tarfile.open(str(sysroot_tar), "r:bz2") as tf:
        members = tf.getnames()
    with tarfile.open(str(buildroot_tar), "r:bz2") as tf:
        _ = tf.getnames()

    # Verify python.elf is present (exact path match).
    if "bin/python.elf" not in members:
        raise ValueError("Sysroot tarball missing bin/python.elf")

    print("\t\t*** Package verification PASSED ***")
