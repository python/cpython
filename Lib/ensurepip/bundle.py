"""Download Pip and setuptools dists for bundling."""

import sys

from ._bundler import ensure_wheels_are_downloaded


if __name__ == '__main__':
    ensure_wheels_are_downloaded(verbosity='-v' in sys.argv)
