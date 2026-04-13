import hashlib
import os
import sys

def main():
    filenames, hashes, sizes = [], [], []

    for file in sys.argv[1:]:
        if not os.path.isfile(file):
            continue

        with open(file, 'rb') as f:
            data = f.read()
            md5 = hashlib.md5()
            md5.update(data)
            filenames.append(os.path.split(file)[1])
            hashes.append(md5.hexdigest())
            sizes.append(str(len(data)))

    print('{:40s}  {:<32s}  {:<9s}'.format('File', 'MD5', 'Size'))
    for f, h, s in zip(filenames, hashes, sizes):
        print('{:40s}  {:>32s}  {:>9s}'.format(f, h, s))



if __name__ == "__main__":
    sys.exit(int(main() or 0))
