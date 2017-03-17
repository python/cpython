Issue #27487: Warn if a submodule argument to "python -m" or
runpy.run_module() is found in sys.modules after parent packages are
imported, but before the submodule is executed.