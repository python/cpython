"""Check shared extension modules
"""

import argparse
import collections
import enum
import importlib._bootstrap
from importlib.machinery import ExtensionFileLoader, BuiltinImporter
from importlib.util import spec_from_file_location, spec_from_loader
import os
import pathlib
import logging
import sys
import sysconfig
import warnings


PYBUILDDIR_TXT = "pybuilddir.txt"
EXT_SUFFIX = sysconfig.get_config_var("EXT_SUFFIX")
CROSS_COMPILING = "_PYTHON_HOST_PLATFORM" in os.environ


logger = logging.getLogger(__name__)

parser = argparse.ArgumentParser(
    prog="check_shared_ext", description="Check shared extension modules"
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="Enable debug logging",
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


def get_builddir(args: argparse.Namespace) -> pathlib.Path:
    with open(PYBUILDDIR_TXT, encoding="utf-8") as f:
        builddir = f.read()
    builddir = pathlib.Path(builddir)
    logger.debug("%s: %s", PYBUILDDIR_TXT, builddir)
    return builddir


def get_makefile_modules(args: argparse.Namespace) -> list[ModuleInfo]:
    """Get list of modules from Makefile

    MODBUILT_NAMES: modules in *static* block
    MODSHARED_NAMES: modules in *shared* block
    MODDISABLED_NAMES: modules in *disabled* block

    Modules built by setup.py addext() have a MODULE_{modname} entry,
    but are not listed in MODSHARED_NAMES.

    Modules built by old-style setup.py add() have neither a  MODULE_{modname}
    entry nor an entry in MODSHARED_NAMES.
    """
    moddisabled = set(sysconfig.get_config_var("MODDISABLED_NAMES").split())
    modbuiltin = set(sys.builtin_module_names)

    modules = []
    for key, value in sysconfig.get_config_vars().items():
        if not key.startswith("MODULE_"):
            continue
        if key.endswith(
            ("_CFLAGS", "_DEPS", "_LDFLAGS", "_OBJS", "CTYPES_MALLOC_CLOSURE")
        ):
            continue
        if value not in {"yes", "disabled", "n/a"}:
            raise ValueError(f"Unsupported {value} for {key}")

        modname = key[7:].lower()
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

        if state and not is_builtin:
            builddir_path = args.builddir / f"{modname}{args.ext_suffix}"
        else:
            builddir_path = None

        modules.append(ModuleInfo(modname, state, is_builtin, builddir_path))

    modules.sort()
    return modules


def check_module(args: argparse.Namespace, modinfo: ModuleInfo):
    """Attempt to import module and report errors"""
    if not modinfo.state:
        logger.debug("Skipping '%s' (%s)", modinfo.name, modinfo.state)
        return None

    if modinfo.is_builtin:
        spec = spec_from_loader(modinfo.name, loader=BuiltinImporter)
    else:
        location = os.fspath(modinfo.builddir_path)
        loader = ExtensionFileLoader(modinfo.name, location)
        spec = spec_from_file_location(modinfo.name, location, loader=loader)

    logger.debug("Checking '%s' (%s)", modinfo.name, modinfo.builddir_path)
    try:
        with warnings.catch_warnings():
            # ignore deprecation warning from deprecated modules
            warnings.simplefilter("ignore", DeprecationWarning)
            importlib._bootstrap._load(spec)
    except ImportError as e:
        logger.error("%s failed to import: %s", modinfo.name, e)
        return e
    except Exception as e:
        logger.exception("Importing extension '%s' failed!", modinfo.name)
        return e
    else:
        return None


def rename_module(args: argparse.Namespace, modinfo: ModuleInfo) -> None:
    """Rename module file"""
    if modinfo.builddir_path is None:
        logger.error("Cannot mark builtin module '%s' as failed!", modinfo.name)
        return

    failed_name = f"{modinfo.name}_failed{args.ext_suffix}"
    if modinfo.builddir_path.is_symlink():
        symlink = modinfo.builddir_path
        module_path = modinfo.builddir_path.resolve().relative_to(os.getcwd())
        failed_path = module_path.parent / failed_name
    else:
        symlink = None
        module_path = modinfo.builddir_path
        failed_path = args.builddir / failed_name

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
    args.ext_suffix = EXT_SUFFIX

    logging.basicConfig(
        level=logging.DEBUG if args.debug else logging.INFO,
        format="[%(levelname)s] %(message)s",
    )

    args.builddir = get_builddir(args)
    args.modules = get_makefile_modules(args)

    if CROSS_COMPILING:
        parser.exit(0, f"SKIP, cannot check modules when cross-compiling\n")

    failed = []

    for modinfo in args.modules:
        err = check_module(args, modinfo)
        if err is not None:
            failed.append(modinfo)

    for modinfo in failed:
        if modinfo.builddir_path is not None:
            rename_module(args, modinfo)

    # TODO: report missing, disabled, and n/a modules

    if failed:
        parser.exit(
            2,
            f"{len(failed)} of {len(args.modules)} module(s) failed to import: "
            f"{' '.join(modinfo.name for modinfo in failed)}\n",
        )
    else:
        parser.exit(0, f"{len(args.modules)} modules checked.\n")


if __name__ == "__main__":
    main()
