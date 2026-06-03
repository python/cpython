import json
import pathlib
import shutil

import _shared

# https://reproducible-builds.org/docs/archives/
# https://sethmlarson.dev/security-developer-in-residence-weekly-report-14
# - mtime
# - uid
# - gid
# - uname
# - gname
# https://docs.python.org/3/library/tarfile.html#writing-examples

# ☐ bin
#   ☐ pythonN.M.wasmtime
#   ❌ idleN.M
#   ❌ pydocN.M
# ☐ include/pythonN.Md?
#   ☐ pyconfig.h
#   ☐ .h files
# ✅ lib
#   ✅ pkgconfig (from Misc/)
#   ✅ pythonN.M
#     ❌ lib-dynload
#     ✅ LICENSE.txt (license_file())
#     ✅ Stuff from build/lib.* (build_dir_files())
#     ✅ Lib/
# ❌ share/man/man1/
# ☐ python.wasm


def build_dir(context):
    """The path to the build directory pointed to by pybuilddir.txt."""
    relative_dir = (
        (_shared.wasi_build_path(context) / "pybuilddir.txt")
        .read_text()
        .strip()
    )
    return _shared.wasi_build_path(context) / relative_dir


def build_details(context):
    """Get the JSON contents of build-details.json."""
    with (build_dir(context) / "build-details.json").open() as f:
        return json.load(f)


def pythonXY(context, support_debug=False):
    """Calculate the "pythonX.Y" part of a path.

    If *support_debug* is True, then "d" is appended as appropriate.
    """
    details = build_details(context)
    major = details["language"]["version_info"]["major"]
    minor = details["language"]["version_info"]["minor"]
    name = f"python{major}.{minor}"
    if support_debug and "d" in details["abi"]["flags"]:
        name += "d"
    return name


def lib_python(context):
    return pathlib.PurePath("lib") / pythonXY(context)


def license_file(context):
    """Have <src>/LICENSE end up as lib/pythonXY/LICENSE.txt."""
    return (lib_python(context) / "LICENSE.txt", _shared.CHECKOUT / "LICENSE")


def build_dir_files(context):
    """Have build/lib.* files end up in lib/pythonXY.

    Symlinks are skipped as those files are covered by handling
    <build>/Modules.
    """
    return [
        (lib_python(context) / path.name, path)
        for path in build_dir(context).iterdir()
        if path.is_file(follow_symlinks=False)
    ]


def stdlib_files(context):
    """Have <src>/Lib files end up in lib/pythonXY."""
    lib_dir = _shared.CHECKOUT / "Lib"
    lib_files = []
    for root, dirs, files in lib_dir.walk():
        try:
            dirs.remove("__pycache__")
        except ValueError:
            pass

        for file in files:
            file_path = pathlib.Path(root) / file
            details = (
                lib_python(context) / file_path.relative_to(lib_dir),
                file_path
            )
            lib_files.append(details)
    return lib_files


def pkgconfig_files(context):
    """Have <src>/Misc/python*.pc end up in lib/pkgconfig.

    Each file ends up being listed under `python3` and `python-3.N`.
    """
    misc_dir = _shared.wasi_build_path(context) / "Misc"
    details = build_details(context)
    major = details["language"]["version_info"]["major"]
    minor = details["language"]["version_info"]["minor"]
    pkgconfig = pathlib.PurePath("lib") / "pkgconfig"
    return [
        (pkgconfig / f"python{major}.pc", misc_dir / "python.pc"),
        (pkgconfig / f"python-{major}.{minor}.pc", misc_dir / "python.pc"),
        (pkgconfig / f"python3-embed.pc", misc_dir / "python-embed.pc"),
        (pkgconfig / f"python-{major}.{minor}-embed.pc", misc_dir / "python-embed.pc"),
    ]


def filename_stem(context):
    """Calculate the stem of the archive file name."""
    # XXX File name: python-3.15.0a8-wasm32-wasip1.tar.xz
    # Include/patchlevel.h: `#define PY_VERSION              "3.15.0a8+"`
    # `build-details.json` for either/both version and triple


def package(context):
    dist = _shared.CHECKOUT / "dist"
    if dist.exists():
        shutil.rmtree(dist)
    files = []
    files.append(license_file(context))
    files.extend(build_dir_files(context))
    files.extend(stdlib_files(context))
    files.extend(pkgconfig_files(context))
    for dest, src in files:
        target = dist / dest
        target.parent.mkdir(parents=True, exist_ok=True)
        src.copy(target)
