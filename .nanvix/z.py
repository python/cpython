# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

"""Nanvix build script for CPython.

Usage:
    ./z setup      # Download Nanvix sysroot and dependencies
    ./z build      # Cross-compile python.elf and libpython.a
    ./z test       # Run test suite (hello-world on nanvixd.elf)
    ./z benchmark  # Run hello-world benchmark with release ramfs
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

import os
import shutil
import sys

# ---------------------------------------------------------------------------
# Local modules (loaded via importlib since .nanvix/ is not a valid package name)
# ---------------------------------------------------------------------------
import sys as _sys
import tempfile
from pathlib import Path

from nanvix_zutil import (
    CFG_SYSROOT,
    CFG_TOOLCHAIN,
    EXIT_MISSING_DEP,
    ZScript,
    log,
    suffix_dep,
)
from nanvix_zutil.buildroot import (
    Buildroot,
    Dependency,
    extract_nanvix_version_base,
)
from nanvix_zutil.github import resolve_release_with_fallback

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
    "xz": ["liblzma.a"],
}


class CPythonBuild(ZScript):
    """Build script for nanvix/cpython."""

    if sys.platform == "win32":
        SYSROOT_REQUIRED_FILES: tuple[str, ...] = (
            "lib/libposix.a",
            "lib/user.ld",
            "bin/nanvixd.exe",
            "bin/kernel.elf",
            "bin/mkramfs.exe",
        )

        SYSROOT_MULTI_PROCESS_FILES: tuple[str, ...] = ()

    # ---- Local Nanvix overlay --------------------------------------------

    def _overlay_local_nanvix(self) -> None:
        """Re-overlay local Nanvix binaries into the sysroot.

        Called before build/test/release so that local changes are
        picked up even after the initial ``setup()`` run.  Reads the
        ``WITH_NANVIX`` environment variable (set by ``z.sh``) or falls
        back to the path persisted in ``.nanvix/env.json``.

        Delegates to ``Sysroot.overlay_local_nanvix()``.
        """
        nanvix_path = os.environ.get("WITH_NANVIX") or self.config.get(
            _CFG_LOCAL_NANVIX, ""
        )
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

        from nanvix_zutil import Sysroot

        Sysroot(Path(sysroot)).overlay_local_nanvix(Path(nanvix_path))

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

        Delegates sysroot/dependency download, ``--with-nanvix`` overlay,
        and verification to the base class.  Adds cpython-specific
        post-processing: missing-dep fallback and buildroot→sysroot merge.
        """
        # Base class handles: sysroot download, WITH_NANVIX overlay,
        # dependency installation, Windows binaries, and verification.
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

        # For standalone deployment mode, produce an initrd image
        # containing the system daemons and the application binary.
        if self.config.deployment_mode == "standalone":
            self.make_initrd(f"python{config.EXE}")

    def test(self) -> None:
        """Run the CPython test suite (hello + regrtest)."""
        self._overlay_local_nanvix()
        sysroot, toolchain = self._get_host_paths()
        kwargs = self._build_kwargs()

        nanvixd_extra = None

        test_mod.run_all(
            sysroot,
            toolchain,
            self.repo_root,
            **kwargs,
            nanvixd_extra=nanvixd_extra,
            run_fn=lambda *args, **kw: self.run(*args, **kw),  # type: ignore[arg-type]
            docker=self.docker is not None,
        )

    def benchmark(self) -> None:
        """Run hello-world benchmark with a release-style ramfs."""
        self._overlay_local_nanvix()
        sysroot, toolchain = self._get_host_paths()
        kwargs = self._build_kwargs()

        nanvixd_extra = None

        # run_benchmark does not use 'release' — always stages a
        # non-release build and applies release trimming itself.
        bench_kwargs = {k: v for k, v in kwargs.items() if k != "release"}
        test_mod.run_benchmark(
            sysroot,
            toolchain,
            self.repo_root,
            **bench_kwargs,
            nanvixd_extra=nanvixd_extra,
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
        # Remove initrd image generated for standalone mode.
        initrd = self.repo_root / "python.img"
        if initrd.exists():
            initrd.unlink()

    def distclean(self) -> None:
        """Deep clean: remove all build artifacts, caches, and untracked files."""
        build_mod.distclean(self.repo_root)

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
            self._download_dep_fallback(resolved, buildroot)

    def _download_dep_fallback(
        self,
        dep: Dependency,
        buildroot: Path,
    ) -> None:
        """Download *dep* using a fallback asset variant.

        Delegates download and extraction to ``Buildroot.install_dep``
        (which handles ``.tar.gz``, ``.tar.bz2``, and ``.zip``
        transparently).  Adds cpython-specific logic:

        - Fuzzy release discovery (scan releases for ``prefix-nanvix-*``
          when the exact tag is missing).
        - Multiple deployment-mode candidates (standalone, single-process,
          multi-process).
        - Extraction of ``python-packages/`` payload (e.g. lxml).
        """
        dep_name = dep.name
        repo = dep.repo
        ref = str(dep.ref.value)
        platform = self.config.machine
        memory = self.config.memory_size
        deployment = self.config.deployment_mode
        gh_token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")

        # --- Resolve release (with fuzzy fallback via zutils) ---
        release: dict[str, object] | None = None
        base_version = extract_nanvix_version_base(ref)
        try:
            if base_version is not None:
                release, _ = resolve_release_with_fallback(
                    repo=repo,
                    version_specifier=ref,
                    base_version=base_version,
                    gh_token=gh_token,
                )
            else:
                from nanvix_zutil.github import resolve_release

                release = resolve_release(
                    repo=repo,
                    version_specifier=ref,
                    gh_token=gh_token,
                )
        except SystemExit:
            log.warning(f"No compatible release for {dep_name} ({ref})")
            return

        # --- Try deployment-mode candidates via Buildroot.install_dep ---
        br = Buildroot(buildroot)
        modes = [deployment, "standalone", "single-process", "multi-process"]
        seen: set[str] = set()
        installed = False
        for mode in modes:
            if mode in seen:
                continue
            seen.add(mode)
            fallback_dep = Dependency(
                name=dep_name,
                repo=repo,
                ref=dep.ref,
            )
            try:
                br.install_dep(
                    fallback_dep,
                    machine=platform,
                    deployment_mode=mode,
                    memory_size=memory,
                    gh_token=gh_token,
                    _release=release,
                )
                installed = True
                break
            except SystemExit:
                continue

        if not installed:
            log.warning(f"No compatible fallback asset for {dep_name}")
            return

        # --- CPython-specific: extract python-packages/ (e.g. lxml) ---
        cache_dir = buildroot.parent / "cache"
        asset_prefix = f"{dep_name}-{platform}-"
        for cached in sorted(cache_dir.iterdir()) if cache_dir.is_dir() else []:
            if not cached.name.startswith(asset_prefix):
                continue
            self._extract_python_packages(cached, buildroot)
            break

    def _extract_python_packages(self, asset_path: Path, buildroot: Path) -> None:
        """Extract ``python-packages/`` from an archive into *buildroot*."""
        import tarfile
        import zipfile

        with tempfile.TemporaryDirectory() as tmpdir:
            extract_dir = Path(tmpdir) / "extracted"
            extract_dir.mkdir()

            if zipfile.is_zipfile(asset_path):
                with zipfile.ZipFile(asset_path) as zf:
                    for member in zf.namelist():
                        if "python-packages" not in member:
                            continue
                        if os.path.isabs(member) or ".." in member.split("/"):
                            continue
                        dest = (extract_dir / member).resolve()
                        if not dest.is_relative_to(extract_dir.resolve()):
                            continue
                        zf.extract(member, extract_dir)
            else:
                with tarfile.open(str(asset_path), "r:*") as tf:
                    pkg_members = [
                        m
                        for m in tf.getmembers()
                        if "python-packages" in m.name
                        and not os.path.isabs(m.name)
                        and ".." not in m.name.split("/")
                    ]
                    if not pkg_members:
                        return
                    try:
                        tf.extractall(
                            str(extract_dir), members=pkg_members, filter="data"
                        )
                    except TypeError:
                        tf.extractall(str(extract_dir), members=pkg_members)

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
                log.info(f"Installed python packages from {asset_path.name}")
                break


if __name__ == "__main__":
    CPythonBuild.main()
