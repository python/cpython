# Simple Python script to download a file. Used as a fallback
# when other more reliable methods fail.
#
from __future__ import print_function
import sys

try:
    from requests import get
except ImportError:
    try:
        from urllib.request import urlretrieve
        USING = "urllib.request.urlretrieve"
    except ImportError:
        try:
            from urllib import urlretrieve
            USING = "urllib.retrieve"
        except ImportError:
            print("Python at", sys.executable, "is not suitable",
                  "for downloading files.", file=sys.stderr)
            sys.exit(2)
else:
    USING = "requests.get"

    def urlretrieve(url, filename):
        r = get(url, stream=True)
        r.raise_for_status()
        with open(filename, 'wb') as f:
            for chunk in r.iter_content(chunk_size=1024):
                f.write(chunk)
        return filename

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: urlretrieve.py [url] [filename]", file=sys.stderr)
        sys.exit(1)
    URL = sys.argv[1]
    FILENAME = sys.argv[2]
    print("Downloading from", URL, "to", FILENAME, "using", USING)
    urlretrieve(URL, FILENAME)
