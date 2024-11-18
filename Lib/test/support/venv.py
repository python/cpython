import contextlib
import logging
import os
import subprocess
import shlex
import sys
import sysconfig
import tempfile
import venv


class VirtualEnvironment:
    def __init__(self, prefix, **venv_create_args):
        self._logger = logging.getLogger(self.__class__.__name__)
        venv.create(prefix, **venv_create_args)
        self._prefix = prefix
        self._paths = sysconfig.get_paths(
            scheme='venv',
            vars={'base': self.prefix},
            expand=True,
        )

    @classmethod
    @contextlib.contextmanager
    def from_tmpdir(cls, *, prefix=None, dir=None, **venv_create_args):
        delete = not bool(os.environ.get('PYTHON_TESTS_KEEP_VENV'))
        with tempfile.TemporaryDirectory(prefix=prefix, dir=dir, delete=delete) as tmpdir:
            yield cls(tmpdir, **venv_create_args)

    @property
    def prefix(self):
        return self._prefix

    @property
    def paths(self):
        return self._paths

    @property
    def interpreter(self):
        return os.path.join(self.paths['scripts'], os.path.basename(sys.executable))

    def _format_output(self, name, data, indent='\t'):
        if not data:
            return indent + f'{name}: (none)'
        if len(data.splitlines()) == 1:
            return indent + f'{name}: {data}'
        else:
            prefixed_lines = '\n'.join(indent + '> ' + line for line in data.splitlines())
            return indent + f'{name}:\n' + prefixed_lines

    def run(self, *args, **subprocess_args):
        if subprocess_args.get('shell'):
            raise ValueError('Running the subprocess in shell mode is not supported.')
        default_args = {
            'capture_output': True,
            'check': True,
        }
        try:
            result = subprocess.run([self.interpreter, *args], **default_args | subprocess_args)
        except subprocess.CalledProcessError as e:
            if e.returncode != 0:
                self._logger.error(
                    f'Interpreter returned non-zero exit status {e.returncode}.\n'
                    + self._format_output('COMMAND', shlex.join(e.cmd)) + '\n'
                    + self._format_output('STDOUT', e.stdout.decode()) + '\n'
                    + self._format_output('STDERR', e.stderr.decode()) + '\n'
                )
            raise
        else:
            return result
