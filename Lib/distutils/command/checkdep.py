"""distutils.command.x

Implements the Distutils 'x' command.
"""

# created 2000/mm/dd, John Doe

__revision__ = "$Id$"

from distutils.core import Command

class DependencyFailure(Exception): pass

class VersionTooOld(DependencyFailure): pass

class VersionNotKnown(DependencyFailure): pass

class checkdep (Command):

    # Brief (40-50 characters) description of the command
    description = "check package dependencies"

    # List of option tuples: long name, short name (None if no short
    # name), and help string.
    # Later on, we might have auto-fetch and the like here. Feel free.
    user_options = []

    def initialize_options (self):
        self.debug = None

    # initialize_options()


    def finalize_options (self):
        pass
    # finalize_options()


    def run (self):
        from distutils.version import LooseVersion
        failed = []
        for pkg, ver in self.distribution.metadata.requires:
            if pkg == 'python':
                if ver is not None:
                    # Special case the 'python' package
                    import sys
                    thisver = LooseVersion('%d.%d.%d'%sys.version_info[:3])
                    if thisver < ver:
                        failed.append(((pkg,ver), VersionTooOld(thisver)))
                continue
            # Kinda hacky - we should do more here
            try:
                mod = __import__(pkg)
            except Exception, e:
                failed.append(((pkg,ver), e))
                continue
            if ver is not None:
                if hasattr(mod, '__version__'):
                    thisver = LooseVersion(mod.__version__)
                    if thisver < ver:
                        failed.append(((pkg,ver), VersionTooOld(thisver)))
                else:
                    failed.append(((pkg,ver), VersionNotKnown()))

        if failed:
            raise DependencyFailure, failed

    # run()

# class x
