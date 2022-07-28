"""Check extension modules

The script checks shared and built-in extension modules. It verifies that the
modules have been built and that they can be imported successfully. Missing
modules and failed imports are reported to the user. Shared extension
files are renamed on failed import.

Module information is parsed from several sources:

- core modules hard-coded in Modules/config.c.in
- Windows-specific modules that are hard-coded in PC/config.c
- MODULE_{name}_STATE entries in Makefile (provided through sysconfig)
- Various makesetup files:
  - $(srcdir)/Modules/Setup
  - Modules/Setup.[local|bootstrap|stdlib] files, which are generated
    from $(srcdir)/Modules/Setup.*.in files

See --help for more information
"""
import argparse
import collections
import enum
import logging
import os
import pathlib
import re
import sys
import sysconfig
import warnings

from importlib._bootstrap import _load as bootstrap_load
from importlib.machinery import BuiltinImporter, ExtensionFileLoader, ModuleSpec
from importlib.util import spec_from_file_location, spec_from_loader
from typing import Iterable

SRC_DIR = pathlib.Path(__file__).parent.parent.parent

# core modules, hard-coded in Modules/config.h.in
CORE_MODULES = {
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

# Windows-only modules
WINDOWS_MODULES = {
    "_msi",
    "_overlapped",
    "_testconsole",
    "_winapi",
    "msvcrt",
    "nt",
    "winreg",
    "winsound",
}


logger = logging.getLogger(__name__)

parser = argparse.ArgumentParser(
    prog="check_extension_modules",
    description=__doc__,
    formatter_class=argparse.RawDescriptionHelpFormatter,
)

parser.add_argument(
    "--verbose",
    action="store_true",
    help="Verbose, report builtin, shared, and unavailable modules",
)

parser.add_argument(
    "--debug",
    action="store_true",
    help="Enable debug logging",
)

parser.add_argument(
    "--strict",
    action=argparse.BooleanOptionalAction,
    help=(
        "Strict check, fail when a module is missing or fails to import"
        "(default: no, unless env var PYTHONSTRICTEXTENSIONBUILD is set)"
    ),
    default=bool(os.environ.get("PYTHONSTRICTEXTENSIONBUILD")),
)

parser.add_argument(
    "--cross-compiling",
    action=argparse.BooleanOptionalAction,
    help=(
        "Use cross-compiling checks "
        "(default: no, unless env var _PYTHON_HOST_PLATFORM is set)."
    ),
    default="_PYTHON_HOST_PLATFORM" in os.environ,
)

parser.add_argument(
    "--list-module-names",
    action="store_true",
    help="Print a list of module names to stdout and exit",
)


class ModuleState(enum.Enum):
    # Makefile state "yes"
    BUILTIN = "builtin"
    SHARED = "shared"

    DISABLED = "disabled"
    MISSING = "missing"
    NA = "n/a"
    # disabled by Setup / makesetup rule
    DISABLED_SETUP = "disabled_setup"

    def __bool__(self):
        return self.value in {"builtin", "shared"}


ModuleInfo = collections.namedtuple("ModuleInfo", "name state")


class ModuleChecker:
    pybuilddir_txt = "pybuilddir.txt"

    setup_files = (
        # see end of configure.ac
        "Modules/Setup.local",
        "Modules/Setup.stdlib",
        "Modules/Setup.bootstrap",
        SRC_DIR / "Modules/Setup",
    )

    def __init__(self, cross_compiling: bool = False, strict: bool = False):
        self.cross_compiling = cross_compiling
        self.strict_extensions_build = strict
        self.ext_suffix = sysconfig.get_config_var("EXT_SUFFIX")
        self.platform = sysconfig.get_platform()
        self.builddir = self.get_builddir()
        self.modules = self.get_modules()

        self.builtin_ok = []
        self.shared_ok = []
        self.failed_on_import = []
        self.missing = []
        self.disabled_configure = []
        self.disabled_setup = []
        self.notavailable = []

    def check(self):
        for modinfo in self.modules:
            logger.debug("Checking '%s' (%s)", modinfo.name, self.get_location(modinfo))
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
                    if modinfo.state == ModuleState.BUILTIN:
                        self.builtin_ok.append(modinfo)
                    else:
                        assert modinfo.state == ModuleState.SHARED
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
            print("The following *built-in* modules have been successfully built:")
            print_three_column(self.builtin_ok)
            print()

        if verbose and self.shared_ok:
            print("The following *shared* modules have been successfully built:")
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
            print(
                f"The following modules are not available on platform '{self.platform}':"
            )
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
            names.update(WINDOWS_MODULES)
        return names

    def get_builddir(self) -> pathlib.Path:
        try:
            with open(self.pybuilddir_txt, encoding="utf-8") as f:
                builddir = f.read()
        except FileNotFoundError:
            logger.error("%s must be run from the top build directory", __file__)
            raise
        builddir = pathlib.Path(builddir)
        logger.debug("%s: %s", self.pybuilddir_txt, builddir)
        return builddir

    def get_modules(self) -> list[ModuleInfo]:
        """Get module info from sysconfig and Modules/Setup* files"""
        seen = set()
        modules = []
        # parsing order is important, first entry wins
        for modinfo in self.get_core_modules():
            modules.append(modinfo)
            seen.add(modinfo.name)
        for setup_file in self.setup_files:
            for modinfo in self.parse_setup_file(setup_file):
                if modinfo.name not in seen:
                    modules.append(modinfo)
                    seen.add(modinfo.name)
        for modinfo in self.get_sysconfig_modules():
            if modinfo.name not in seen:
                modules.append(modinfo)
                seen.add(modinfo.name)
        logger.debug("Found %i modules in total", len(modules))
        modules.sort()
        return modules

    def get_core_modules(self) -> Iterable[ModuleInfo]:
        """Get hard-coded core modules"""
        for name in CORE_MODULES:
            modinfo = ModuleInfo(name, ModuleState.BUILTIN)
            logger.debug("Found core module %s", modinfo)
            yield modinfo

    def get_sysconfig_modules(self) -> Iterable[ModuleInfo]:
        """Get modules defined in Makefile through sysconfig

        MODBUILT_NAMES: modules in *static* block
        MODSHARED_NAMES: modules in *shared* block
        MODDISABLED_NAMES: modules in *disabled* block
        """
        moddisabled = set(sysconfig.get_config_var("MODDISABLED_NAMES").split())
        if self.cross_compiling:
            modbuiltin = set(sysconfig.get_config_var("MODBUILT_NAMES").split())
        else:
            modbuiltin = set(sys.builtin_module_names)

        for key, value in sysconfig.get_config_vars().items():
            if not key.startswith("MODULE_") or not key.endswith("_STATE"):
                continue
            if value not in {"yes", "disabled", "missing", "n/a"}:
                raise ValueError(f"Unsupported value '{value}' for {key}")

            modname = key[7:-6].lower()
            if modname in moddisabled:
                # Setup "*disabled*" rule
                state = ModuleState.DISABLED_SETUP
            elif value in {"disabled", "missing", "n/a"}:
                state = ModuleState(value)
            elif modname in modbuiltin:
                assert value == "yes"
                state = ModuleState.BUILTIN
            else:
                assert value == "yes"
                state = ModuleState.SHARED

            modinfo = ModuleInfo(modname, state)
            logger.debug("Found %s in Makefile", modinfo)
            yield modinfo

    def parse_setup_file(self, setup_file: pathlib.Path) -> Iterable[ModuleInfo]:
        """Parse a Modules/Setup file"""
        assign_var = re.compile(r"^\w+=")  # EGG_SPAM=foo
        # default to static module
        state = ModuleState.BUILTIN
        logger.debug("Parsing Setup file %s", setup_file)
        with open(setup_file, encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#") or assign_var.match(line):
                    continue
                match line.split():
                    case ["*shared*"]:
                        state = ModuleState.SHARED
                    case ["*static*"]:
                        state = ModuleState.BUILTIN
                    case ["*disabled*"]:
                        state = ModuleState.DISABLED
                    case ["*noconfig*"]:
                        state = None
                    case [*items]:
                        if state == ModuleState.DISABLED:
                            # *disabled* can disable multiple modules per line
                            for item in items:
                                modinfo = ModuleInfo(item, state)
                                logger.debug("Found %s in %s", modinfo, setup_file)
                                yield modinfo
                        elif state in {ModuleState.SHARED, ModuleState.BUILTIN}:
                            # *shared* and *static*, first item is the name of the module.
                            modinfo = ModuleInfo(items[0], state)
                            logger.debug("Found %s in %s", modinfo, setup_file)
                            yield modinfo

    def get_spec(self, modinfo: ModuleInfo) -> ModuleSpec:
        """Get ModuleSpec for builtin or extension module"""
        if modinfo.state == ModuleState.SHARED:
            location = os.fspath(self.get_location(modinfo))
            loader = ExtensionFileLoader(modinfo.name, location)
            return spec_from_file_location(modinfo.name, location, loader=loader)
        elif modinfo.state == ModuleState.BUILTIN:
            return spec_from_loader(modinfo.name, loader=BuiltinImporter)
        else:
            raise ValueError(modinfo)

    def get_location(self, modinfo: ModuleInfo) -> pathlib.Path:
        """Get shared library location in build directory"""
        if modinfo.state == ModuleState.SHARED:
            return self.builddir / f"{modinfo.name}{self.ext_suffix}"
        else:
            return None

    def _check_file(self, modinfo: ModuleInfo, spec: ModuleSpec):
        """Check that the module file is present and not empty"""
        if spec.loader is BuiltinImporter:
            return
        try:
            st = os.stat(spec.origin)
        except FileNotFoundError:
            logger.error("%s (%s) is missing", modinfo.name, spec.origin)
            raise
        if not st.st_size:
            raise ImportError(f"{spec.origin} is an empty file")

    def check_module_import(self, modinfo: ModuleInfo):
        """Attempt to import module and report errors"""
        spec = self.get_spec(modinfo)
        self._check_file(modinfo, spec)
        try:
            with warnings.catch_warnings():
                # ignore deprecation warning from deprecated modules
                warnings.simplefilter("ignore", DeprecationWarning)
                bootstrap_load(spec)
        except ImportError as e:
            logger.error("%s failed to import: %s", modinfo.name, e)
            raise
        except Exception as e:
            logger.exception("Importing extension '%s' failed!", modinfo.name)
            raise

    def check_module_cross(self, modinfo: ModuleInfo):
        """Sanity check for cross compiling"""
        spec = self.get_spec(modinfo)
        self._check_file(modinfo, spec)

    def rename_module(self, modinfo: ModuleInfo) -> None:
        """Rename module file"""
        if modinfo.state == ModuleState.BUILTIN:
            logger.error("Cannot mark builtin module '%s' as failed!", modinfo.name)
            return

        failed_name = f"{modinfo.name}_failed{self.ext_suffix}"
        builddir_path = self.get_location(modinfo)
        if builddir_path.is_symlink():
            symlink = builddir_path
            module_path = builddir_path.resolve().relative_to(os.getcwd())
            failed_path = module_path.parent / failed_name
        else:
            symlink = None
            module_path = builddir_path
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
    if args.debug:
        args.verbose = True
    logging.basicConfig(
        level=logging.DEBUG if args.debug else logging.INFO,
        format="[%(levelname)s] %(message)s",
    )

    checker = ModuleChecker(
        cross_compiling=args.cross_compiling,
        strict=args.strict,
    )
    if args.list_module_names:
        names = checker.list_module_names(all=True)
        for name in sorted(names):
            print(name)
    else:
        checker.check()
        checker.summary(verbose=args.verbose)
        try:
            checker.check_strict_build()
        except RuntimeError as e:
            parser.exit(1, f"\nError: {e}\n")


if __name__ == "__main__":
    main()
