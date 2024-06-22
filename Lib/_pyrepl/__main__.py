# Important: put as few things as possible in the global namespace of this module,
# as it's easy for things in this module's `__globals__` to accidentally end up
# in the globals of the REPL

from ._main import interactive_console

if __name__ == "__main__":
    interactive_console()
