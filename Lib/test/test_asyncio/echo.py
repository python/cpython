import os

if __name__ == '__main__':
    while True:
        buf = os.read(0, 1024)
        os.write(1, buf)
