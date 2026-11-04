# This script is called by test_xpickle as a subprocess to load and dump
# pickles in a different Python version.
import os
import pickle
import struct
import sys


# This allows the xpickle worker to import picklecommon.py, which it needs
# since some of the pickle objects hold references to picklecommon.py.
test_mod_path = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                             'picklecommon.py'))
if sys.version_info >= (3, 5):
    import importlib.util
    spec = importlib.util.spec_from_file_location('test.picklecommon', test_mod_path)
    sys.modules['test'] = type(sys)('test')
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

def read_exact(f, n):
    buf = b''
    while len(buf) < n:
        chunk = f.read(n - len(buf))
        if not chunk:
            raise EOFError
        buf += chunk
    return buf

in_stream = getattr(sys.stdin, 'buffer', sys.stdin)
out_stream = getattr(sys.stdout, 'buffer', sys.stdout)

try:
    while True:
        size, = struct.unpack('!i', read_exact(in_stream, 4))
        if not size:
            break
        data = read_exact(in_stream, size)
        protocol, = struct.unpack('!i', data[:4])
        obj = pickle.loads(data[4:])
        data = pickle.dumps(obj, protocol)
        out_stream.write(struct.pack('!i', len(data)) + data)
        out_stream.flush()
except Exception as exc:
    # dump the exception to stdout and write to stderr, then exit
    try:
        data = pickle.dumps(exc)
        out_stream.write(struct.pack('!i', -len(data)) + data)
        out_stream.flush()
    except Exception:
        out_stream.write(struct.pack('!i', 0))
        out_stream.flush()
        sys.stderr.write(repr(exc))
        sys.stderr.flush()
    sys.exit(1)
