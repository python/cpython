import functools
import pathlib
import tomllib


CHECKOUT = HERE = pathlib.Path(__file__).parent

while CHECKOUT != CHECKOUT.parent:
    if (CHECKOUT / "configure").is_file():
        break
    CHECKOUT = CHECKOUT.parent
else:
    raise FileNotFoundError(
        "Unable to find the root of the CPython checkout by looking for 'configure'"
    )

CROSS_BUILD_DIR = CHECKOUT / "cross-build"


_NO_CACHE = object()

def forced_cache(func):
    """Cache the result of a function no matter what.

    Useful for functions that will only ever be called with the same arguments.
    """
    cache = _NO_CACHE

    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        nonlocal cache
        if cache is _NO_CACHE:
            cache = func(*args, **kwargs)
        return cache

    return wrapper


@forced_cache
def host_triple(context):
    """Determine the target triple for the WASI host build."""
    if getattr(context, "host_triple", None):
        return context.host_triple

    with (HERE / "config.toml").open("rb") as file:
        config = tomllib.load(file)

    # Cache the result.
    context.host_triple = config["targets"]["host-triple"]
    return context.host_triple


def wasi_build_path(context):
    """Determine the path to the WASI build directory."""
    return CROSS_BUILD_DIR / host_triple(context)
