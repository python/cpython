import codecs
import unittest
from functools import partial
from threading import Event

from test.support import threading_helper
from test.support.threading_helper import run_concurrently

threading_helper.requires_working_threading(module=True)

CODEC_NAME = "test_free_threading_codec"


def identity(input, errors="strict"):
    return input, len(input)


def search_function(name):
    if name != CODEC_NAME:
        return None
    # New callables on each lookup, so that the cached CodecInfo holds the
    # only references to them.
    return codecs.CodecInfo(partial(identity), partial(identity), name=name)


class TestCodecs(unittest.TestCase):
    def test_racing_encode_and_unregister(self):
        # gh-159399: codecs.encode() must keep the encoder alive when another
        # thread unregisters the search function, which clears the cache.
        done = Event()

        def writer():
            try:
                for _ in range(50000):
                    codecs.register(search_function)
                    # The thread that runs the search function owns the
                    # cached CodecInfo, so clearing the cache from here can
                    # leave a reader holding the last reference to it.
                    codecs.lookup(CODEC_NAME)
                    codecs.unregister(search_function)
            finally:
                done.set()

        def reader():
            while not done.is_set():
                try:
                    codecs.encode("x", CODEC_NAME)
                except LookupError:
                    pass

        self.addCleanup(codecs.unregister, search_function)
        run_concurrently([writer] + [reader] * 4)


if __name__ == "__main__":
    unittest.main()
