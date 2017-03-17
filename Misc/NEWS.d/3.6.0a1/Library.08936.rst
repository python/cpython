Issue #26367: importlib.__import__() raises ImportError like
builtins.__import__() when ``level`` is specified but without an accompanying
package specified.