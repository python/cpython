if __name__ == "__main__":
    import pathlib
    import runpy
    import sys

    print(
        "⚠️ WARNING: This script is deprecated and slated for removal in Python 3.20; "
        "execute the `Platforms/emscripten/` directory instead (i.e. `python Platforms/emscripten`)\n",
        file=sys.stderr,
    )

    checkout = pathlib.Path(__file__).parents[3]
    emscripten_dir = (checkout / "Platforms/emscripten").absolute()
    runpy.run_path(str(emscripten_dir), run_name="__main__")
