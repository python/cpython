# This test covers backwards compatibility with previous versions of Python
# by bouncing pickled objects through Python versions by running xpickle_worker.py.
import io
import os
import pickle
import struct
import subprocess
import sys
import unittest


from test import support
from test import pickletester

try:
    import _pickle
    has_c_implementation = True
except ModuleNotFoundError:
    has_c_implementation = False

support.requires('xpickle')

is_windows = sys.platform.startswith('win')

# Map python version to a tuple containing the name of a corresponding valid
# Python binary to execute and its arguments.
py_executable_map = {}

protocols_map = {
    3: (3, 0),
    4: (3, 4),
    5: (3, 8),
}

def highest_proto_for_py_version(py_version):
    """Finds the highest supported pickle protocol for a given Python version.
    Args:
        py_version: a 2-tuple of the major, minor version. Eg. Python 3.7 would
                    be (3, 7)
    Returns:
        int for the highest supported pickle protocol
    """
    proto = 2
    for p, v in protocols_map.items():
        if py_version < v:
            break
        proto = p
    return proto

def have_python_version(py_version):
    """Check whether a Python binary exists for the given py_version and has
    support. This respects your PATH.
    For Windows, it will first try to use the py launcher specified in PEP 397.
    Otherwise (and for all other platforms), it will attempt to check for
    python<py_version[0]>.<py_version[1]>.

    Eg. given a *py_version* of (3, 7), the function will attempt to try
    'py -3.7' (for Windows) first, then 'python3.7', and return
    ['py', '-3.7'] (on Windows) or ['python3.7'] on other platforms.

    Args:
        py_version: a 2-tuple of the major, minor version. Eg. python 3.7 would
                    be (3, 7)
    Returns:
        List/Tuple containing the Python binary name and its required arguments,
        or None if no valid binary names found.
    """
    python_str = ".".join(map(str, py_version))
    targets = [('py', f'-{python_str}'), (f'python{python_str}',)]
    if py_version not in py_executable_map:
        for target in targets[0 if is_windows else 1:]:
            try:
                worker = subprocess.Popen([*target, '-c', 'pass'],
                                          stdout=subprocess.DEVNULL,
                                          stderr=subprocess.DEVNULL,
                                          shell=is_windows)
                worker.communicate()
                if worker.returncode == 0:
                    py_executable_map[py_version] = target
                break
            except FileNotFoundError:
                pass

    return py_executable_map.get(py_version, None)


def read_exact(f, n):
    buf = b''
    while len(buf) < n:
        chunk = f.read(n - len(buf))
        if not chunk:
            raise EOFError
        buf += chunk
    return buf


class AbstractCompatTests(pickletester.AbstractPickleTests):
    py_version = None
    worker = None

    @classmethod
    def setUpClass(cls):
        assert cls.py_version is not None, 'Needs a python version tuple'
        if not have_python_version(cls.py_version):
            py_version_str = ".".join(map(str, cls.py_version))
            raise unittest.SkipTest(f'Python {py_version_str} not available')
        cls.addClassCleanup(cls.finish_worker)
        # Override the default pickle protocol to match what xpickle worker
        # will be running.
        highest_protocol = highest_proto_for_py_version(cls.py_version)
        cls.enterClassContext(support.swap_attr(pickletester, 'protocols',
                                                range(highest_protocol + 1)))
        cls.enterClassContext(support.swap_attr(pickle, 'HIGHEST_PROTOCOL',
                                                highest_protocol))

    @classmethod
    def start_worker(cls, python):
        target = os.path.join(os.path.dirname(__file__), 'xpickle_worker.py')
        worker = subprocess.Popen([*python, target],
                                  stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  # For windows bpo-17023.
                                  shell=is_windows)
        cls.worker = worker
        return worker

    @classmethod
    def finish_worker(cls):
        worker = cls.worker
        if worker is None:
            return
        cls.worker = None
        worker.stdin.close()
        worker.stdout.close()
        worker.stderr.close()
        worker.terminate()
        worker.wait()

    @classmethod
    def send_to_worker(cls, python, data):
        """Bounce a pickled object through another version of Python.
        This will send data to a child process where it will
        be unpickled, then repickled and sent back to the parent process.
        Args:
            python: list containing the python binary to start and its arguments
            data: bytes object to send to the child process
        Returns:
            The pickled data received from the child process.
        """
        worker = cls.worker
        if worker is None:
            worker = cls.start_worker(python)

        try:
            worker.stdin.write(struct.pack('!i', len(data)) + data)
            worker.stdin.flush()

            size, = struct.unpack('!i', read_exact(worker.stdout, 4))
            if size > 0:
                return read_exact(worker.stdout, size)
            # if the worker fails, it will write the exception to stdout
            if size < 0:
                stdout = read_exact(worker.stdout, -size)
                try:
                    exception = pickle.loads(stdout)
                except (pickle.UnpicklingError, EOFError):
                    pass
                else:
                    if isinstance(exception, Exception):
                        # To allow for tests which test for errors.
                        raise exception
            _, stderr = worker.communicate()
            raise RuntimeError(stderr)
        except:
            cls.finish_worker()
            raise

    def dumps(self, arg, proto=0, **kwargs):
        # Skip tests that require buffer_callback arguments since
        # there isn't a reliable way to marshal/pickle the callback and ensure
        # it works in a different Python version.
        if 'buffer_callback' in kwargs:
            self.skipTest('Test does not support "buffer_callback" argument.')
        f = io.BytesIO()
        p = self.pickler(f, proto, **kwargs)
        p.dump(arg)
        data = struct.pack('!i', proto) + f.getvalue()
        python = py_executable_map[self.py_version]
        return self.send_to_worker(python, data)

    def loads(self, buf, **kwds):
        f = io.BytesIO(buf)
        u = self.unpickler(f, **kwds)
        return u.load()

    # A scaled-down version of test_bytes from pickletester, to reduce
    # the number of calls to self.dumps() and hence reduce the number of
    # child python processes forked. This allows the test to complete
    # much faster (the one from pickletester takes 3-4 minutes when running
    # under text_xpickle).
    def test_bytes(self):
        if self.py_version < (3, 0):
            self.skipTest('not supported in Python < 3.0')
        for proto in pickletester.protocols:
            for s in b'', b'xyz', b'xyz'*100:
                p = self.dumps(s, proto)
                self.assert_is_copy(s, self.loads(p))
            s = bytes(range(256))
            p = self.dumps(s, proto)
            self.assert_is_copy(s, self.loads(p))
            s = bytes([i for i in range(256) for _ in range(2)])
            p = self.dumps(s, proto)
            self.assert_is_copy(s, self.loads(p))

    # These tests are disabled because they require some special setup
    # on the worker that's hard to keep in sync.
    test_global_ext1 = None
    test_global_ext2 = None
    test_global_ext4 = None

    # These tests fail because they require classes from pickletester
    # which cannot be properly imported by the xpickle worker.
    test_recursive_nested_names = None
    test_recursive_nested_names2 = None

    # Attribute lookup problems are expected, disable the test
    test_dynamic_class = None
    test_evil_class_mutating_dict = None

    # Expected exception is raised during unpickling in a subprocess.
    test_pickle_setstate_None = None

    # Other Python version may not have NumPy.
    test_buffers_numpy = None

    # Skip tests that require buffer_callback arguments since
    # there isn't a reliable way to marshal/pickle the callback and ensure
    # it works in a different Python version.
    test_in_band_buffers = None
    test_buffers_error = None
    test_oob_buffers = None
    test_oob_buffers_writable_to_readonly = None

class PyPicklePythonCompat(AbstractCompatTests):
    pickler = pickle._Pickler
    unpickler = pickle._Unpickler

if has_c_implementation:
    class CPicklePythonCompat(AbstractCompatTests):
        pickler = _pickle.Pickler
        unpickler = _pickle.Unpickler


def make_test(py_version, base):
    class_dict = {'py_version': py_version}
    name = base.__name__.replace('Python', 'Python%d%d' % py_version)
    return type(name, (base, unittest.TestCase), class_dict)

def load_tests(loader, tests, pattern):
    def add_tests(py_version):
        test_class = make_test(py_version, PyPicklePythonCompat)
        tests.addTest(loader.loadTestsFromTestCase(test_class))
        if has_c_implementation:
            test_class = make_test(py_version, CPicklePythonCompat)
            tests.addTest(loader.loadTestsFromTestCase(test_class))

    value = support.get_resource_value('xpickle')
    if value is None:
        major = sys.version_info.major
        assert major == 3
        add_tests((2, 7))
        for minor in range(2, sys.version_info.minor):
            add_tests((major, minor))
    else:
        add_tests(tuple(map(int, value.split('.'))))
    return tests


if __name__ == '__main__':
    unittest.main()
