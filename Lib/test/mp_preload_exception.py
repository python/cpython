import multiprocessing
import sys

#
# This test verifies that preload on a nonexistant module raises an exception
# that eventually leads to any new process start failing, when we specify that
# as the desired behavior.
#

def f():
    print('f')

if __name__ == "__main__":
    raise_exceptions = int(sys.argv[1])!=0
    ctx = multiprocessing.get_context("forkserver")
    ctx.set_forkserver_preload(["__main__","test.mp_preload_import_does_not_exist"], raise_exceptions)
    proc = ctx.Process(target=f)
    exception_thrown = False
    try:
        proc.start()
        proc.join()
    except Exception:
        exception_thrown=True
    if exception_thrown != raise_exceptions:
        raise RuntimeError('Difference between exception_thrown and raise_exceptions')
    print('done')
