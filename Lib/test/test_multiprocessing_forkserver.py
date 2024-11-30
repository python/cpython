import unittest
import multiprocessing
import os
import signal
import time

class TestForkserver(unittest.TestCase):

    def setUp(self):
        self.manager = multiprocessing.Manager()
        self.shared_dict = self.manager.dict()

    def tearDown(self):
        self.manager.shutdown()

    def test_shared_memory_access(self):
        def worker(shared_dict):
            shared_dict['key'] = 'value'

        process = multiprocessing.Process(target=worker, args=(self.shared_dict,))
        process.start()
        process.join()

        self.assertEqual(self.shared_dict['key'], 'value')

    def test_forkserver_process_return_code(self):
        def worker():
            os._exit(1)

        process = multiprocessing.Process(target=worker)
        process.start()
        process.join()

        self.assertEqual(process.exitcode, 1)

    def test_forkserver_signal_handling(self):
        def worker():
            time.sleep(5)

        process = multiprocessing.Process(target=worker)
        process.start()
        os.kill(process.pid, signal.SIGTERM)
        process.join()

        self.assertEqual(process.exitcode, -signal.SIGTERM)

if __name__ == '__main__':
    unittest.main()
