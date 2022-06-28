"""Check shared extension modules
"""

import argparse
import collections
import enum
import importlib._bootstrap
from importlib.machinery import BuiltinImporter, ExtensionFileLoader, ModuleSpec
from importlib.util import spec_from_file_location, spec_from_loader
import os
import pathlib
import logging
import sys
import sysconfig
import warnings


logger = logging.getLogger(__name__)

parser = argparse.ArgumentParser(
    prog="check_shared_ext", description="Check shared extension modules"
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="Enable debug logging",
)
parser.add_argument(
    "--list-module-names",
    action="store_true",
    help="Print a list of module names to stdout",
)


def validate_tzpath():
    base_tzpath = sysconfig.get_config_var("TZPATH")
    if not base_tzpath:
        return

    tzpaths = base_tzpath.split(os.pathsep)
    bad_paths = [tzpath for tzpath in tzpaths if not os.path.isabs(tzpath)]
    if bad_paths:
        raise ValueError(
            "TZPATH must contain only absolute paths, found:\n"
            f"{tzpaths!r}\n"
            "with invalid paths:\n"
            f"{bad_paths!r}"
        )


class ModuleState(enum.Enum):
    YES = "yes"
    DISABLED = "disabled"
    MISSING = "missing"
    NA = "n/a"
    # disabled by Setup / makesetup rule
    DISABLED_SETUP = "disabled_setup"

    def __bool__(self):
        return self.value == "yes"


ModuleInfo = collections.namedtuple("ModuleInfo", "name state is_builtin builddir_path")


class CheckExtensions:
    pybuilddir_txt = "pybuilddir.txt"
    # defined in Modules/config.h.in
    config_h_modules = frozenset(
        {
            "_ast",
            "_imp",
            "_string",
            "_tokenize",
            "_warnings",
            "builtins",
            "gc",
            "marshal",
            "sys",
        }
    )
    # built by setup.py
    setup_py_modules = frozenset({"_curses", "_curses_panel", "_dbm", "readline"})
    # Windows-only modules
    windows_modules = frozenset(
        {
            "_msi",
            "_overlapped",
            "_testconsole",
            "_winapi",
            "msvcrt",
            "nt",
            "winreg",
            "winsound",
        }
    )

    def __init__(self):
        self.cross_compiling = "_PYTHON_HOST_PLATFORM" in os.environ
        self.strict_extensions_build = os.environ.get("PYTHONSTRICTEXTENSIONBUILD")
        self.ext_suffix = sysconfig.get_config_var("EXT_SUFFIX")
        self.platform = sysconfig.get_platform()
        self.builddir = self.get_builddir()
        self.modules = self.get_makefile_modules()

        self.builtin_ok = []
        self.shared_ok = []
        self.failed_on_import = []
        self.missing = []
        self.disabled_configure = []
        self.disabled_setup = []
        self.notavailable = []

    def check(self):
        for modinfo in self.modules:
            logger.debug("Checking '%s' (%s)", modinfo.name, modinfo.builddir_path)
            if modinfo.state == ModuleState.DISABLED:
                self.disabled_configure.append(modinfo)
            elif modinfo.state == ModuleState.DISABLED_SETUP:
                self.disabled_setup.append(modinfo)
            elif modinfo.state == ModuleState.MISSING:
                self.missing.append(modinfo)
            elif modinfo.state == ModuleState.NA:
                self.notavailable.append(modinfo)
            else:
                try:
                    if self.cross_compiling:
                        self.check_module_cross(modinfo)
                    else:
                        self.check_module_import(modinfo)
                except (ImportError, FileNotFoundError):
                    self.rename_module(modinfo)
                    self.failed_on_import.append(modinfo)
                else:
                    if modinfo.is_builtin:
                        self.builtin_ok.append(modinfo)
                    else:
                        self.shared_ok.append(modinfo)

    def summary(self, *, verbose: bool = False):
        longest = max([len(e.name) for e in self.modules], default=0)

        def print_three_column(modinfos: list[ModuleInfo]):
            names = [modinfo.name for modinfo in modinfos]
            names.sort(key=str.lower)
            # guarantee zip() doesn't drop anything
            while len(names) % 3:
                names.append("")
            for l, m, r in zip(names[::3], names[1::3], names[2::3]):
                print("%-*s   %-*s   %-*s" % (longest, l, longest, m, longest, r))

        if verbose and self.builtin_ok:
            print("The following built-in modules have been successfully built:")
            print_three_column(self.builtin_ok)
            print()

        if verbose and self.shared_ok:
            print("The following shared modules have been successfully built:")
            print_three_column(self.shared_ok)
            print()

        if self.disabled_configure:
            print("The following modules are *disabled* in configure script:")
            print_three_column(self.disabled_configure)
            print()

        if self.disabled_setup:
            print("The following modules are *disabled* in Modules/Setup files:")
            print_three_column(self.disabled_setup)
            print()

        if verbose and self.notavailable:
            print(f"The following modules are not available on {self.platform}:")
            print_three_column(self.notavailable)
            print()

        if self.missing:
            print("The necessary bits to build these optional modules were not found:")
            print_three_column(self.missing)
            print("To find the necessary bits, look in configure.ac and config.log.")
            print()

        if self.failed_on_import:
            print(
                "Following modules built successfully "
                "but were removed because they could not be imported:"
            )
            print_three_column(self.failed_on_import)
            print()

        if any(
            modinfo.name == "_ssl" for modinfo in self.missing + self.failed_on_import
        ):
            print("Could not build the ssl module!")
            print("Python requires a OpenSSL 1.1.1 or newer")
            if sysconfig.get_config_var("OPENSSL_LDFLAGS"):
                print("Custom linker flags may require --with-openssl-rpath=auto")
            print()

        disabled = len(self.disabled_configure) + len(self.disabled_setup)
        print(
            f"Checked {len(self.modules)} modules ("
            f"{len(self.builtin_ok)} built-in, "
            f"{len(self.shared_ok)} shared, "
            f"{len(self.notavailable)} n/a on {self.platform}, "
            f"{disabled} disabled, "
            f"{len(self.missing)} missing, "
            f"{len(self.failed_on_import)} failed on import)"
        )

    def check_strict_build(self):
        """Fail if modules are missing and it's a strict build"""
        if self.strict_extensions_build and (self.failed_on_import or self.missing):
            raise RuntimeError("Failed to build some stdlib modules")

    def list_module_names(self, *, all: bool = False) -> set:
        names = {modinfo.name for modinfo in self.modules}
        if all:
            names.update(self.config_h_modules)
            names.update(self.setup_py_modules)
            names.update(self.windows_modules)
        return names

    def get_builddir(self) -> pathlib.Path:
        with open(self.pybuilddir_txt, encoding="utf-8") as f:
            builddir = f.read()
        builddir = pathlib.Path(builddir)
        logger.debug("%s: %s", self.pybuilddir_txt, builddir)
        return builddir

    def get_makefile_modules(self) -> list[ModuleInfo]:
        """Get list of modules from Makefile

        MODBUILT_NAMES: modules in *static* block
        MODSHARED_NAMES: modules in *shared* block
        MODDISABLED_NAMES: modules in *disabled* block

        Modules built by setup.py addext() have a MODULE_{modname}_STATE entry,
        but are not listed in MODSHARED_NAMES.

        Modules built by old-style setup.py add() have neither a  MODULE_{modname}
        entry nor an entry in MODSHARED_NAMES.
        """
        moddisabled = set(sysconfig.get_config_var("MODDISABLED_NAMES").split())
        if self.cross_compiling:
            modbuiltin = set(sysconfig.get_config_var("MODBUILT_NAMES").split())
        else:
            modbuiltin = set(sys.builtin_module_names)

        modules = []
        for key, value in sysconfig.get_config_vars().items():
            if not key.startswith("MODULE_") or not key.endswith("_STATE"):
                continue
            if value not in {"yes", "disabled", "missing", "n/a"}:
                raise ValueError(f"Unsupported value '{value}' for {key}")

            modname = key[7:-6].lower()
            is_builtin = modname in modbuiltin
            if modname in moddisabled:
                # Setup "*disabled*" rule
                state = ModuleState.DISABLED_SETUP
            else:
                try:
                    state = ModuleState(value)
                except ValueError:
                    logger.exception("Invalid module state for %s", modname)
                    raise

            if is_builtin:
                builddir_path = None
            else:
                builddir_path = self.builddir / f"{modname}{self.ext_suffix}"

            modules.append(ModuleInfo(modname, state, is_builtin, builddir_path))

        modules.sort()
        return modules

    def get_spec(self, modinfo: ModuleInfo) -> ModuleSpec:
        """Get ModuleSpec for builtin or extension module"""
        if modinfo.is_builtin:
            return spec_from_loader(modinfo.name, loader=BuiltinImporter)
        else:
            location = os.fspath(modinfo.builddir_path)
            loader = ExtensionFileLoader(modinfo.name, location)
            return spec_from_file_location(modinfo.name, location, loader=loader)

    def check_module_import(self, modinfo: ModuleInfo):
        """Attempt to import module and report errors"""
        spec = self.get_spec(modinfo)
        try:
            with warnings.catch_warnings():
                # ignore deprecation warning from deprecated modules
                warnings.simplefilter("ignore", DeprecationWarning)
                importlib._bootstrap._load(spec)
        except ImportError as e:
            logger.error("%s failed to import: %s", modinfo.name, e)
            raise
        except Exception as e:
            logger.exception("Importing extension '%s' failed!", modinfo.name)
            raise

    def check_module_cross(self, modinfo: ModuleInfo):
        """Sanity check for cross compiling"""
        spec = self.get_spec(modinfo)
        if spec.loader is BuiltinImporter:
            return
        st = os.stat(spec.origin)
        if not st.st_size:
            raise ImportError(f"{spec.origin} is an empty file")

    def rename_module(self, modinfo: ModuleInfo) -> None:
        """Rename module file"""
        if modinfo.builddir_path is None:
            logger.error("Cannot mark builtin module '%s' as failed!", modinfo.name)
            return

        failed_name = f"{modinfo.name}_failed{self.ext_suffix}"
        if modinfo.builddir_path.is_symlink():
            symlink = modinfo.builddir_path
            module_path = modinfo.builddir_path.resolve().relative_to(os.getcwd())
            failed_path = module_path.parent / failed_name
        else:
            symlink = None
            module_path = modinfo.builddir_path
            failed_path = self.builddir / failed_name

        # remove old failed file
        failed_path.unlink(missing_ok=True)
        # remove symlink
        if symlink is not None:
            symlink.unlink(missing_ok=True)
        # rename shared extension file
        try:
            module_path.rename(failed_path)
        except FileNotFoundError:
            logger.debug("Shared extension file '%s' does not exist.", module_path)
        else:
            logger.debug("Rename '%s' -> '%s'", module_path, failed_path)


def main():
    args = parser.parse_args()
    logging.basicConfig(
        level=logging.DEBUG if args.debug else logging.INFO,
        format="[%(levelname)s] %(message)s",
    )

    checker = CheckExtensions()
    if args.list_module_names:
        names = checker.list_module_names(all=True)
        for name in sorted(names):
            print(name)
    else:
        checker.check()
        checker.summary(verbose=args.debug)
        checker.check_strict_build()
        validate_tzpath()


if __name__ == "__main__":
    main()
