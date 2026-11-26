import sys

from . import Distribution, MetadataNotFound


def describe(dist: Distribution) -> str:
    """
    Describe the distribution by name, or by location when the
    distribution is too invalid to name itself.

    >>> describe(Distribution.at('missing-1.0.dist-info'))
    'missing-1.0.dist-info (no metadata)'
    """
    try:
        return dist.name
    except MetadataNotFound:
        return f'{dist} (no metadata)'
    except KeyError:
        return f'{dist} (metadata without a name)'


def inspect(path):
    print("Inspecting", path)
    dists = list(Distribution.discover(path=[path]))
    if not dists:
        return
    print("Found", len(dists), "packages:", end=' ')
    print(', '.join(map(describe, dists)))


def run():
    for path in sys.path:
        inspect(path)


if __name__ == '__main__':
    run()
