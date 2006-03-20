"""A helper to download input files needed by assorted encoding tests.

fetch_data_files.py [directory]

Files are downloaded to directory `directory`.  If a directory isn't given,
it defaults to the current directory (.).
"""

DATA_URLS = """
    http://people.freebsd.org/~perky/i18n/BIG5HKSCS.TXT
    http://people.freebsd.org/~perky/i18n/EUC-CN.TXT
    http://people.freebsd.org/~perky/i18n/EUC-JISX0213.TXT
    http://people.freebsd.org/~perky/i18n/EUC-JP.TXT
    http://people.freebsd.org/~perky/i18n/EUC-KR.TXT
    http://people.freebsd.org/~perky/i18n/SHIFT_JISX0213.TXT

    http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP932.TXT
    http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP936.TXT
    http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP949.TXT
    http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP950.TXT

    http://www.unicode.org/Public/3.2-Update/NormalizationTest-3.2.0.txt

    http://www.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/SHIFTJIS.TXT
    http://www.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/KSC/JOHAB.TXT
    http://www.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/OTHER/BIG5.TXT
"""

# Adapted from test_support.open_urlresource() in Python 2.5.
# Fetch the file give by `url` off the web, and store it in directory
# `directory`.  The file name is extracted from the last URL component.
# If the file already exists, it's not fetched again.
def fetch_file_from_url(url, directory):
    import urllib, urlparse
    import os.path

    filename = urlparse.urlparse(url)[2].split('/')[-1] # '/': it's a URL!
    target = os.path.join(directory, filename)
    if os.path.exists(target):
        print "\tskipping %r -- already exists" % target
    else:
        print "\tfetching %s ..." % url
        urllib.urlretrieve(url, target)

def main(urls, directory):
    print "Downloading data files to %r" % directory
    for url in urls.split():
        fetch_file_from_url(url, directory)

if __name__ == "__main__":
    import sys

    n = len(sys.argv)
    if n == 1:
        directory = "."
    elif n == 2:
        directory = sys.argv[1]
    else:
        raise ValueError("no more than one argument allowed")

    main(DATA_URLS, directory)
