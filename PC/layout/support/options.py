"""
List of optional components.
"""

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "3.8"


__all__ = []


def public(f):
    __all__.append(f.__name__)
    return f


OPTIONS = {
    "stable": {"help": "stable ABI stub"},
    "pip": {"help": "pip"},
    "pip-user": {"help": "pip.ini file for default --user"},
    "tcltk": {"help": "Tcl, Tk and tkinter"},
    "idle": {"help": "Idle"},
    "tests": {"help": "test suite"},
    "tools": {"help": "tools"},
    "venv": {"help": "venv"},
    "dev": {"help": "headers and libs"},
    "symbols": {"help": "symbols"},
    "underpth": {"help": "a python._pth file", "not-in-all": True},
    "launchers": {"help": "specific launchers"},
    "appxmanifest": {"help": "an appxmanifest"},
    "props": {"help": "a python.props file"},
    "nuspec": {"help": "a python.nuspec file"},
    "chm": {"help": "the CHM documentation"},
    "html-doc": {"help": "the HTML documentation"},
    "freethreaded": {"help": "freethreaded binaries", "not-in-all": True},
    "alias": {"help": "aliased python.exe entry-point binaries"},
    "alias3": {"help": "aliased python3.exe entry-point binaries"},
    "alias3x": {"help": "aliased python3.x.exe entry-point binaries"},
    "install-json": {"help": "a PyManager __install__.json file"},
    "install-embed-json": {"help": "a PyManager __install__.json file for embeddable distro"},
    "install-test-json": {"help": "a PyManager __install__.json for the test distro"},
}


PRESETS = {
    "appx": {
        "help": "APPX package",
        "options": [
            "stable",
            "pip",
            "tcltk",
            "idle",
            "venv",
            "dev",
            "launchers",
            "appxmanifest",
            "alias",
            "alias3x",
            # XXX: Disabled for now "precompile",
        ],
    },
    "nuget": {
        "help": "nuget package",
        "options": [
            "dev",
            "pip",
            "stable",
            "venv",
            "props",
            "nuspec",
            "alias",
        ],
    },
    "iot": {"help": "Windows IoT Core", "options": ["alias", "stable", "pip"]},
    "default": {
        "help": "development kit package",
        "options": [
            "stable",
            "pip",
            "tcltk",
            "idle",
            "tests",
            "venv",
            "dev",
            "symbols",
            "html-doc",
            "alias",
        ],
    },
    "embed": {
        "help": "embeddable package",
        "options": [
            "alias",
            "stable",
            "zip-lib",
            "flat-dlls",
            "underpth",
            "precompile",
        ],
    },
    "pymanager": {
        "help": "PyManager package",
        "options": [
            "stable",
            "pip",
            "tcltk",
            "idle",
            "venv",
            "dev",
            "html-doc",
            "install-json",
        ],
    },
    "pymanager-test": {
        "help": "PyManager test package",
        "options": [
            "stable",
            "pip",
            "tcltk",
            "idle",
            "venv",
            "dev",
            "html-doc",
            "symbols",
            "tests",
            "install-test-json",
        ],
    },
}


@public
def get_argparse_options():
    for opt, info in OPTIONS.items():
        help = "When specified, includes {}".format(info["help"])
        if info.get("not-in-all"):
            help = "{}. Not affected by --include-all".format(help)

        yield "--include-{}".format(opt), help

    for opt, info in PRESETS.items():
        help = "When specified, includes default options for {}".format(info["help"])
        yield "--preset-{}".format(opt), help


def ns_get(ns, key, default=False):
    return getattr(ns, key.replace("-", "_"), default)


def ns_set(ns, key, value=True):
    k1 = key.replace("-", "_")
    k2 = "include_{}".format(k1)
    if hasattr(ns, k2):
        setattr(ns, k2, value)
    elif hasattr(ns, k1):
        setattr(ns, k1, value)
    else:
        raise AttributeError("no argument named '{}'".format(k1))


@public
def update_presets(ns):
    for preset, info in PRESETS.items():
        if ns_get(ns, "preset-{}".format(preset)):
            for opt in info["options"]:
                ns_set(ns, opt)

    if ns.include_all:
        for opt in OPTIONS:
            if OPTIONS[opt].get("not-in-all"):
                continue
            ns_set(ns, opt)
