import multiprocessing

def foo(conn):
    conn.send("123")

# Because "if __name__ == '__main__'" is missing this will not work
# correctly on Windows.  However, we should get a RuntimeError rather
# than the Windows equivalent of a fork bomb.

r, w = multiprocessing.Pipe(False)
p = multiprocessing.Process(target=foo, args=(w,))
p.start()
w.close()
print(r.recv())
r.close()
p.join()
