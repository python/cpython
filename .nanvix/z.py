# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

"""Nanvix build script for CPython.

Usage:
    ./z setup     # Download Nanvix sysroot and dependencies
    ./z build     # Cross-compile python.elf and libpython3.12.a
    ./z test      # Run test suite (hello-world on nanvixd.elf)
    ./z release   # Package release tarballs (sysroot + buildroot)
    ./z clean     # Remove build artifacts
"""

import json
import os
import shutil
import sys
import tarfile
import tempfile
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
# Use /sysroot so that release tarballs don't contain ephemeral runner paths.
_DEFAULT_INSTALL_PREFIX = "/sysroot"


# Map dependency names to the library files they install into buildroot/lib.
_DEP_EXPECTED_LIBS: dict[str, list[str]] = {
    "bzip2": ["libbz2.a"],
    "libffi": ["libffi.a"],
    "zlib": ["libz.a"],
    "sqlite": ["libsqlite3.a"],
    "openssl": ["libssl.a", "libcrypto.a"],
}


class CPythonBuild(ZScript):
    """Build script for nanvix/cpython."""

    # ---- Make invocation -------------------------------------------------

    def _run_make(self, make_args: list[str]) -> None:
        """Execute a make command, delegating Docker wrapping to the Makefile.

        On Windows, passes ``DOCKER_HOST_MODE=windows`` so that
        ``.nanvix/mk/docker-host.mk`` handles tar-based source copying,
        CRLF normalization, and output copy-back inside Docker.
        """
        if sys.platform == "win32":
            if not shutil.which("docker"):
                log.fatal(
                    "Docker is required for cross-compilation on Windows "
                    "but was not found on PATH.",
                    code=EXIT_MISSING_DEP,
                    hint="Install Docker Desktop: "
                    "https://docs.docker.com/desktop/install/windows/",
                )
            # Insert DOCKER_HOST_MODE right after CONFIG_NANVIX=y so the
            # Makefile activates the host-side Docker wrapper.
            make_args.insert(4, "DOCKER_HOST_MODE=windows")
        self.run(*make_args, cwd=self.repo_root, docker=False)

    # ---- Make argument builder -------------------------------------------

    def _make_args(self, *targets: str, with_install_prefix: bool = True) -> list[str]:
        """Build the common make argument list."""
        sysroot = self.config.get(CFG_SYSROOT, "")
        if not sysroot:
            log.fatal(
                f"{CFG_SYSROOT} is not set.",
                code=EXIT_MISSING_DEP,
                hint="Run `./z setup` first to download the sysroot.",
            )
        toolchain = self.config.get(CFG_TOOLCHAIN, "/opt/nanvix")

        args = [
            "make",
            "-f",
            "Makefile.nanvix",
            f"{_MAKE_VAR_CONFIG}=y",
            f"{_MAKE_VAR_HOME}={sysroot}",
            f"{_MAKE_VAR_TOOLCHAIN}={toolchain}",
        ]

        args.extend(
            [
                f"{_MAKE_VAR_PLATFORM}={self.config.machine}",
                f"{_MAKE_VAR_PROCESS_MODE}={self.config.deployment_mode}",
                f"{_MAKE_VAR_MEMORY_SIZE}={self.config.memory_size}",
            ]
        )

        if with_install_prefix:
            args.append(f"{_MAKE_VAR_INSTALL_PREFIX}={_DEFAULT_INSTALL_PREFIX}")

        release = os.environ.get(_MAKE_VAR_RELEASE, "no")
        args.append(f"{_MAKE_VAR_RELEASE}={release}")

        args.extend(targets)
        return args

    def setup(self) -> None:
        """Download the Nanvix sysroot and dependencies.

        Overrides the base ``setup()`` to handle dependency resolution
        with fallback: when a dependency hasn't been built for the exact
        sysroot version, the newest compatible release is used instead.

        The base class is not called for dependency installation because
        its retry loop wastes ~30 seconds on expected 404s before raising.
        """
        # ---- Step 1: download and verify the sysroot (from base class) ----
        from nanvix_zutil import Sysroot, CFG_GH_TOKEN

        self.sysroot = Sysroot.download(
            machine=self.config.machine,
            deployment_mode=self.config.deployment_mode,
            memory_size=self.config.memory_size,
            tag=self.manifest.sysroot_ref.value,
            gh_token=self.config.get(CFG_GH_TOKEN),
            dest=self.nanvix_dir / "sysroot",
            config=self.config,
        )
        self.sysroot.verify(self.sysroot_required_files())
        self.config.set(CFG_SYSROOT, str(self.sysroot.path))

        # ---- Step 2: install dependencies with fallback resolution ----
        self._install_missing_deps()
        self.config.save()

        buildroot = self.nanvix_dir / "buildroot"
        sysroot = self.config.get(CFG_SYSROOT, "")
        if not sysroot or not buildroot.is_dir():
            return

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

    def build(self) -> None:
        """Cross-compile python.elf and libpython3.12.a for Nanvix."""
        self._run_make(self._make_args("all"))

    def test(self) -> None:
        """Run the CPython test suite."""
        targets = self.targets if self.targets else ["test"]
        self._run_make(self._make_args(*targets))

    def release(self) -> None:
        """Package the CPython release tarballs and verify them."""
        os.environ[_MAKE_VAR_RELEASE] = "yes"
        self._run_make(self._make_args("package"))
        self._run_make(self._make_args("verify-package"))

    def clean(self) -> None:
        """Remove build artifacts."""
        if sys.platform == "win32":
            # On Windows, clean does not need Docker — just remove local files.
            for name in (".nanvix-configured", "python.elf", "python.exe"):
                p = self.repo_root / name
                if p.is_file():
                    p.unlink()
                    log.info(f"Removed {name}")
            for name in ("_test_staging", "staging"):
                p = self.repo_root / name
                if p.is_dir():
                    shutil.rmtree(p)
                    log.info(f"Removed {name}/")
            test_staging = self.repo_root / ".nanvix" / "_test_staging"
            if test_staging.is_dir():
                shutil.rmtree(test_staging)
                log.info("Removed .nanvix/_test_staging/")
        else:
            self.run(
                "make", "-f", "Makefile.nanvix", "clean",
                cwd=self.repo_root,
            )

    # ---- Fallback dependency download ------------------------------------

    def _install_missing_deps(self) -> None:
        """Download missing dependency libraries using fallback assets."""
        buildroot = self.nanvix_dir / "buildroot"
        buildroot.mkdir(parents=True, exist_ok=True)
        lib_dir = buildroot / "lib"

        sysroot_tag = self.manifest.sysroot_ref.value
        nanvix_version = sysroot_tag.removeprefix("v") if sysroot_tag else ""

        for dep in self.manifest.dependencies:
            expected = _DEP_EXPECTED_LIBS.get(dep.name, [])
            if not expected:
                continue
            if all((lib_dir / lib).exists() for lib in expected):
                continue
            resolved = suffix_dep(dep, nanvix_version) if nanvix_version else dep
            self._download_dep_fallback(
                resolved.name, resolved.repo, resolved.ref.value, buildroot
            )

    def _download_dep_fallback(
        self, dep_name: str, repo: str, ref: str, buildroot: Path,
    ) -> None:
        """Download *dep_name* using a fallback asset variant.

        First tries the exact release tag. If that fails (e.g. not yet
        built for the current sysroot version), scans all releases for
        the newest matching tag.
        """
        platform = self.config.machine
        memory = self.config.memory_size
        release = None

        # 1. Try exact tag.
        api_url = f"https://api.github.com/repos/{repo}/releases/tags/{ref}"
        try:
            req = urllib.request.Request(api_url)
            req.add_header("Accept", "application/vnd.github+json")
            with urllib.request.urlopen(req, timeout=30) as resp:
                release = json.loads(resp.read())
        except (OSError, ValueError, urllib.error.URLError):
            pass

        # 2. Fallback: find the newest release whose tag shares the same
        #    upstream version prefix (e.g. "1.3.1-nanvix-*").
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
            # Prefer assets matching the platform.
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
                # Use filter="data" to prevent path traversal attacks.
                # The filter parameter requires Python 3.12+; fall back
                # gracefully on older interpreters.
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


if __name__ == "__main__":
    CPythonBuild.main()
