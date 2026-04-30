import io
import json
from . import constants

_LEVELS = {
    0xA0: "alpha",
    0xB0: "beta",
    0xC0: "candidate",
    0xF0: "final",
}


_TEMPLATE = {
    "schema_version": "1.0",
    "base_prefix": ".",
    "base_interpreter": "python.exe",
    "platform": None,   # Set later
    "language": {
        "version": f"{constants.VER_MAJOR}.{constants.VER_MINOR}",
        "version_info": {
            "major": constants.VER_MAJOR,
            "minor": constants.VER_MINOR,
            "micro": constants.VER_MICRO,
            "releaselevel": _LEVELS.get(constants.VER_FIELD4 & 0xF0, "final"),
            "serial": constants.VER_FIELD4 & 0x0F,
        },
    },
    "implementation": {
        "name": "cpython",
        "cache_tag": f"cpython-{constants.VER_MAJOR}{constants.VER_MINOR}",
        "version": {
            "major": constants.VER_MAJOR,
            "minor": constants.VER_MINOR,
            "micro": constants.VER_MICRO,
            "releaselevel": _LEVELS.get(constants.VER_FIELD4 & 0xF0, "final"),
            "serial": constants.VER_FIELD4 & 0x0F,
        },
    },
    "abi": {
        "flags": [],
        "extension_suffix": ".pyd",
        "stable_abi_suffix": ".pyd",
    },
    "suffixes": {
        "source": [".py"],
        "bytecode": [".pyc"],
        "extensions": [".pyd"],
    },
    "libpython": {
        "dynamic": constants.PYTHON_DLL_NAME,
        "dynamic_stableabi": constants.PYTHON_STABLE_DLL_NAME,
        "link_extensions": True,
    },
    "c_api": {
    },
}


def _with_d(path):
    pre, sep, post = path.partition(".")
    return pre + "_d" + sep + post


def _add_d(data, *args):
    for a in args[:-1]:
        data = data[a]
    a = args[-1]
    v = data[a]
    if isinstance(v, list):
        data[a] = [_with_d(i) for i in data[a]]
    else:
        data[a] = _with_d(data[a])


def get_builddetails(ns):
    if not ns.include_builddetails_json:
        return

    details = dict(_TEMPLATE)

    plat = {
        "win32": "win32",
        "amd64": "win-amd64",
        "arm64": "win-arm64",
    }.get(ns.arch, ns.arch)

    pyd_abi_flags = ""
    if ns.include_freethreaded:
        details["abi"]["flags"].append("t")
        pyd_abi_flags += "t"
    if ns.debug:
        details["abi"]["flags"].append("d")

    norm_plat = plat.replace("-", "_")
    ext_suffix = f".cp{constants.VER_MAJOR}{constants.VER_MINOR}{pyd_abi_flags}-{norm_plat}.pyd"
    details["abi"]["extension_suffix"] = ext_suffix
    details["suffixes"]["extensions"].insert(0, ext_suffix)

    details["platform"] = plat

    if ns.include_dev:
        details["c_api"]["headers"] = "Include"

    if ns.include_freethreaded:
        details["libpython"]["dynamic"] = constants.FREETHREADED_PYTHON_DLL_NAME
        details["libpython"]["dynamic_stableabi"] = constants.FREETHREADED_PYTHON_STABLE_DLL_NAME

    if ns.debug:
        _add_d(details, "base_interpreter")
        _add_d(details, "abi", "stable_abi_suffix")
        _add_d(details, "abi", "extension_suffix")
        _add_d(details, "suffixes", "extensions")
        _add_d(details, "libpython", "dynamic")
        _add_d(details, "libpython", "dynamic_stableabi")

    buffer = io.StringIO()
    json.dump(details, buffer, indent=2)
    yield "build-details.json", ("build-details.json", buffer.getvalue().encode())
