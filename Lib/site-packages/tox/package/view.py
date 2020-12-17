import os
from itertools import chain

import six

from tox.reporter import verbosity1


def create_session_view(package, temp_dir):
    """once we build a package we cannot return that directly, as a subsequent call
    might delete that package (in order to do its own build); therefore we need to
    return a view of the file that it's not prone to deletion and can be removed when the
    session ends
    """
    if not package:
        return package
    package_dir = temp_dir.join("package")
    package_dir.ensure(dir=True)

    # we'll number the active instances, and use the max value as session folder for a new build
    # note we cannot change package names as PEP-491 (wheel binary format)
    # is strict about file name structure
    exists = [i.basename for i in package_dir.listdir()]
    file_id = max(chain((0,), (int(i) for i in exists if six.text_type(i).isnumeric())))

    session_dir = package_dir.join(str(file_id + 1))
    session_dir.ensure(dir=True)
    session_package = session_dir.join(package.basename)

    # if we can do hard links do that, otherwise just copy
    links = False
    if hasattr(os, "link"):
        try:
            os.link(str(package), str(session_package))
            links = True
        except (OSError, NotImplementedError):
            pass
    if not links:
        package.copy(session_package)
    operation = "links" if links else "copied"
    common = session_package.common(package)
    verbosity1(
        "package {} {} to {} ({})".format(
            common.bestrelpath(session_package),
            operation,
            common.bestrelpath(package),
            common,
        ),
    )
    return session_package
