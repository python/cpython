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
    "distutils": {"help": "distutils"},
    "tcltk": {"help": "Tcl, Tk and tkinter"},
    "idle": {"help": "Idle"},
    "tests": {"help": "test suite"},
    "tools": {"help": "tools"},
    "venv": {"help": "venv"},
    "dev": {"help": "headers and libs"},
    "symbols": {"help": "symbols"},
    "bdist-wininst": {"help": "bdist_wininst support"},
    "underpth": {"help": "a python._pth file", "not-in-all": True},
    "launchers": {"help": "specific launchers"},
    "appxmanifest": {"help": "an appxmanifest"},
    "props": {"help": "a python.props file"},
    "nuspec": {"help": "a python.nuspec file"},
    "chm": {"help": "the CHM documentation"},
    "html-doc": {"help": "the HTML documentation"},
}


PRESETS = {
    "appx": {
        "help": "APPX package",
        "options": [
            "stable",
            "pip",
            "pip-user",
            "distutils",
            "tcltk",
            "idle",
            "venv",
            "dev",
            "launchers",
            "appxmanifest",
            # XXX: Disabled for now "precompile",
        ],
    },
    "nuget": {
        "help": "nuget package",
        "options": [
            "dev",
            "tools",
            "pip",
            "stable",
            "distutils",
            "venv",
            "props",
            "nuspec",
        ],
    },
    "iot": {"help": "Windows IoT Core", "options": ["stable", "pip"]},
    "default": {
        "help": "development kit package",
        "options": [
            "stable",
            "pip",
            "distutils",
            "tcltk",
            "idle",
            "tests",
            "tools",
            "venv",
            "dev",
            "symbols",
            "bdist-wininst",
            "chm",
        ],
    },
    "embed": {
        "help": "embeddable package",
        "options": ["stable", "zip-lib", "flat-dlls", "underpth", "precompile"],
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
