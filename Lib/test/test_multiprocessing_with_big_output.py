import unittest
from multiprocessing.pool import Pool
from time import sleep


def func():
    return 'test' * (10 ** 9)


class TestMultiprocessingWithBigOutput(unittest.TestCase):
    @unittest.skip("skipping test for manual use only. Highly demanding on RAM and time consuming")
    def test_big_subprocess_output(self):
        with Pool() as pool:
            task = pool.apply_async(func)
            for i in range(10):
                if task.ready():
                    task.get()
                    break
                sleep(30)
            else:
                self.fail('test_big_subprocess_output() timeout.')


if __name__ == '__main__':
    unittest.main()
