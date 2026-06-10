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


def pythonXY(context, support_debug=False):
    """Calculate the "pythonX.Y" part of a path.

    If *support_debug* is True, then "d" is appended as appropriate.
    """
    details = context.wasi_build_details
    major = details["language"]["version_info"]["major"]
    minor = details["language"]["version_info"]["minor"]
    name = f"python{major}.{minor}"
    if support_debug and context.is_debug:
        name += "d"
    return name


def lib_python(context):
    return pathlib.PurePath("lib") / pythonXY(context)


def license_file(context):
    """Have <src>/LICENSE end up as lib/pythonXY/LICENSE.txt."""
    return (lib_python(context) / "LICENSE.txt", context.checkout / "LICENSE")


def build_dir_files(context):
    """Have build/lib.* files end up in lib/pythonXY.

    Symlinks are skipped as those files are covered by handling
    <build>/Modules.
    """
    return [
        (lib_python(context) / path.name, path)
        for path in context.wasi_pybuilddir.iterdir()
        if path.is_file(follow_symlinks=False)
    ]


def stdlib_files(context):
    """Have <src>/Lib files end up in lib/pythonXY."""
    lib_dir = context.checkout / "Lib"
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
                file_path,
            )
            lib_files.append(details)
    return lib_files


def pkgconfig_files(context):
    """Have <src>/Misc/python*.pc end up in lib/pkgconfig.

    Each file ends up being listed under `python3` and `python-3.N`.
    """
    misc_dir = context.wasi_build_path / "Misc"
    details = context.wasi_build_details
    major = details["language"]["version_info"]["major"]
    minor = details["language"]["version_info"]["minor"]
    pkgconfig = pathlib.PurePath("lib") / "pkgconfig"
    return [
        (pkgconfig / f"python{major}.pc", misc_dir / "python.pc"),
        (pkgconfig / f"python-{major}.{minor}.pc", misc_dir / "python.pc"),
        (pkgconfig / f"python3-embed.pc", misc_dir / "python-embed.pc"),
        (
            pkgconfig / f"python-{major}.{minor}-embed.pc",
            misc_dir / "python-embed.pc",
        ),
    ]


def filename_stem(context):
    """Calculate the stem of the archive file name."""
    version_info = context.wasi_build_details["language"]["version_info"]
    version = f"python-{version_info['major']}.{version_info['minor']}.{version_info['micro']}"
    if version_info["releaselevel"] != "final":
        version += version_info["releaselevel"][0] + str(
            version_info["serial"]
        )

    return f"{version}-{context.host_triple}"


def copy_files(files, base):
    for dest, src in files:
        target = base / dest
        target.parent.mkdir(parents=True, exist_ok=True)
        src.copy(target)


def package(context):
    dist = context.checkout / "dist"
    if dist.exists():
        _shared.log("🧹", f"Deleting {dist} ...")
        shutil.rmtree(dist)

    indent = "  "
    base = dist / filename_stem(context)
    _shared.log("📝", f"Copying files to {base} ...")
    _shared.log("📁", "lib/", spacing=indent * 2)
    _shared.log("📁", "pythonN.M/", spacing=indent * 3)
    _shared.log("📄", "LICENSE.txt", spacing=indent * 4)
    copy_files([license_file(context)], base)
    _shared.log("📄", "files in pybuilddir.txt", spacing=indent * 4)
    copy_files(build_dir_files(context), base)
    _shared.log("📄", "**/*.py", spacing=indent * 4)
    copy_files(stdlib_files(context), base)
    _shared.log("📁", "pkgconfig/", spacing=indent * 3)
    _shared.log("📄", "python*.pc", spacing=indent * 4)
    copy_files(pkgconfig_files(context), base)
