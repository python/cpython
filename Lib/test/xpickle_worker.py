# This script is called by test_xpickle as a subprocess to load and dump
# pickles in a different Python version.
import importlib.util
import os
import pickle
import sys


# This allows the xpickle worker to import picklecommon.py, which it needs
# since some of the pickle objects hold references to picklecommon.py.
test_mod_path = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                             'picklecommon.py'))
spec = importlib.util.spec_from_file_location('test.picklecommon', test_mod_path)
test_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(test_module)
sys.modules['test.picklecommon'] = test_module


in_stream = sys.stdin.buffer
out_stream = sys.stdout.buffer

try:
    message = pickle.load(in_stream)
    protocol, obj = message
    pickle.dump(obj, out_stream, protocol)
except Exception as e:
    # dump the exception to stdout and write to stderr, then exit
    pickle.dump(e, out_stream)
    sys.stderr.write(repr(e))
    sys.exit(1)
