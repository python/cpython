import unittest
from concurrent.futures import ThreadPoolExecutor


NUM_THREADS = 100
NUM_LOOPS = 10

def looping_print_to_file(f, i):
    for j in range(NUM_LOOPS):
        # p = 'x' * 90 + '123456789'
        p = f"{i:2}i,{j}j\n" + '0123456789' * 10
        print(p, file=f)

class IoThreadingTestCase(unittest.TestCase):
    def test_io_threading(self):
        with ThreadPoolExecutor(max_workers=NUM_THREADS) as executor:
            with open('test_io_threading.out', 'w') as f:
                for i in range(NUM_THREADS):
                    executor.submit(looping_print_to_file, f, i)

                executor.shutdown(wait=True)

        with open('test_io_threading.out', 'r') as f:
            lines = set(x.rstrip() for x in f.readlines() if ',' in x)

        assert len(lines) == NUM_THREADS * NUM_LOOPS, repr(len(lines))

        for i in range(NUM_THREADS):
            for j in range(NUM_LOOPS):
                p = f"{i:2}i,{j}j"

                assert p in lines, repr(p)


if __name__ == "__main__":
    unittest.main()
