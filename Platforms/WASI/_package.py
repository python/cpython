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
# ✅ include/pythonN.Md?
#   ✅ pyconfig.h
#   ✅ .h files
# ✅ lib
#   ✅ pkgconfig (from Misc/)
#   ✅ pythonN.M
#     ❌ lib-dynload
#     ✅ LICENSE.txt (license_file())
#     ✅ Stuff from build/lib.* (build_dir_files())
#     ✅ Lib/
# ✅ share/man/man1/
# ☐ python.wasm


def python_version(context, debug_ok=False):
    """Calculate the M.N part of Python's version.

    If *debug_ok* is True, then "d" is appended as appropriate.
    """
    details = context.wasi_build_details
    major = details["language"]["version_info"]["major"]
    minor = details["language"]["version_info"]["minor"]
    version = f"{major}.{minor}"
    if debug_ok and context.is_debug:
        version += "d"
    return version


def lib_python(context):
    return pathlib.PurePath("lib") / f"python{python_version(context)}"


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
    pkgconfig = pathlib.PurePath("lib") / "pkgconfig"
    return [
        (
            pkgconfig / f"python-{python_version(context, debug_ok=True)}.pc",
            misc_dir / "python.pc",
        ),
        (
            pkgconfig
            / f"python-{python_version(context, debug_ok=True)}-embed.pc",
            misc_dir / "python-embed.pc",
        ),
    ]


def pkgconfig_symlinks(pkgconfig_files, context):
    assert len(pkgconfig_files) == 2
    embed, plain = (
        (0, 1) if pkgconfig_files[0].name.endswith("-embed.pc") else (1, 0)
    )
    embed_path, plain_path = pkgconfig_files[embed], pkgconfig_files[plain]
    major = context.wasi_build_details["language"]["version_info"]["major"]
    paths = [
        (embed_path.parent / f"python{major}-embed.pc", embed_path),
        (plain_path.parent / f"python{major}.pc", plain_path),
    ]
    if context.is_debug:
        paths += [
            (
                embed_path.parent
                / f"python-{python_version(context)}-embed.pc",
                embed_path,
            ),
            (
                plain_path.parent / f"python-{python_version(context)}.pc",
                plain_path,
            ),
        ]

    return paths


def pyconfig_file(context):
    """Have <build>/pyconfig.h end up in include/pythonXYd?/pyconfig.h."""
    return (
        pathlib.PurePath("include")
        / f"python{python_version(context, debug_ok=True)}"
        / "pyconfig.h",
        context.wasi_build_path / "pyconfig.h",
    )


def header_files(context):
    """Have <build>/Include/*.h end up in include/pythonXYd?/."""
    include_dir = context.checkout / "Include"
    files = []
    for root, dirs, filenames in include_dir.walk():
        for filename in filenames:
            file_path = pathlib.Path(root) / filename
            details = (
                pathlib.PurePath("include")
                / f"python{python_version(context, debug_ok=True)}"
                / file_path.relative_to(include_dir),
                file_path,
            )
            files.append(details)
    return files


def man_file(context):
    """Have Misc/python.man end up in share/man/man1/."""
    man_dir = pathlib.PurePath("share", "man", "man1")
    man_file = context.checkout / "Misc" / "python.man"
    return (
        man_dir / f"python{python_version(context)}.1",
        man_file,
    )


def man_symlink(man_path, context):
    """Symlink pythonN.M.1 to pythonN.1."""
    major = context.wasi_build_details["language"]["version_info"]["major"]
    return (man_path.parent / f"python{major}.1", man_path)


def wasmtime_config_file(context):
    """Have wasmtime.toml end up in etc/pythonXY/."""
    config = context.checkout / "Platforms" / "WASI" / "wasmtime.toml"
    return (
        pathlib.PurePath("etc")
        / f"python{python_version(context)}"
        / "wasmtime.toml",
        config,
    )


def wasm_file(context):
    """Have python.wasm end up at lib/pythonXY/lib-wasm/pythonX.Yd?.wasm."""
    XXX


def python_wasmtime_script(context):
    """Create bin/pythonN.Md?.wasmtime."""
    XXX


def python_wasmtime_symlink(context):
    """Symlink bin/pythonN.Md?.wasmtime.

    - bin/pythonN.M.wasmtime (if debug build)
    - bin/pythonNd?.wasmtime
    - bin/pythonN.wasmtime (if debug build)
    """
    # XXX


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


def symlink_files(files, base):
    for target, source in files:
        (base / target).symlink_to(base / source)


def package(context):
    dist = context.checkout / "dist"
    if dist.exists():
        _shared.log("🧹", f"Deleting {dist} ...")
        shutil.rmtree(dist)

    indent = "  "
    base = dist / filename_stem(context)
    _shared.log("📝", f"Copying files to {base} ...")
    _shared.log("📁", "include/", spacing=indent * 2)
    _shared.log("📁", "pythonN.Md?/", spacing=indent * 3)
    _shared.log("📄", "pyconfig.h", spacing=indent * 4)
    copy_files([pyconfig_file(context)], base)
    _shared.log("📄", "**/*.h", spacing=indent * 4)
    copy_files(header_files(context), base)
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
    pkgconfig_paths = pkgconfig_files(context)
    copy_files(pkgconfig_paths, base)
    symlink_files(
        pkgconfig_symlinks([path for path, _ in pkgconfig_paths], context),
        base,
    )
    _shared.log("📁", "share", spacing=indent * 2)
    _shared.log("📁", "man", spacing=indent * 3)
    _shared.log("📁", "man1", spacing=indent * 4)
    _shared.log("📄", "python*.1", spacing=indent * 5)
    man_path = man_file(context)
    copy_files([man_path], base)
    symlink_files([man_symlink(man_path[0], context)], base)

    _shared.log("📁", "etc/", spacing=indent * 2)
    _shared.log("📁", "pythonN.M/", spacing=indent * 3)
    _shared.log("📄", "wasmtime.toml", spacing=indent * 4)
    copy_files([wasmtime_config_file(context)], base)
