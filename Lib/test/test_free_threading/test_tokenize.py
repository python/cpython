import io
import time
import unittest
import tokenize
from functools import partial
from threading import Thread

from test.support import threading_helper


@threading_helper.requires_working_threading()
class TestTokenize(unittest.TestCase):
    def test_tokenizer_iter(self):
        source = io.StringIO("for _ in a:\n  pass")
        it = tokenize._tokenize.TokenizerIter(source.readline, extra_tokens=False)

        tokens = []
        def next_token(it):
            while True:
                try:
                    r = next(it)
                    tokens.append(tokenize.TokenInfo._make(r))
                    time.sleep(0.03)
                except StopIteration:
                    return

        threads = []
        for _ in range(5):
            threads.append(Thread(target=partial(next_token, it)))

        for thread in threads:
            thread.start()

        for thread in threads:
            thread.join()

        expected_tokens = [
            tokenize.TokenInfo(type=1, string='for', start=(1, 0), end=(1, 3), line='for _ in a:\n'),
            tokenize.TokenInfo(type=1, string='_', start=(1, 4), end=(1, 5), line='for _ in a:\n'),
            tokenize.TokenInfo(type=1, string='in', start=(1, 6), end=(1, 8), line='for _ in a:\n'),
            tokenize.TokenInfo(type=1, string='a', start=(1, 9), end=(1, 10), line='for _ in a:\n'),
            tokenize.TokenInfo(type=11, string=':', start=(1, 10), end=(1, 11), line='for _ in a:\n'),
            tokenize.TokenInfo(type=4, string='', start=(1, 11), end=(1, 11), line='for _ in a:\n'),
            tokenize.TokenInfo(type=5, string='', start=(2, -1), end=(2, -1), line='  pass'),
            tokenize.TokenInfo(type=1, string='pass', start=(2, 2), end=(2, 6), line='  pass'),
            tokenize.TokenInfo(type=4, string='', start=(2, 6), end=(2, 6), line='  pass'),
            tokenize.TokenInfo(type=6, string='', start=(2, -1), end=(2, -1), line='  pass'),
            tokenize.TokenInfo(type=0, string='', start=(2, -1), end=(2, -1), line='  pass'),
        ]

        tokens.sort()
        expected_tokens.sort()
        self.assertListEqual(tokens, expected_tokens)


if __name__ == "__main__":
    unittest.main()
