if __name__ == "__main__":
    import pathlib
    import runpy

    runpy.run_path(pathlib.Path(__file__).parent / "wasi", run_name="__main__")
