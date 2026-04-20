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

import json
import os
import shutil
import subprocess
import sys
import tarfile
import time
import urllib.request
from pathlib import Path

import sys as _sys
_sys.path.insert(0, str(Path(__file__).resolve().parent))
from _loader import load_sibling

config = load_sibling("config", __file__)
build_mod = load_sibling("build", __file__)
ramfs_mod = load_sibling("ramfs", __file__)


# ---------------------------------------------------------------------------
# Windows: download release artifacts as install cache
# ---------------------------------------------------------------------------

def _download_release_as_cache(
    repo_root: Path,
    platform: str,
    process_mode: str,
    memory_size: str,
) -> Path:
    """Download the latest cpython release tarball and extract it as _install_cache.

    This lets ``./z test`` work on Windows without a prior ``./z build``
    (which requires Docker). The release tarball contains the same
    sysroot tree that ``./z build`` would produce.
    """
    cache_dir = repo_root / ".nanvix" / "_install_cache"
    if cache_dir.exists():
        shutil.rmtree(cache_dir)
    cache_dir.mkdir(parents=True, exist_ok=True)

    # Resolve the latest release from nanvix/cpython.
    gh_token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
    api_url = "https://api.github.com/repos/nanvix/cpython/releases/latest"
    req = urllib.request.Request(api_url)
    req.add_header("Accept", "application/vnd.github+json")
    if gh_token:
        req.add_header("Authorization", f"Bearer {gh_token}")

    with urllib.request.urlopen(req, timeout=30) as resp:
        release = json.loads(resp.read())

    tag = release["tag_name"]
    print(f"  Resolved cpython release: {tag}")

    # Find a standalone tarball asset.
    asset_prefix = f"cpython-{platform}-{process_mode}-{memory_size}"
    asset_url = None
    asset_name = None
    for a in release.get("assets", []):
        name = a.get("name", "")
        if (name.startswith(asset_prefix)
                and name.endswith(".tar.bz2")
                and "buildroot" not in name):
            asset_url = a["browser_download_url"]
            asset_name = name
            break

    if not asset_url:
        raise FileNotFoundError(
            f"No cpython release asset matching '{asset_prefix}*.tar.bz2' "
            f"in release {tag}. Available assets: "
            + ", ".join(a["name"] for a in release.get("assets", []))
        )

    # Download.
    dl_dir = repo_root / ".nanvix" / "cache"
    dl_dir.mkdir(parents=True, exist_ok=True)
    tarball = dl_dir / asset_name
    if not tarball.is_file():
        print(f"  Downloading {asset_name}...")
        urllib.request.urlretrieve(asset_url, str(tarball))

    # Extract into _install_cache with path-traversal protection.
    print(f"  Extracting to {cache_dir}...")
    with tarfile.open(tarball, "r:bz2") as tf:
        base = cache_dir.resolve()
        for member in tf.getmembers():
            if member.issym() or member.islnk():
                raise tarfile.TarError(
                    f"refusing to extract link entry: {member.name}"
                )
            resolved = (base / member.name).resolve()
            if os.path.commonpath([str(base), str(resolved)]) != str(base):
                raise tarfile.TarError(
                    f"refusing to extract path outside destination: {member.name}"
                )
        tf.extractall(cache_dir)

    # The tarball contains {bin/, sysroot/, cpython-ramfs.img}.
    # Restructure if needed so that sysroot/ is at cache_dir/sysroot/.
    sysroot = cache_dir / "sysroot"
    if not sysroot.is_dir():
        python_lib_dir = Path(config.PYTHON_LIB_DIR)
        for candidate in cache_dir.rglob(str(python_lib_dir)):
            parent = candidate
            for _ in python_lib_dir.parts:
                parent = parent.parent
            if parent != cache_dir:
                sysroot.mkdir(exist_ok=True)
                for item in parent.iterdir():
                    shutil.move(str(item), str(sysroot / item.name))
                break

    # Copy the stripped python binary into sysroot/bin/ if present.
    bin_dir = sysroot / "bin"
    bin_dir.mkdir(exist_ok=True)
    python_elf = cache_dir / "bin" / "python.elf"
    if python_elf.is_file():
        shutil.copy2(python_elf, bin_dir / config.python_binary())
        print(f"  Installed python binary ({python_elf.stat().st_size // 1024}K)")

    # Copy the test suite from the source tree into the sysroot.
    # The release tarball is trimmed (no Lib/test/), but regrtest
    # needs it. The source checkout has the full Lib/test/.
    pylib_dir = sysroot / "lib" / config.PYTHON_LIB_DIR
    test_dst = pylib_dir / "test"
    test_src = repo_root / "Lib" / "test"
    if test_src.is_dir() and not test_dst.is_dir():
        shutil.copytree(test_src, test_dst)
        test_count = sum(1 for _ in test_dst.rglob("*.py"))
        print(f"  Copied test suite from source tree ({test_count} files)")

    print(f"  Install cache ready at {cache_dir}")
    return cache_dir


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
        if install_cache.is_dir():
            shutil.copytree(install_cache, staging)
            print("  Using cached install from ./z build")
        else:
            # Fallback: download the release tarball and use it as the
            # install cache. This lets ``./z test`` work on Windows
            # without a prior ``./z build`` (which requires Docker).
            print("  Install cache not found — downloading release artifacts...")
            install_cache = _download_release_as_cache(
                repo_root, platform, process_mode, memory_size,
            )
            shutil.copytree(install_cache, staging)
            print("  Using downloaded release as install cache")
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
        exclude_set = set(config.STANDALONE_EXCLUDE)
        test_list = [m for m in test_list if m not in exclude_set]
    else:
        # Direct mode: host filesystem, no ramfs.
        if nanvixd_extra:
            env["NANVIXD_EXTRA_ARGS"] = " ".join(nanvixd_extra)
        else:
            env.pop("NANVIXD_EXTRA_ARGS", None)
        env.pop("NANVIX_STANDALONE", None)

    cmd = [sys.executable, str(run_tests_script)] + test_list

    print(f"Test: regrtest ({len(test_list)} modules, {process_mode})...")
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
