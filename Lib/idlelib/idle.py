import os
import sys

def add_idlelib_to_path():
    """
    Adds the parent directory of the current file's parent directory
    to sys.path to allow running IDLE from a non-standard location.
    """
    idlelib_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    if idlelib_dir not in sys.path:
        sys.path.insert(0, idlelib_dir)
    return idlelib_dir

def main():
    """
    Import and run IDLE's main shell.
    """
    add_idlelib_to_path()
    try:
        from idlelib.pyshell import main as idle_main
    except ImportError as e:
        print("Could not import idlelib.pyshell:", e)
        sys.exit(1)
    idle_main()

if __name__ == "__main__":
    main()
