import os
import sys
import sysconfig


def reset_tzpath(to=None):
    global TZPATH

    tzpaths = to
    if tzpaths is not None:
        if isinstance(tzpaths, (str, bytes)):
            raise TypeError(
                f"tzpaths must be a list or tuple, "
                + f"not {type(tzpaths)}: {tzpaths!r}"
            )
        elif not all(map(os.path.isabs, tzpaths)):
            raise ValueError(_get_invalid_paths_message(tzpaths))
        base_tzpath = tzpaths
    else:
        env_var = os.environ.get("PYTHONTZPATH", None)
        if env_var is not None:
            base_tzpath = _parse_python_tzpath(env_var)
        else:
            base_tzpath = _parse_python_tzpath(
                sysconfig.get_config_var("TZPATH")
            )

    TZPATH = tuple(base_tzpath)


def _parse_python_tzpath(env_var):
    if not env_var:
        return ()

    raw_tzpath = env_var.split(os.pathsep)
    new_tzpath = tuple(filter(os.path.isabs, raw_tzpath))

    # If anything has been filtered out, we will warn about it
    if len(new_tzpath) != len(raw_tzpath):
        import warnings

        msg = _get_invalid_paths_message(raw_tzpath)

        warnings.warn(
            "Invalid paths specified in PYTHONTZPATH environment variable."
            + msg,
            InvalidTZPathWarning,
        )

    return new_tzpath


def _get_invalid_paths_message(tzpaths):
    invalid_paths = (path for path in tzpaths if not os.path.isabs(path))

    prefix = "\n    "
    indented_str = prefix + prefix.join(invalid_paths)

    return (
        "Paths should be absolute but found the following relative paths:"
        + indented_str
    )


def find_tzfile(key):
    """Retrieve the path to a TZif file from a key."""
    _validate_tzfile_path(key)
    for search_path in TZPATH:
        filepath = os.path.join(search_path, key)
        if os.path.isfile(filepath):
            return filepath

    return None


_TEST_PATH = os.path.normpath(os.path.join("_", "_"))[:-1]


def _validate_tzfile_path(path, _base=_TEST_PATH):
    if os.path.isabs(path):
        raise ValueError(
            f"ZoneInfo keys may not be absolute paths, got: {path}"
        )

    # We only care about the kinds of path normalizations that would change the
    # length of the key - e.g. a/../b -> a/b, or a/b/ -> a/b. On Windows,
    # normpath will also change from a/b to a\b, but that would still preserve
    # the length.
    new_path = os.path.normpath(path)
    if len(new_path) != len(path):
        raise ValueError(
            f"ZoneInfo keys must be normalized relative paths, got: {path}"
        )

    resolved = os.path.normpath(os.path.join(_base, new_path))
    if not resolved.startswith(_base):
        raise ValueError(
            f"ZoneInfo keys must refer to subdirectories of TZPATH, got: {path}"
        )


del _TEST_PATH


class InvalidTZPathWarning(RuntimeWarning):
    """Warning raised if an invalid path is specified in PYTHONTZPATH."""


TZPATH = ()
reset_tzpath()
