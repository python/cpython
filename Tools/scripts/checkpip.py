#!/usr/bin/env python3
"""
Checks that the version of the projects bundled in ensurepip are the latest
versions available.
"""
import ensurepip
import json
import urllib.request
import sys


def main():
    outofdate = False

    for project, version in ensurepip._PROJECTS:
        data = json.loads(urllib.request.urlopen(
            "https://pypi.org/pypi/{}/json".format(project),
            cadefault=True,
        ).read().decode("utf8"))
        upstream_version = data["info"]["version"]

        if version != upstream_version:
            outofdate = True
            print("The latest version of {} on PyPI is {}, but ensurepip "
                  "has {}".format(project, upstream_version, version))

    if outofdate:
        sys.exit(1)


if __name__ == "__main__":
    main()
