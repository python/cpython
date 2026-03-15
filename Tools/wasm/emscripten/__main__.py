if __name__ == "__main__":
    import pathlib
    import runpy

    checkout = pathlib.Path(__file__).parents[3]
    emscripten_dir = (checkout / "Platforms/emscripten").absolute()
    runpy.run_path(str(emscripten_dir), run_name="__main__")
