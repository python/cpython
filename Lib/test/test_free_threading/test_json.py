from threading import Barrier, Thread
from test.test_json import CTest
from test.support import threading_helper


def encode_json_helper(
    json, worker, data, number_of_threads=12, number_of_json_encodings=100
):
    worker_threads = []
    barrier = Barrier(number_of_threads)
    for index in range(number_of_threads):
        worker_threads.append(
            Thread(target=worker, args=[barrier, data, index])
        )
    for t in worker_threads:
        t.start()
    for ii in range(number_of_json_encodings):
        json.dumps(data)
    data.clear()
    for t in worker_threads:
        t.join()


class MyMapping(dict):
    def __init__(self):
        self.mapping = []

    def items(self):
        return self.mapping


@threading_helper.reap_threads
@threading_helper.requires_working_threading()
class TestJsonEncoding(CTest):
    # Test encoding json with concurrent threads modifying the data cannot
    # corrupt the interpreter

    def test_json_mutating_list(self):
        def worker(barrier, data, index):
            barrier.wait()
            while data:
                for d in data:
                    if len(d) > 5:
                        d.clear()
                    else:
                        d.append(index)

        data = [[], []]
        encode_json_helper(self.json, worker, data)

    def test_json_mutating_exact_dict(self):
        def worker(barrier, data, index):
            barrier.wait()
            while data:
                for d in data:
                    if len(d) > 5:
                        try:
                            key = list(d)[0]
                            d.pop(key)
                        except (KeyError, IndexError):
                            pass
                    else:
                        d[index] = index

        data = [{}, {}]
        encode_json_helper(self.json, worker, data)

    def test_json_mutating_mapping(self):
        def worker(barrier, data, index):
            barrier.wait()
            while data:
                for d in data:
                    if len(d.mapping) > 3:
                        d.mapping.clear()
                    else:
                        d.mapping.append((index, index))

        data = [MyMapping(), MyMapping()]
        encode_json_helper(self.json, worker, data)
