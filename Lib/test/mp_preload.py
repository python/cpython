import multiprocessing

multiprocessing.Lock()

#
# This test verifies that preload is behaving as expected. By preloading
# both __main__ and mp_preload_import, both this module and mp_preload_import
# should be loaded in the forkserver process when it serves new processes.
# This means that each new process and call to f() will not cause additional
# module loading.
#
# The expected output is then:
# mp_preload
# mp_preload
# mp_preload_import
# f
# f
# f
#
# Any deviation from this means something is broken.
#
def f():
    import test.mp_preload_import
    print('f')

print("mp_preload")
if __name__ == "__main__":
    ctx = multiprocessing.get_context("forkserver")
    ctx.set_forkserver_preload(["__main__","test.mp_preload_import"], True)
    for i in range(3):
        proc = ctx.Process(target=f)
        proc.start()
        proc.join()
