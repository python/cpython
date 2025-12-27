import sys

def myimport(*args):
    return sys.modules[__name__]


new_globals = dict(globals())
new_globals["__builtins__"] = {
    "__import__": myimport,
}
basic2 = 42
basic = __lazy_import__("test.test_import.data.lazy_imports.basic2",
                        globals=new_globals)
basic
