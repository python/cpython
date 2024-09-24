"""Example code for howto/concurrency.rst.

The examples take advantage of the literalinclude directive's
:start-after: and :end-before: options.
"""

from collections import namedtuple
import os.path
import re
import shlex
import subprocess
import sys


IMPLS_DIR = os.path.dirname(__file__)
#if IMPLS_DIR not in sys.path:
#    sys.path.insert(0, IMPLS_DIR)


class example(staticmethod):
    """A function containing example code.

    The function will be called when this file is run as a script.
    """

    registry = []

    def __init__(self, func):
        super().__init__(func)
        self.func = func

    def __set_name__(self, cls, name):
        assert name == self.func.__name__, (name, self.func.__name__)
        type(self).registry.append((self.func, cls))


class Examples:
    """Code examples for docs using "literalinclude"."""


class WorkloadExamples(Examples):
    """Examples of a single concurrency workload."""


#######################################
# workload: grep
#######################################

class GrepExamples(WorkloadExamples):

    @staticmethod
    def app(name, kind='basic', cf=False):
        import shlex
        from grep import Options, resolve_impl, grep
        from grep.__main__ import render_matches
        opts = Options(
            #recursive=True,
            #ignorecase = True,
            #invertmatch = True,
            #showfilename = True,
            #showfilename = False,
            #filesonly = 'invert',
            #filesonly = 'match',
            #showonlymatch = True,
            #quiet = True,
            #hideerrors = True,
        )
        #opts = Options(recursive=True, filesonly='match')
        impl = resolve_impl(name, kind, cf)
        pat = 'help'
        #filenames = '.'
        filenames = ['make.bat', 'Makefile']
        print(f'# grep {opts} {shlex.quote(pat)} {shlex.join(filenames)}')
        print()
        matches = grep(pat, opts, *filenames, impl=impl)
        if name == 'asyncio':
            assert hasattr(type(matches), '__aiter__'), (name,)
            async def search():
                async for line in render_matches(matches, opts):
                    print(line)
            import asyncio
            asyncio.run(search())
        else:
            assert not hasattr(type(matches), '__aiter__'), (name,)
            for line in render_matches(matches, opts):
                print(line)

    @example
    def run_sequentially():
        GrepExamples.app('sequential')

    @example
    def run_using_threads():
        GrepExamples.app('threads')

    @example
    def run_using_cf_threads():
        GrepExamples.app('threads', cf=True)

    @example
    def run_using_subinterpreters():
        GrepExamples.app('interpreters')

    @example
    def run_using_cf_subinterpreters():
        GrepExamples.app('interpreters', cf=True)

    @example
    def run_using_async():
        GrepExamples.app('asyncio')

    @example
    def run_using_multiprocessing():
        GrepExamples.app('multiprocessing')

    @example
    def run_using_cf_multiprocessing():
        GrepExamples.app('multiprocessing', cf=True)


#######################################
# workload: image resizer
#######################################

class ImageResizer(WorkloadExamples):

    @example
    def run_sequentially():
        # [start-image-resizer-sequential]
        # sequential 2
        ...
        # [end-image-resizer-sequential]

    @example
    def run_using_threads():
        # [start-image-resizer-threads]
        import threading

        def task():
            ...

        t = threading.Thread(target=task)
        t.start()

        ...
        # [end-image-resizer-threads]

    @example
    def run_using_cf_thread():
        # [start-image-resizer-cf-thread]
        # concurrent.futures 2
        ...
        # [end-image-resizer-cf-thread]

    @example
    def run_using_subinterpreters():
        # [start-image-resizer-subinterpreters]
        # subinterpreters 2
        ...
        # [end-image-resizer-subinterpreters]

    @example
    def run_using_cf_subinterpreters():
        # [start-image-resizer-cf-subinterpreters]
        # concurrent.futures 2
        ...
        # [end-image-resizer-cf-subinterpreters]

    @example
    def run_using_async():
        # [start-image-resizer-async]
        # async 2
        ...
        # [end-image-resizer-async]

    @example
    def run_using_multiprocessing():
        # [start-image-resizer-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-image-resizer-multiprocessing]

    @example
    def run_using_cf_multiprocessing():
        # [start-image-resizer-cf-multiprocessing]
        # concurrent.futures 2
        ...
        # [end-image-resizer-cf-multiprocessing]


#######################################
# workload 2: ...
#######################################

class Workload2(WorkloadExamples):

    @example
    def run_sequentially():
        # [start-w2-sequential]
        # sequential 3
        ...
        # [end-w2-sequential]

    @example
    def run_using_threads():
        # [start-w2-threads]
        import threading

        def task():
            ...

        t = threading.Thread(target=task)
        t.start()

        ...
        # [end-w2-threads]

    @example
    def run_using_cf_thread():
        # [start-w2-cf-thread]
        # concurrent.futures 3
        ...
        # [end-w2-cf-thread]

    @example
    def run_using_subinterpreters():
        # [start-w2-subinterpreters]
        # subinterpreters 3
        ...
        # [end-w2-subinterpreters]

    @example
    def run_using_cf_subinterpreters():
        # [start-w2-cf-subinterpreters]
        # concurrent.futures 3
        ...
        # [end-w2-cf-subinterpreters]

    @example
    def run_using_async():
        # [start-w2-async]
        # async 3
        ...
        # [end-w2-async]

    @example
    def run_using_multiprocessing():
        # [start-w2-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-w2-multiprocessing]

    @example
    def run_using_cf_multiprocessing():
        # [start-w2-cf-multiprocessing]
        # concurrent.futures 3
        ...
        # [end-w2-cf-multiprocessing]


# [start-w2-cf]
# concurrent.futures 2
...
# [end-w2-cf]


#######################################
# workload 3: ...
#######################################

class Workload3(WorkloadExamples):

    @example
    def run_sequentially():
        # [start-w3-sequential]
        # sequential 3
        ...
        # [end-w3-sequential]

    @example
    def run_using_threads():
        # [start-w3-threads]
        import threading

        def task():
            ...

        t = threading.Thread(target=task)
        t.start()

        ...
        # [end-w3-threads]

    @example
    def run_using_cf_thread():
        # [start-w3-cf-thread]
        # concurrent.futures 3
        ...
        # [end-w3-cf-thread]

    @example
    def run_using_subinterpreters():
        # [start-w3-subinterpreters]
        # subinterpreters 3
        ...
        # [end-w3-subinterpreters]

    @example
    def run_using_cf_subinterpreters():
        # [start-w3-cf-subinterpreters]
        # concurrent.futures 3
        ...
        # [end-w3-cf-subinterpreters]

    @example
    def run_using_async():
        # [start-w3-async]
        # async 3
        ...
        # [end-w3-async]

    @example
    def run_using_multiprocessing():
        # [start-w3-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-w3-multiprocessing]

    @example
    def run_using_cf_multiprocessing():
        # [start-w3-cf-multiprocessing]
        # concurrent.futures 3
        ...
        # [end-w3-cf-multiprocessing]


# [start-w3-cf]
# concurrent.futures 3
...
# [end-w3-cf]


ALL = object()


def registry_resolve(registry, name, kind=None):
    orig = name
    if name is None:
        name = ''
    while True:
        try:
            return name, registry[name]
        except KeyError:
            # Try aliases.
            try:
                name = registry[f'|{name}|']
            except KeyError:
                raise ValueError(f'unsupported {kind or "???"} registry key {name!r}')


def registry_resolve_all(registry, name=ALL, kind=None):
    if name is ALL or name is registry:
        for pair in registry.items():
            name, _ = pair
            if name.startswith('|'):
                continue
            yield pair
    else:
        yield registry_resolve(registry, name, kind)


def registry_render(registry, kind=None, indent='', vfmt=None, parent=None):
    kind = f'{kind} - ' if kind else ''
    entries = {}
    aliases_by_target = {}
    default = None
    for name, value in registry.items():
        if name == '||':
            default, _ = registry_resolve(registry, value)
        elif name.startswith('|'):
            target, _ = registry_resolve(registry, value)
            try:
                aliases = aliases_by_target[target]
            except KeyError:
                aliases = aliases_by_target[target] = []
            aliases.append(name.strip('|'))
        else:
            entries[name] = value
    for name, spec in entries.items():
        label = f'{parent}.{name}' if parent else name
        line = f'{indent}* {kind}{label}'
        if vfmt:
            specstr = vfmt(spec, name) if callable(vfmt) else vfmt.format(spec)
            line = f'{line:30} => {specstr}'
        if name == default:
            line = f'{line:60} (default)'
        aliases = aliases_by_target.get(name)
        if aliases:
            line = f'{line:70} (aliases: {", ".join(aliases)})'
        yield line, name, spec


EXAMPLES = {
    'grep': {
        'IMPLS': {
            '||': 'sequential',
            'sequential': {
                '||': 'basic',
                'basic': '<>',
                'parts': ['grep-parts.py', '--impl', 'sequential'],
            },
            'threads': {
                '||': 'basic',
                'basic': '<>',
                'cf': '<>',
                'parts': ['grep-parts.py', '--impl', 'threads'],
                'parts-cf': ['grep-parts.py', '--impl', 'threads-cf'],
            },
            'interpreters': {
                '||': 'basic',
                'basic': '<>',
                #'cf': '<>',
                'parts': ['grep-parts.py', '--impl', 'interpreters'],
            },
            '|async|': 'asyncio',
            'asyncio': {
                '||': 'basic',
                'basic': '<>',
                'parts': ['grep-parts.py', '--impl', 'asyncio'],
            },
            'multiprocessing': {
                '||': 'basic',
                'basic': '<>',
                #'cf': '<>',
                'parts': ['grep-parts.py', '--impl', 'multiprocessing'],
            },
        },
        'OPTS': {
            '||': 'min',
            '|min|': 'min.all',
            'min.all': "-r '.. index::' .",
            'min.mixed': 'help Makefile make.bat requirements.txt',
        },
    },
}


class Selection(namedtuple('Selection', 'app model impl')):

    _opts = None
    _cmd = None
    _executable = None
    _executable_argv = None
    _subargv = None
    _subargvstr = None
    _defaultimpl = None

    REGEX = re.compile(rf"""
        ^
        ( \* | \w+ )  # <app>
        (?:
            \.
            ( \* | \w+ )  # <model>
            (?:
                \.
                ( \* | \w+ )  # <impl>
             )?
         )?
        (?:
            :
            ( \S+ (?: \s+ \S+ )? )  # <opts>
         )?
        \s*
        $
        """, re.VERBOSE)

    @classmethod
    def parse(cls, text):
        m = cls.REGEX.match(text)
        if not m:
            raise ValueError(f'unsupported selection {text!r}')
        app, model, impl, opts = m.groups()

        if app == '*':
            app = ALL
        if model == '*':
            model = ALL
        if impl == '*':
            impl = ALL
        if opts == '*':
            opts = ALL
        self = cls(app, model, impl, opts)
        return self

    @classmethod
    def _from_resolved(cls, app, model, impl, opts, cmd, argv,
                       defaultimpl=False,
                       ):
        self = cls.__new__(cls, app, model, impl, opts)
        executable_argv = None
        if cmd == '<>':
            cmd = f'{app}-{model}' if defaultimpl else f'{app}-{model}-{impl}'
            executable = cmd + '.py'
        elif isinstance(cmd, str):
            executable = cmd
        else:
            cmd = tuple(cmd)
            executable, *executable_argv = cmd
        if os.path.basename(executable) == executable:
            executable = os.path.join(IMPLS_DIR, executable)
            if not os.path.exists(executable):
                executable = os.path.basename(executable)
        self._cmd = cmd
        self._executable = executable
        self._executable_argv = executable_argv
        if isinstance(argv, str):
            self._subargvstr = argv
        else:
            self._subargv = tuple(argv)
        self._defaultimpl = defaultimpl
        return self

    def __new__(cls, app, model, impl, opts=None):
        self = super().__new__(cls, app, model or None, impl or None)
        self._opts = opts or None
        return self

    @property
    def opts(self):
        return self._opts

    @property
    def cmd(self):
        for bad in (ALL, None):
            if bad in self or self._opts is bad:
                raise RuntimeError('not resolved')
        return self._cmd

    @property
    def executable(self):
        for bad in (ALL, None):
            if bad in self or self._opts is bad:
                raise RuntimeError('not resolved')
        return self._executable

    @property
    def executable_argv(self):
        if self._executable_argv is None:
            return ()
        return self._executable_argv

    @property
    def subargv(self):
        for bad in (ALL, None):
            if bad in self or self._opts is bad:
                raise RuntimeError('not resolved')
        if self._subargv is None:
            self._subargv = tuple(shlex.split(self._subargvstr))
        return self._subargv

    @property
    def argv(self):
        for bad in (ALL, None):
            if bad in self or self._opts is bad:
                raise RuntimeError('not resolved')
        argv = [self._executable]
        if self._executable.endswith('.py'):
            argv.insert(0, sys.executable)
        if self._executable_argv:
            argv.extend(self._executable_argv)
        if self._subargv is None:
            self._subargv = shlex.split(self._subargvstr)
        argv.extend(self._subargv)
        return argv

    def resolve(self, argv=None):
        cls = type(self)
        if self._cmd and self._cmd is not ALL:
            assert self._argv is not ALL
            if argv is None:
                resolved = self
            else:
                default = self._defaultimpl
                resolved = cls._from_resolved(
                            *self, self._opts, self._cmd, argv, default)
            yield resolved
        else:
            assert self.app is not None
            requested = argv
            resolve = registry_resolve_all
            for app, app_spec in resolve(EXAMPLES, self.app, 'app'):
                models = app_spec['IMPLS']
                all_opts = app_spec['OPTS']
                for model, impls in resolve(models, self.model, 'model'):
                    default, _ = registry_resolve(impls, '')
                    for impl, cmd in resolve(impls, self.impl, 'impl'):
                        if requested:
                            argvs = [('???', requested)]
                        else:
                            argvs = resolve(all_opts, self._opts, 'opts')
                        isdefault = (impl == default)
                        for opts, argv in argvs:
                            yield cls._from_resolved(
                                app, model, impl, opts, cmd, argv, isdefault)


#######################################
# A script to run the examples
#######################################

USAGE = 'run-examples [-h|--help] [SELECTION ..] [-- ..]'
HELP = """
SELECTION is one of the following:

  APP
  APP.MODEL
  APP.MODEL.IMPL
  APP:OPTS
  APP.MODEL:OPTS
  APP.MODEL.IMPL:OPTS

where each field is a known name or the wildcard (*).
"""[:-1]

if __name__ == '__main__':
    argv = sys.argv[1:]
    remainder = None
    if '--' in argv:
        index = argv.index('--')
        remainder = argv[index+1:]
        argv = argv[:index]
    if '-h' in argv or '--help' in argv:
        print(USAGE)
        print(HELP)
        print()
        print('Supported selection field values:')
        print()
        indent = ' ' * 2
        def render(reg, kind, depth=0, vfmt=None, parent=None):
            for l, _, s in registry_render(
                    reg, kind, indent*depth, vfmt, parent):
                yield l, s
        for line, app, app_spec in registry_render(EXAMPLES, 'app'):
            print(line)
            app_impls = app_spec['IMPLS']
            print(f'{indent}* concurrency models:')
            for line, _ in render(app_impls, None, 2):
                print(line.format(''))
            print(f'{indent}* implementations:')
            for model, impls in registry_resolve_all(app_impls, kind='model'):
                default, _ = registry_resolve(impls, '')
                def vfmt_impl(cmd, impl, app=app, model=model):
                    opts, argv = '???', ()
                    sel = Selection._from_resolved(
                            app, model, impl, opts, cmd, argv, impl==default)
                    if sel.cmd != sel.executable:
                        return os.path.basename(sel.executable)
                    else:
                        return sel.executable
                for line, cmd in render(impls, None, 2, vfmt_impl, model):
                    print(line)
            print(f'{indent}* argv options:')
            for line, _ in render(app_spec['OPTS'], None, 2, f'{app} {{}}'):
                print(line)
        sys.exit(0)

    if not argv:
        argv = ['*.*.*:*']

    div = '#' * 40
    for text in argv:
        sel = Selection.parse(text)
        for sel in sel.resolve(remainder or None):
            print()
            print(div)
            print('# app:    ', sel.app)
            print('# model:  ', sel.model)
            print('# impl:   ', sel.impl)
            print('# opts:   ', sel.opts)
            print('# script: ', sel.executable, *sel.executable_argv)
            print('# argv:   ', sel.app, shlex.join(sel.subargv))
            print(div)
            print()
            proc = subprocess.run(sel.argv)
            print()
            print('# returncode:', proc.returncode)
