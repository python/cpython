import multiprocessing
import sys

print(f"{__name__}")

# TODO: This is necessary as the fork server doesn't flush output after
#       preloading modules, and hence buffered output is inherited by child
#       processes. See gh-135335 (this should be removed once that is fixed).
sys.stdout.flush()

def f():
    print("f")

if __name__ == "__main__":
    ctx = multiprocessing.get_context("forkserver")
    ctx.set_forkserver_preload(['__main__'])
    for _ in range(2):
        p = ctx.Process(target=f)
        p.start()
        p.join()
