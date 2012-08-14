import multiprocessing, sys

def foo():
    print("123")

# Because "if __name__ == '__main__'" is missing this will not work
# correctly on Windows.  However, we should get a RuntimeError rather
# than the Windows equivalent of a fork bomb.

p = multiprocessing.Process(target=foo)
p.start()
p.join()
sys.exit(p.exitcode)
