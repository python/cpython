# This script is called by test_xpickle as a subprocess to load and dump
# pickles in a different Python version.
import os
import pickle
import sys


# This allows the xpickle worker to import picklecommon.py, which it needs
# since some of the pickle objects hold references to picklecommon.py.
test_mod_path = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                             'picklecommon.py'))
if sys.version_info >= (3, 5):
    import importlib.util
    spec = importlib.util.spec_from_file_location('test.picklecommon', test_mod_path)
    test_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(test_module)
    sys.modules['test.picklecommon'] = test_module
else:
    test_module = type(sys)('test.picklecommon')
    sys.modules['test.picklecommon'] = test_module
    sys.modules['test'] = type(sys)('test')
    with open(test_mod_path, 'rb') as f:
        sources = f.read()
    exec(sources, vars(test_module))


in_stream = getattr(sys.stdin, 'buffer', sys.stdin)
out_stream = getattr(sys.stdout, 'buffer', sys.stdout)

try:
    message = pickle.load(in_stream)
    protocol, obj = message
    pickle.dump(obj, out_stream, protocol)
except Exception as e:
    # dump the exception to stdout and write to stderr, then exit
    pickle.dump(e, out_stream)
    sys.stderr.write(repr(e))
    sys.exit(1)
