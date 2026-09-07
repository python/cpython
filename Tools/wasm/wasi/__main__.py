if __name__ == "__main__":
    import pathlib
    import runpy
    import sys

    print(
        "⚠️ WARNING: This script is deprecated and slated for removal in Python 3.20; "
        "execute the `Platforms/WASI/` directory instead (i.e. `python Platforms/WASI`)\n",
        file=sys.stderr,
    )

    checkout = pathlib.Path(__file__).parent.parent.parent.parent

    runpy.run_path(checkout / "Platforms" / "WASI", run_name="__main__")
