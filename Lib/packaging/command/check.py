"""Check PEP compliance of metadata."""

from packaging import logger
from packaging.command.cmd import Command
from packaging.errors import PackagingSetupError
from packaging.util import resolve_name

class check(Command):

    description = "check PEP compliance of metadata"

    user_options = [('metadata', 'm', 'Verify metadata'),
                    ('all', 'a',
                     ('runs extended set of checks')),
                    ('strict', 's',
                     'Will exit with an error if a check fails')]

    boolean_options = ['metadata', 'all', 'strict']

    def initialize_options(self):
        """Sets default values for options."""
        self.all = False
        self.metadata = True
        self.strict = False
        self._warnings = []

    def finalize_options(self):
        pass

    def warn(self, msg, *args):
        """Wrapper around logging that also remembers messages."""
        # XXX we could use a special handler for this, but would need to test
        # if it works even if the logger has a too high level
        self._warnings.append((msg, args))
        return logger.warning(self.get_command_name() + msg, *args)

    def run(self):
        """Runs the command."""
        # perform the various tests
        if self.metadata:
            self.check_metadata()
        if self.all:
            self.check_restructuredtext()
            self.check_hooks_resolvable()

        # let's raise an error in strict mode, if we have at least
        # one warning
        if self.strict and len(self._warnings) > 0:
            msg = '\n'.join(msg % args for msg, args in self._warnings)
            raise PackagingSetupError(msg)

    def check_metadata(self):
        """Ensures that all required elements of metadata are supplied.

        name, version, URL, author

        Warns if any are missing.
        """
        missing, warnings = self.distribution.metadata.check(strict=True)
        if missing != []:
            self.warn('missing required metadata: %s', ', '.join(missing))
        for warning in warnings:
            self.warn(warning)

    def check_restructuredtext(self):
        """Checks if the long string fields are reST-compliant."""
        missing, warnings = self.distribution.metadata.check(restructuredtext=True)
        if self.distribution.metadata.docutils_support:
            for warning in warnings:
                line = warning[-1].get('line')
                if line is None:
                    warning = warning[1]
                else:
                    warning = '%s (line %s)' % (warning[1], line)
                self.warn(warning)
        elif self.strict:
            raise PackagingSetupError('The docutils package is needed.')

    def check_hooks_resolvable(self):
        for options in self.distribution.command_options.values():
            for hook_kind in ("pre_hook", "post_hook"):
                if hook_kind not in options:
                    break
                for hook_name in options[hook_kind][1].values():
                    try:
                        resolve_name(hook_name)
                    except ImportError:
                        self.warn('name %r cannot be resolved', hook_name)
