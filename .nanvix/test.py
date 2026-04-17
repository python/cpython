# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

"""Test orchestration for Nanvix CPython.

Replaces test-common.mk, test-standalone.mk, test-multi-process.mk,
test-single-process.mk, test-hyperlight.mk, test-microvm.mk, and
test-run-host.py.

Provides staging, hello-world validation, and regrtest dispatch for
all deployment modes and platforms.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

import sys as _sys
_sys.path.insert(0, str(Path(__file__).resolve().parent))
from _loader import load_sibling

config = load_sibling("config", __file__)
build_mod = load_sibling("build", __file__)
ramfs_mod = load_sibling("ramfs", __file__)


# ---------------------------------------------------------------------------
# Staging
# ---------------------------------------------------------------------------

def stage(
    sysroot: str | Path,
    toolchain: str | Path,
    repo_root: Path,
    *,
    platform: str = config.DEFAULT_PLATFORM,
    process_mode: str = config.DEFAULT_PROCESS_MODE,
    memory_size: str = config.DEFAULT_MEMORY_SIZE,
    install_prefix: str = config.DEFAULT_INSTALL_PREFIX,
    release: bool = False,
    run_fn=None,
) -> Path:
    """Build, install, and stage CPython for testing.

    Returns the test staging directory.
    """
    staging = repo_root / ".nanvix" / "_test_staging"

    print("Running CPython tests on Nanvix...")
    if staging.exists():
        shutil.rmtree(staging)

    if config.IS_WINDOWS:
        # On Windows, use the cached install tree produced by ``./z build``
        # so that no Docker invocation is needed during testing.
        install_cache = repo_root / ".nanvix" / "_install_cache"
        if not install_cache.is_dir():
            raise FileNotFoundError(
                "Install cache not found at .nanvix/_install_cache/. "
                "Run `./z build` before `./z test` to create the install cache."
            )
        shutil.copytree(install_cache, staging)
        print("  Using cached install from ./z build")
    else:
        # Linux: build and install directly.
        build_mod.build(
            sysroot, toolchain, repo_root,
            platform=platform,
            process_mode=process_mode,
            memory_size=memory_size,
            install_prefix=install_prefix,
            release=release,
            run_fn=run_fn,
        )

        build_mod.install(
            sysroot, toolchain, repo_root, staging,
            platform=platform,
            process_mode=process_mode,
            memory_size=memory_size,
            install_prefix=install_prefix,
            release=release,
            run_fn=run_fn,
        )

    sysroot_dir = staging / "sysroot"

    # Ensure _sysconfigdata module is present in the installed sysroot.
    # make install should copy it from build/<pybuilddir>/ but this can
    # silently fail when PYTHON_FOR_BUILD is not available or when the
    # install recipe is interrupted.  Fall back to copying from the build
    # directory directly.
    scdata_name = f"{config.SYSCONFIGDATA_NAME}.py"
    scdata_dst = sysroot_dir / "lib" / config.PYTHON_LIB_DIR / scdata_name
    if not scdata_dst.is_file():
        pybuilddir = repo_root / "pybuilddir.txt"
        if pybuilddir.is_file():
            bdir = repo_root / pybuilddir.read_text().strip()
            scdata_src = bdir / scdata_name
            if scdata_src.is_file():
                shutil.copy2(scdata_src, scdata_dst)
                print(f"  Copied {scdata_name} from build dir (make install missed it)")
            else:
                print(f"  WARNING: {scdata_name} not found in build dir {bdir}")
        else:
            print(f"  WARNING: pybuilddir.txt not found; cannot locate {scdata_name}")
    else:
        print(f"  Verified: {scdata_name} installed ({scdata_dst.stat().st_size} bytes)")

    # Copy test script — a simple smoke test that validates the interpreter.
    hello_script = sysroot_dir / "test_hello.py"
    hello_script.write_text(
        "import sys\n"
        "print('CPYTHON_TEST_HELLO: Hello from Python', sys.version_info[:2])\n"
        "print('CPYTHON_TEST_PLATFORM:', sys.platform)\n"
    )

    # Copy Nanvix runtime binaries.
    bin_dir = sysroot_dir / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)
    nanvix_home = Path(sysroot)
    for binary in ["nanvixd.elf", "kernel.elf", "linuxd.elf", "uservm.elf",
                    "nanvixd.exe", "kernel.exe"]:
        src = nanvix_home / "bin" / binary
        if src.is_file():
            shutil.copy2(src, bin_dir / binary)

    # Replace unstripped python binary with stripped python.elf.
    stripped = repo_root / f"python{config.EXE}"
    if stripped.is_file():
        target = bin_dir / config.python_binary()
        shutil.copy2(stripped, target)
        size = target.stat().st_size
        print(f"  Installed stripped python.elf into staging ({size // 1024}K)")

    # Copy guest-side test runner.
    regrtest_runner = repo_root / ".nanvix" / "run-regrtest.py"
    if regrtest_runner.is_file():
        shutil.copy2(regrtest_runner, sysroot_dir / "run-regrtest.py")

    # Invalidate stale ramfs image and cache from previous runs.
    stale_ramfs = repo_root / ".nanvix" / "cpython-rootfs.img"
    if stale_ramfs.is_file():
        stale_ramfs.unlink()
    stale_cache = repo_root / ".nanvix" / "_ramfs_cache"
    if stale_cache.is_dir():
        shutil.rmtree(stale_cache)

    return staging


# ---------------------------------------------------------------------------
# Ramfs staging (standalone mode)
# ---------------------------------------------------------------------------

def stage_ramfs(
    staging: Path,
    nanvix_home: Path,
    repo_root: Path,
    ramfs_img: Path | None = None,
) -> Path:
    """Build a ramfs image for standalone mode testing.

    Uses a cached ramfs directory under .nanvix/_ramfs_cache/.

    Returns the path to the ramfs image.
    """
    if ramfs_img is None:
        ramfs_img = repo_root / ".nanvix" / "cpython-rootfs.img"

    ramfs_cache = repo_root / ".nanvix" / "_ramfs_cache"

    if ramfs_img.is_file() and (ramfs_cache / "sysroot").is_dir():
        print(f"  Using cached ramfs: {ramfs_img}")
        return ramfs_img

    # Build fresh ramfs.
    if ramfs_cache.exists():
        shutil.rmtree(ramfs_cache)
    ramfs_cache.mkdir(parents=True)

    # Copy sysroot from test staging.
    sysroot_src = staging / "sysroot"
    sysroot_dst = ramfs_cache / "sysroot"
    shutil.copytree(sysroot_src, sysroot_dst)

    # Create /tmp for tempfile.gettempdir().
    (sysroot_dst / "tmp").mkdir(exist_ok=True)

    # Trim and build ramfs image (keep tests for test pipeline).
    ramfs_mod.trim_and_build(
        ramfs_cache, nanvix_home, ramfs_img,
        keep_tests=True,
    )

    return ramfs_img


# ---------------------------------------------------------------------------
# Hello-world test
# ---------------------------------------------------------------------------

def run_hello(
    staging: Path,
    *,
    process_mode: str = config.DEFAULT_PROCESS_MODE,
    platform: str = config.DEFAULT_PLATFORM,
    nanvixd_extra: list[str] | None = None,
    ramfs_img: Path | None = None,
    nanvix_home: Path | None = None,
) -> None:
    """Run the hello-world test via nanvixd.

    Standalone mode uses ramfs + ``-bin-dir`` + the semicolon-delimited
    environment variable syntax.  Multi-process and single-process modes
    use direct host-filesystem access (no ramfs).
    """
    sysroot = staging / "sysroot"
    nanvixd_extra = nanvixd_extra or config.PLATFORM_NANVIXD_ARGS.get(platform, [])
    # On Windows, CreateProcess searches for the executable relative to the
    # *parent's* CWD, not the child's cwd. Use an absolute path to avoid this.
    nanvixd = str((sysroot / "bin" / config.nanvixd_binary()).resolve())
    python_bin = f"./bin/{config.python_binary()}"
    standalone = process_mode == "standalone"

    if standalone:
        if ramfs_img is None:
            raise ValueError("ramfs_img is required for standalone mode")

        # Copy mkramfs into staging bin (needed for ramfs generation).
        if nanvix_home:
            mkramfs_name = config.mkramfs_binary()
            mkramfs_src = nanvix_home / "bin" / mkramfs_name
            if mkramfs_src.is_file():
                shutil.copy2(mkramfs_src, sysroot / "bin" / mkramfs_name)

    print(f"Test: Hello world ({process_mode})...")

    if standalone:
        # Standalone: semicolon-delimited env vars + ramfs.
        cmd = [
            nanvixd,
            "-bin-dir", "./bin", "-ramfs", str(ramfs_img),
            *nanvixd_extra,
            "--", python_bin,
            f"-B ./test_hello.py;PYTHONHOME=/ PYTHONDONTWRITEBYTECODE=1"
            f" _PYTHON_SYSCONFIGDATA_NAME={config.SYSCONFIGDATA_NAME}",
        ]
    else:
        # Direct mode: guest accesses host filesystem, no ramfs.
        cmd = [
            nanvixd,
            *nanvixd_extra,
            "--", python_bin,
            "./test_hello.py",
        ]

    start = time.monotonic()
    try:
        result = subprocess.run(
            cmd,
            stdin=subprocess.DEVNULL,
            capture_output=True,
            text=True,
            timeout=120,
            cwd=sysroot,
        )
    except subprocess.TimeoutExpired:
        raise RuntimeError("Hello test timed out after 120s")

    elapsed_ms = int((time.monotonic() - start) * 1000)
    output = (result.stdout + "\n" + result.stderr).strip()
    print(f"  Execution time: {elapsed_ms} ms")

    if result.returncode != 0:
        print(f"  FAIL: Hello test exited with status {result.returncode}")
        print(output)
        raise RuntimeError(
            f"Hello test exited with status {result.returncode}"
        )

    # Validate output.
    found_hello = False
    for line in output.splitlines():
        if line.startswith("CPYTHON_TEST_"):
            tag = line.split(":")[0].replace("CPYTHON_TEST_", "")
            print(f"  {tag}: {line.strip()}")
            if tag == "HELLO":
                found_hello = True

    if not found_hello:
        print("  FAIL: Hello test did not produce expected output")
        print(output)
        raise RuntimeError("Hello test did not produce expected output")

    print("  PASS")


# ---------------------------------------------------------------------------
# Regression tests
# ---------------------------------------------------------------------------

def run_regrtest(
    staging: Path,
    repo_root: Path,
    *,
    process_mode: str = config.DEFAULT_PROCESS_MODE,
    platform: str = config.DEFAULT_PLATFORM,
    test_list: list[str] | None = None,
    batch_size: int = config.DEFAULT_TEST_BATCH_SIZE,
    nanvixd_extra: list[str] | None = None,
    ramfs_img: Path | None = None,
    release: bool = False,
) -> None:
    """Run stdlib regression tests via run-tests.py."""
    if release:
        print("Test: regrtest skipped (NANVIX_RELEASE=yes)")
        return

    if test_list is None:
        test_list = config.NANVIX_TEST_LIST

    nanvixd_extra = nanvixd_extra or config.PLATFORM_NANVIXD_ARGS.get(platform, [])
    standalone = process_mode == "standalone"

    print(f"Test: regrtest ({len(test_list)} modules, {process_mode})...")

    sysroot = staging / "sysroot"
    run_tests_script = repo_root / ".nanvix" / "run-tests.py"

    env = os.environ.copy()
    env["NANVIX_TEST_BATCH_SIZE"] = str(batch_size)
    env["NANVIX_PYTHON_BIN"] = f"./bin/{config.python_binary()}"

    if standalone:
        # Standalone: ramfs + semicolon env syntax.
        if ramfs_img is None:
            ramfs_img = repo_root / ".nanvix" / "cpython-rootfs.img"
        extra_str = f"-bin-dir ./bin -ramfs {ramfs_img}"
        if nanvixd_extra:
            extra_str += " " + " ".join(nanvixd_extra)
        env["NANVIXD_EXTRA_ARGS"] = extra_str
        env["NANVIX_STANDALONE"] = "1"
        env["NANVIX_EXCLUDE_TESTS"] = " ".join(config.STANDALONE_EXCLUDE)
    else:
        # Direct mode: host filesystem, no ramfs.
        if nanvixd_extra:
            env["NANVIXD_EXTRA_ARGS"] = " ".join(nanvixd_extra)
        else:
            env.pop("NANVIXD_EXTRA_ARGS", None)
        env.pop("NANVIX_STANDALONE", None)

    cmd = [sys.executable, str(run_tests_script)] + test_list

    result = subprocess.run(cmd, cwd=sysroot, env=env)
    if result.returncode != 0:
        raise RuntimeError(
            f"regrtest failed with exit code {result.returncode}"
        )


# ---------------------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------------------

def cleanup(repo_root: Path) -> None:
    """Clean up test artifacts."""
    staging = repo_root / ".nanvix" / "_test_staging"
    if staging.is_dir():
        shutil.rmtree(staging)
    for name in ["cpython_test.log", "cpython_regrtest.log", "cpython_regrtest_batch.log"]:
        p = repo_root / ".nanvix" / name
        if p.is_file():
            p.unlink()


def deep_cleanup(repo_root: Path) -> None:
    """Deep clean: remove cached ramfs template."""
    cleanup(repo_root)
    ramfs_cache = repo_root / ".nanvix" / "_ramfs_cache"
    if ramfs_cache.is_dir():
        shutil.rmtree(ramfs_cache)
    ramfs_img = repo_root / ".nanvix" / "cpython-rootfs.img"
    if ramfs_img.is_file():
        ramfs_img.unlink()


# ---------------------------------------------------------------------------
# Aggregate test runner
# ---------------------------------------------------------------------------

def run_all(
    sysroot: str | Path,
    toolchain: str | Path,
    repo_root: Path,
    *,
    platform: str = config.DEFAULT_PLATFORM,
    process_mode: str = config.DEFAULT_PROCESS_MODE,
    memory_size: str = config.DEFAULT_MEMORY_SIZE,
    install_prefix: str = config.DEFAULT_INSTALL_PREFIX,
    release: bool = False,
    test_list: list[str] | None = None,
    batch_size: int = config.DEFAULT_TEST_BATCH_SIZE,
    run_fn=None,
) -> None:
    """Run the complete test pipeline: stage → hello → regrtest → cleanup."""
    nanvix_home = Path(sysroot)
    standalone = process_mode == "standalone"

    # Stage.
    staging = stage(
        sysroot, toolchain, repo_root,
        platform=platform,
        process_mode=process_mode,
        memory_size=memory_size,
        install_prefix=install_prefix,
        release=release,
        run_fn=run_fn,
    )

    # Ramfs — only needed for standalone mode.  Multi-process and
    # single-process use host-filesystem access (no ramfs).
    ramfs_img = None
    if standalone:
        ramfs_img = stage_ramfs(staging, nanvix_home, repo_root)

    # Hello test.
    run_hello(
        staging,
        process_mode=process_mode,
        platform=platform,
        ramfs_img=ramfs_img,
        nanvix_home=nanvix_home,
    )

    # Regression tests.
    run_regrtest(
        staging, repo_root,
        process_mode=process_mode,
        platform=platform,
        test_list=test_list,
        batch_size=batch_size,
        ramfs_img=ramfs_img,
        release=release,
    )

    # Cleanup.
    cleanup(repo_root)

    print("\t\t*** CPython tests PASSED ***")
