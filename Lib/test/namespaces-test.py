import os
import sys


def main():
    fd = os.open('/proc/self/ns/uts', os.O_RDONLY)
    try:
        print(os.readlink('/proc/self/ns/uts'))
        os.unshare(os.CLONE_NEWUTS)
        print(os.readlink('/proc/self/ns/uts'))
        os.setns(fd, os.CLONE_NEWUTS)
        print(os.readlink('/proc/self/ns/uts'))
    except OSError as e:
        sys.stderr.write(str(e.errno))
        sys.exit(2)
    finally:
        os.close(fd)


if __name__ == '__main__':
    main()
