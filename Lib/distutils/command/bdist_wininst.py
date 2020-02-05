"""distutils.command.bdist_wininst stub

Suppress the 'bdist_wininst' command, while still allowing
setuptools to import it without breaking."""

from distutils.core import Command
from distutils.errors import DistutilsPlatformError


class bdist_wininst(Command):
    description = "create an executable installer for MS Windows"

    # Marker for tests that we have the unsupported bdist_wininst
    _unsupported = True

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        raise DistutilsPlatformError(
            "bdist_wininst has been removed in Python 3.9"
        )
