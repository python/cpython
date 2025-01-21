import dis
import queue
import threading
import time
import unittest
from test.support import (import_helper, cpython_only, Py_GIL_DISABLED,
                          requires_specialization_ft)

_testinternalcapi = import_helper.import_module("_testinternalcapi")
_testlimitedcapi = import_helper.import_module("_testlimitedcapi")

NUMTHREADS = 5

def get_tlbc_instructions(f):
    co = dis._get_code_object(f)
    tlbc = _testinternalcapi.get_tlbc(co)
    return [i.opname for i in dis._get_instructions_bytes(tlbc)]


class IterationDeoptTests(unittest.TestCase):
    def check_deopt(self, get_iter, opcode, is_generator=False):
        input = range(100)
        expected_len = len(input)
        q = queue.Queue()
        barrier = threading.Barrier(NUMTHREADS + 1)
        done = threading.Event()
        def worker():
            # A complicated dance to get a weak reference to an iterator
            # _only_ (strongly) referenced by the for loop, so that we can
            # force our loop to deopt mid-way through.
            it = get_iter(input)
            ref = _testlimitedcapi.pylong_fromvoidptr(it)
            q.put(ref)
            # We can't use enumerate without affecting the loop, so keep a
            # manual counter.
            i = 0
            loop_a_little_more = 5
            results = []
            try:
                # Make sure we're not still specialized from a previous run.
                ops = get_tlbc_instructions(worker)
                self.assertIn('FOR_ITER', ops)
                self.assertNotIn(opcode, ops)
                for item in it:
                    results.append(item)
                    i += 1

                    # We have to be very careful exiting the loop, because
                    # if the main thread hasn't dereferenced the unsafe
                    # weakref to our iterator yet, exiting will make it
                    # invalid and cause a crash. Getting the timing right is
                    # difficult, though, since it depends on the OS
                    # scheduler and the system load. As a final safeguard,
                    # if we're close to finishing the loop, just wait for the
                    # main thread.
                    if i + loop_a_little_more > expected_len:
                        done.wait()

                    if i == 1:
                        del it
                    # Warm up. The first iteration didn't count because of
                    # the extra reference to the iterator.
                    if i < 10:
                        continue
                    if i == 10:
                        ops = get_tlbc_instructions(worker)
                        self.assertIn(opcode, ops)
                        # Let the main thread know it's time to reference our iterator.
                        barrier.wait()
                        continue
                    # Continue iterating while at any time our loop may be
                    # forced to deopt, but try to get the thread scheduler
                    # to give the main thread a chance to run.
                    if not done.is_set():
                        time.sleep(0)
                        continue
                    if loop_a_little_more:
                        # Loop a little more after 'done' is set to make sure we
                        # introduce a tsan-detectable race if the loop isn't
                        # deopting appropriately.
                        loop_a_little_more -= 1
                        continue
                    break
                self.assertEqual(results, list(input)[:i])
            except threading.BrokenBarrierError:
                return
            except Exception as e:
                # In case the exception happened before the last barrier,
                # reset it so nothing is left hanging.
                barrier.reset()
                # In case it's the final assertion that failed, just add it
                # to the result queue so it'll show up in the normal test
                # output.
                q.put(e)
                raise
            q.put("SUCCESS")
        # Reset specialization and thread-local bytecode from previous runs.
        worker.__code__ = worker.__code__.replace()
        threads = [threading.Thread(target=worker) for i in range(NUMTHREADS)]
        for t in threads:
            t.start()
        # Get the "weakrefs" from the worker threads.
        refs = [q.get() for i in range(NUMTHREADS)]
        # Wait for each thread to finish its specialization check.
        barrier.wait()
        # Dereference the "weakrefs" we were sent in an extremely unsafe way.
        iterators = [_testlimitedcapi.pylong_asvoidptr(ref) for ref in refs]
        done.set()
        self.assertNotIn(None, iterators)
        # Read data that the iteration writes, to trigger data races if they
        # don't deopt appropriately.
        if is_generator:
            for it in iterators:
                it.gi_running
        else:
            for it in iterators:
                it.__reduce__()
        for t in threads:
            t.join()
        results = [q.get() for i in range(NUMTHREADS)]
        self.assertEqual(results, ["SUCCESS"] * NUMTHREADS)

    @cpython_only
    @requires_specialization_ft
    @unittest.skipIf(not Py_GIL_DISABLED, "requires free-threading")
    def test_deopt_leaking_iterator_list(self):
        def make_list_iter(input):
            return iter(list(input))
        self.check_deopt(make_list_iter, 'FOR_ITER_LIST')

    @cpython_only
    @requires_specialization_ft
    @unittest.skipIf(not Py_GIL_DISABLED, "requires free-threading")
    def test_deopt_leaking_iterator_tuple(self):
        def make_tuple_iter(input):
            return iter(tuple(input))
        self.check_deopt(make_tuple_iter, 'FOR_ITER_TUPLE')

    @cpython_only
    @requires_specialization_ft
    @unittest.skipIf(not Py_GIL_DISABLED, "requires free-threading")
    def test_deopt_leaking_iterator_range(self):
        def make_range_iter(input):
            return iter(input)
        self.check_deopt(make_range_iter, 'FOR_ITER_RANGE')

    @cpython_only
    @requires_specialization_ft
    @unittest.skipIf(not Py_GIL_DISABLED, "requires free-threading")
    def test_deopt_leaking_iterator_generator(self):
        def gen(input):
            for item in input:
                yield item
        self.check_deopt(gen, 'FOR_ITER_GEN', is_generator=True)
