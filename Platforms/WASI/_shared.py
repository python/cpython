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


def host_triple(context):
    """Determine the target triple for the WASI host build."""
    if context.host_triple:
        return context.host_triple

    with (HERE / "config.toml").open("rb") as file:
        config = tomllib.load(file)

    # Cache the result.
    context.host_triple = config["targets"]["host-triple"]
    return context.host_triple
