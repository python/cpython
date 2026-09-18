import os

if __name__ == '__main__':
    buf = os.read(0, 1024)
    os.write(1, b'OUT:'+buf)
    os.write(2, b'ERR:'+buf)
