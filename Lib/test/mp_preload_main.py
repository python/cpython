import multiprocessing

print(f"{__name__}")

def f():
    print("f")

if __name__ == "__main__":
    ctx = multiprocessing.get_context("forkserver")
    ctx.set_forkserver_preload(['__main__'])
    for _ in range(2):
        p = ctx.Process(target=f)
        p.start()
        p.join()
