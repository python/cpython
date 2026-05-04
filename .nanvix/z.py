# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

"""Nanvix build script for CPython.

Usage:
    ./z setup      # Download Nanvix sysroot and dependencies
    ./z build      # Cross-compile python.elf and libpython.a
    ./z test       # Run test suite (hello-world on nanvixd.elf)
    ./z release    # Package release tarballs (sysroot + buildroot)
    ./z clean      # Remove build artifacts
    ./z distclean  # Deep clean (build artifacts + untracked files)

Options:
    --with-nanvix PATH  Use local Nanvix binaries from PATH instead of
                        the downloaded sysroot binaries. PATH should point
                        to a Nanvix build directory containing bin/ and lib/.
                        The path is persisted in .nanvix/env.json, so it
                        only needs to be passed once. Pass again to change
                        it. Works on both Linux and Windows.
"""

import json
import os
import shutil
import sys
import tarfile
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

from nanvix_zutil import (
    CFG_SYSROOT,
    CFG_TOOLCHAIN,
    EXIT_MISSING_DEP,
    ZScript,
    log,
    suffix_dep,
)

# ---------------------------------------------------------------------------
# Local modules (loaded via importlib since .nanvix/ is not a valid package name)
# ---------------------------------------------------------------------------

import sys as _sys

_sys.path.insert(0, str(Path(__file__).resolve().parent))
from _loader import load_sibling

build_mod = load_sibling("build", __file__)
config = load_sibling("config", __file__)
docker_mod = load_sibling("docker", __file__)
package_mod = load_sibling("package", __file__)
test_mod = load_sibling("test", __file__)

# ---------------------------------------------------------------------------
# Path helpers
# ---------------------------------------------------------------------------

# Makefile variable names (build-system-specific).
_MAKE_VAR_CONFIG = "CONFIG_NANVIX"
_MAKE_VAR_HOME = "NANVIX_HOME"
_MAKE_VAR_TOOLCHAIN = "NANVIX_TOOLCHAIN"
_MAKE_VAR_PLATFORM = "PLATFORM"
_MAKE_VAR_PROCESS_MODE = "PROCESS_MODE"
_MAKE_VAR_MEMORY_SIZE = "MEMORY_SIZE"
_MAKE_VAR_INSTALL_PREFIX = "INSTALL_PREFIX"
_MAKE_VAR_RELEASE = "NANVIX_RELEASE"

# CPython embeds --prefix into the binary (sys.prefix, sys.path).
_DEFAULT_INSTALL_PREFIX = config.DEFAULT_INSTALL_PREFIX

# Config key for persisting the --with-nanvix path in env.json.
_CFG_LOCAL_NANVIX = "local_nanvix_path"

# ---------------------------------------------------------------------------
# Early --with-nanvix extraction
# ---------------------------------------------------------------------------
# The nanvix-zutil CLI inspects sys.argv to find the subcommand *before*
# calling CPythonBuild.main().  The shell wrappers (z.sh / z.ps1) strip
# --with-nanvix PATH from argv and pass it via the NANVIX_LOCAL_PATH
# environment variable.  Pick it up here at import time.

_EARLY_LOCAL_NANVIX: str | None = os.environ.get("NANVIX_LOCAL_PATH") or None


# Map dependency names to the library files they install into buildroot/lib.
_DEP_EXPECTED_LIBS: dict[str, list[str]] = {
    "bzip2": ["libbz2.a"],
    "libffi": ["libffi.a"],
    "zlib": ["libz.a"],
    "sqlite": ["libsqlite3.a"],
    "openssl": ["libssl.a", "libcrypto.a"],
    "libxml2": ["libxml2.a"],
    "libxslt": ["libxslt.a", "libexslt.a"],
    "lxml": ["liblxml_etree.a", "liblxml_elementpath.a"],
}


class CPythonBuild(ZScript):
    """Build script for nanvix/cpython."""

    _local_nanvix_path: str | None = None

    if sys.platform == "win32":
        SYSROOT_REQUIRED_FILES: tuple[str, ...] = (
            "lib/libposix.a",
            "lib/user.ld",
            "bin/nanvixd.exe",
            "bin/kernel.elf",
            "bin/mkramfs.exe",
        )

        SYSROOT_MULTI_PROCESS_FILES: tuple[str, ...] = ()

    # ---- CLI entry point -------------------------------------------------

    @classmethod
    def main(cls, *, repo_root: Path | None = None) -> None:
        """Pre-parse ``--with-nanvix`` and delegate to ZScript.main()."""
        if _EARLY_LOCAL_NANVIX is not None:
            cls._local_nanvix_path = _EARLY_LOCAL_NANVIX
        super().main(repo_root=repo_root)

    # ---- Local Nanvix overlay --------------------------------------------

    def _overlay_local_nanvix(self) -> None:
        """Copy local Nanvix binaries and libraries into the sysroot.

        When ``--with-nanvix PATH`` is supplied (or was previously
        persisted in config), this method copies the runtime binaries
        (nanvixd, kernel, mkramfs, …) and libraries (libposix.a, user.ld)
        from the local Nanvix build directory into the configured sysroot
        so that subsequent build and test steps use the local versions.

        The path is persisted in ``.nanvix/env.json`` on first use so
        that later commands pick it up automatically.

        Works on both Linux (ELF binaries) and Windows (.exe binaries).
        """
        # CLI flag takes precedence; fall back to persisted config.
        nanvix_path = self._local_nanvix_path or self.config.get(_CFG_LOCAL_NANVIX, "")
        if not nanvix_path:
            return

        nanvix_path = os.path.abspath(os.path.expanduser(nanvix_path))
        if not os.path.isdir(nanvix_path):
            log.warning(f"--with-nanvix path no longer exists: {nanvix_path}")
            return

        # Persist so subsequent commands reuse the same path.
        if self.config.get(_CFG_LOCAL_NANVIX, "") != nanvix_path:
            self.config.set(_CFG_LOCAL_NANVIX, nanvix_path)
            self.config.save()

        sysroot = self.config.get(CFG_SYSROOT, "")
        if not sysroot:
            return

        nanvix_dir = Path(nanvix_path)
        sysroot_path = Path(sysroot)

        log.info(f"Overlaying local Nanvix binaries from {nanvix_dir}")

        # -- Binaries ------------------------------------------------------
        bin_src = nanvix_dir / "bin"
        bin_dst = sysroot_path / "bin"
        bin_dst.mkdir(parents=True, exist_ok=True)

        if sys.platform == "win32":
            binaries = ["nanvixd.exe", "mkramfs.exe", "kernel.elf"]
        else:
            binaries = [
                "nanvixd.elf",
                "kernel.elf",
                "mkramfs.elf",
                "linuxd.elf",
                "uservm.elf",
            ]

        for name in binaries:
            src = bin_src / name
            if src.is_file():
                shutil.copy2(src, bin_dst / name)
                log.info(f"  Copied {name}")

        # -- Libraries -----------------------------------------------------
        lib_dst = sysroot_path / "lib"
        lib_dst.mkdir(parents=True, exist_ok=True)
        lib_src = nanvix_dir / "lib"

        if lib_src.is_dir():
            for lib_name in ["libposix.a"]:
                src = lib_src / lib_name
                if src.is_file():
                    shutil.copy2(src, lib_dst / lib_name)
                    log.info(f"  Copied {lib_name}")

        # -- Linker script (user.ld) — check multiple locations ------------
        user_ld_candidates = [
            nanvix_dir / "lib" / "user.ld",
            nanvix_dir / "sysroot-release" / "lib" / "user.ld",
            nanvix_dir / "build" / "user" / "linker" / "x86" / "user.ld",
        ]
        for candidate in user_ld_candidates:
            if candidate.is_file():
                shutil.copy2(candidate, lib_dst / "user.ld")
                log.info(f"  Copied user.ld from {candidate}")
                break

    # ---- Common helpers --------------------------------------------------

    def _get_host_paths(self) -> tuple[str, str]:
        """Return (sysroot, toolchain) raw host paths from config."""
        sysroot = self.config.get(CFG_SYSROOT, "")
        if not sysroot:
            log.fatal(
                f"{CFG_SYSROOT} is not set.",
                code=EXIT_MISSING_DEP,
                hint="Run `./z setup` first to download the sysroot.",
            )
        toolchain = self.config.get(CFG_TOOLCHAIN, config.TOOLCHAIN_DEFAULT_PATH)
        return sysroot, toolchain or config.TOOLCHAIN_DEFAULT_PATH

    def _build_kwargs(self, release: bool = False) -> dict[str, object]:
        """Return common keyword arguments for build/test/package modules."""
        return {
            "platform": self.config.machine,
            "process_mode": self.config.deployment_mode,
            "memory_size": self.config.memory_size,
            "install_prefix": _DEFAULT_INSTALL_PREFIX,
            "release": release,
        }

    # ---- Make invocation (for build/install only) ------------------------

    def _run_make(self, make_args: list[str]) -> None:
        """Execute a make command directly (Linux/macOS only).

        On Windows, callers should use ``build_mod.build()`` or
        ``build_mod.install()`` which route through ``docker_mod``
        automatically.
        """
        if sys.platform == "win32":
            raise RuntimeError(
                "_run_make() is not supported on Windows. "
                "Use build_mod.build() / build_mod.install() instead."
            )
        self.run(*make_args, cwd=self.repo_root, docker=False)

    def _make_args(self, *targets: str) -> list[str]:
        """Build the make argument list for configure/build/install."""
        sysroot, toolchain = self._get_host_paths()
        release = os.environ.get(_MAKE_VAR_RELEASE, "no")

        return build_mod.make_args(
            str(self.translate_path(Path(sysroot))),
            str(self.translate_path(Path(toolchain))),
            *targets,
            platform=self.config.machine,
            process_mode=self.config.deployment_mode,
            memory_size=self.config.memory_size,
            install_prefix=_DEFAULT_INSTALL_PREFIX,
            release=(release == "yes"),
        )

    def setup(self) -> bool:
        """Download the Nanvix sysroot and dependencies.

        Delegates sysroot download, Windows binary augmentation, and
        verification to the base class. The local-nanvix override is
        handled before calling super().
        """
        local_nanvix = self._local_nanvix_path
        if local_nanvix:
            local_nanvix = os.path.abspath(os.path.expanduser(local_nanvix))

        used_fallback = False
        if local_nanvix and os.path.isdir(local_nanvix):
            self._setup_from_local_nanvix(local_nanvix)
        else:
            # Base class handles: download sysroot, download Windows
            # binaries (if on Windows), verify required files.
            used_fallback = super().setup()

        self._install_missing_deps()

        self.config.save()

        buildroot = self.nanvix_dir / "buildroot"
        sysroot = self.config.get(CFG_SYSROOT, "")
        if not sysroot or not buildroot.is_dir():
            return used_fallback

        sysroot_path = Path(sysroot)
        for subdir in ("lib", "include"):
            src = buildroot / subdir
            dst = sysroot_path / subdir
            if not src.is_dir():
                continue
            dst.mkdir(parents=True, exist_ok=True)
            for item in src.iterdir():
                target = dst / item.name
                if item.is_dir():
                    shutil.copytree(item, target, dirs_exist_ok=True)
                    log.info(f"Merged directory {subdir}/{item.name} into sysroot")
                else:
                    shutil.copy2(item, target)
                    log.info(f"Merged {subdir}/{item.name} into sysroot")

        # Overlay local Nanvix binaries last so they take precedence.
        self._overlay_local_nanvix()
        return used_fallback

    def build(self) -> None:
        """Cross-compile python.elf and libpython.a for Nanvix."""
        self._overlay_local_nanvix()
        sysroot, toolchain = self._get_host_paths()
        release = os.environ.get(_MAKE_VAR_RELEASE, "no") == "yes"
        build_mod.build(
            sysroot,
            toolchain,
            self.repo_root,
            **self._build_kwargs(release=release),
            run_fn=lambda *args, **kw: self.run(*args, **kw),  # type: ignore[arg-type]
            docker=self.docker is not None,
        )

    def test(self) -> None:
        """Run the CPython test suite (hello + regrtest)."""
        self._overlay_local_nanvix()
        sysroot, toolchain = self._get_host_paths()
        kwargs = self._build_kwargs()

        test_mod.run_all(
            sysroot,
            toolchain,
            self.repo_root,
            **kwargs,
            run_fn=lambda *args, **kw: self.run(*args, **kw),  # type: ignore[arg-type]
            docker=self.docker is not None,
        )

    def release(self) -> None:
        """Package the CPython release tarballs and verify them."""
        self._overlay_local_nanvix()
        sysroot, toolchain = self._get_host_paths()
        kwargs = self._build_kwargs(release=True)

        package_mod.package(
            sysroot,
            toolchain,
            self.repo_root,
            **kwargs,
            run_fn=lambda *args, **kw: self.run(*args, **kw),  # type: ignore[arg-type]
            docker=self.docker is not None,
        )
        package_mod.verify(
            self.repo_root,
            platform=kwargs["platform"],
            process_mode=kwargs["process_mode"],
            memory_size=kwargs["memory_size"],
        )

    def clean(self) -> None:
        """Remove build artifacts."""
        build_mod.clean(self.repo_root)

    def distclean(self) -> None:
        """Deep clean: remove all build artifacts, caches, and untracked files."""
        build_mod.distclean(self.repo_root)

    # ---- Local Nanvix override -------------------------------------------

    def _setup_from_local_nanvix(self, local_nanvix: str) -> None:
        """Set up sysroot from a local Nanvix build directory."""
        from nanvix_zutil import Sysroot

        log.info(f"Using local Nanvix from {local_nanvix}")
        sysroot_dir = self.nanvix_dir / "sysroot"
        if sysroot_dir.exists():
            shutil.rmtree(sysroot_dir)
        sysroot_dir.mkdir(parents=True)

        local = Path(local_nanvix)
        bin_dst = sysroot_dir / "bin"
        bin_dst.mkdir()
        if config.IS_WINDOWS:
            binaries = ["nanvixd.exe", "mkramfs.exe", "kernel.elf"]
        else:
            binaries = [
                "nanvixd.elf",
                "kernel.elf",
                "mkramfs.elf",
                "linuxd.elf",
                "uservm.elf",
            ]
        for name in binaries:
            src = local / "bin" / name
            if src.is_file():
                shutil.copy2(src, bin_dst / name)
                log.info(f"  Copied bin/{name}")

        lib_dst = sysroot_dir / "lib"
        lib_dst.mkdir()
        lib_src = local / "lib"
        if lib_src.is_dir():
            for f in lib_src.iterdir():
                if f.is_file():
                    shutil.copy2(f, lib_dst / f.name)
                    log.info(f"  Copied lib/{f.name}")

        if not (lib_dst / "user.ld").is_file():
            for candidate in [
                local / "sysroot-release" / "lib" / "user.ld",
                local / "build" / "user" / "linker" / "x86" / "user.ld",
            ]:
                if candidate.is_file():
                    shutil.copy2(candidate, lib_dst / "user.ld")
                    log.info(f"  Copied user.ld from {candidate}")
                    break

        self.sysroot = Sysroot(sysroot_dir.resolve())
        self.sysroot.verify(self.sysroot_required_files())
        self.config.set(CFG_SYSROOT, str(self.sysroot.path))
        self.config.set(_CFG_LOCAL_NANVIX, local_nanvix)

    def _install_missing_deps(self) -> None:
        """Download missing dependency libraries using fallback assets."""
        buildroot = self.nanvix_dir / "buildroot"
        buildroot.mkdir(parents=True, exist_ok=True)
        lib_dir = buildroot / "lib"

        sysroot_tag = self.manifest.sysroot_ref.value
        nanvix_version = str(sysroot_tag).removeprefix("v") if sysroot_tag else ""

        for dep in self.manifest.dependencies:
            expected = _DEP_EXPECTED_LIBS.get(dep.name, [])
            if not expected:
                continue
            libs_present = all((lib_dir / lib).exists() for lib in expected)
            # For lxml, also require the python-packages payload.
            if dep.name == "lxml":
                pkg_present = (
                    buildroot / "python-packages" / "lxml" / "__init__.py"
                ).exists()
                if libs_present and pkg_present:
                    continue
            elif libs_present:
                continue
            resolved = suffix_dep(dep, nanvix_version) if nanvix_version else dep
            self._download_dep_fallback(
                resolved.name, resolved.repo, str(resolved.ref.value), buildroot
            )

    def _download_dep_fallback(
        self,
        dep_name: str,
        repo: str,
        ref: str,
        buildroot: Path,
    ) -> None:
        """Download *dep_name* using a fallback asset variant."""
        platform = self.config.machine
        memory = self.config.memory_size
        release = None

        api_url = f"https://api.github.com/repos/{repo}/releases/tags/{ref}"
        try:
            req = urllib.request.Request(api_url)
            req.add_header("Accept", "application/vnd.github+json")
            with urllib.request.urlopen(req, timeout=30) as resp:
                release = json.loads(resp.read())
        except (OSError, ValueError, urllib.error.URLError):
            pass

        if release is None:
            prefix = ref.split("-nanvix-")[0] if "-nanvix-" in ref else ref
            releases_url = f"https://api.github.com/repos/{repo}/releases?per_page=100"
            try:
                req = urllib.request.Request(releases_url)
                req.add_header("Accept", "application/vnd.github+json")
                with urllib.request.urlopen(req, timeout=30) as resp:
                    all_releases = json.loads(resp.read())
            except (OSError, ValueError, urllib.error.URLError) as exc:
                log.warning(f"Cannot query GitHub releases for {dep_name}: {exc}")
                return
            for rel in all_releases:
                tag = rel.get("tag_name", "")
                if tag.startswith(f"{prefix}-nanvix-"):
                    release = rel
                    log.info(f"Using {tag} (fallback for {ref})")
                    break
            if release is None:
                log.warning(f"No compatible release for {dep_name} ({ref})")
                return

        assets = {
            a["name"]: a["browser_download_url"]
            for a in release.get("assets", [])
            if a["name"].endswith(".tar.bz2")
        }

        deployment = self.config.deployment_mode
        candidates = [
            f"{dep_name}-{platform}-{deployment}-{memory}.tar.bz2",
            f"{dep_name}-{platform}-standalone-{memory}.tar.bz2",
            f"{dep_name}-{platform}-single-process-{memory}.tar.bz2",
            f"{dep_name}-{platform}-multi-process-{memory}.tar.bz2",
        ]

        download_url: str | None = None
        chosen: str | None = None
        for name in candidates:
            if name in assets:
                download_url = assets[name]
                chosen = name
                break

        if not download_url:
            for name, url in assets.items():
                if platform in name and name.endswith(f"-{memory}.tar.bz2"):
                    download_url = url
                    chosen = name
                    break
        if not download_url:
            for name, url in assets.items():
                if name.endswith(f"-{memory}.tar.bz2"):
                    download_url = url
                    chosen = name
                    break

        if not download_url or not chosen:
            log.warning(f"No compatible fallback asset for {dep_name}")
            return

        log.info(f"Downloading {chosen} (fallback for {dep_name})...")

        with tempfile.TemporaryDirectory() as tmpdir:
            tarball_path = Path(tmpdir) / chosen
            urllib.request.urlretrieve(download_url, str(tarball_path))

            extract_dir = Path(tmpdir) / "extracted"
            extract_dir.mkdir()
            with tarfile.open(str(tarball_path), "r:bz2") as tf:
                try:
                    tf.extractall(str(extract_dir), filter="data")
                except TypeError:
                    tf.extractall(str(extract_dir))

            lib_dst = buildroot / "lib"
            lib_dst.mkdir(parents=True, exist_ok=True)
            for lib_file in extract_dir.rglob("*.a"):
                shutil.copy2(lib_file, lib_dst / lib_file.name)
                log.info(f"Installed {lib_file.name}")

            inc_dst = buildroot / "include"
            for inc_src in extract_dir.rglob("include"):
                if not inc_src.is_dir():
                    continue
                inc_dst.mkdir(parents=True, exist_ok=True)
                for item in inc_src.iterdir():
                    target = inc_dst / item.name
                    if item.is_dir():
                        shutil.copytree(item, target, dirs_exist_ok=True)
                    else:
                        shutil.copy2(item, target)
                log.info(f"Installed headers for {dep_name}")
                break

            # Extract python-packages/ (e.g. lxml pure-Python files).
            for pkg_src in extract_dir.rglob("python-packages"):
                if not pkg_src.is_dir():
                    continue
                pkg_dst = buildroot / "python-packages"
                pkg_dst.mkdir(parents=True, exist_ok=True)
                for item in pkg_src.iterdir():
                    target = pkg_dst / item.name
                    if item.is_dir():
                        if target.exists():
                            shutil.rmtree(target)
                        shutil.copytree(item, target)
                    else:
                        shutil.copy2(item, target)
                log.info(f"Installed python packages for {dep_name}")
                break


if __name__ == "__main__":
    CPythonBuild.main()
