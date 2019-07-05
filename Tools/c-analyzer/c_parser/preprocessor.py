from . import util


def run(filename, *,
        _gcc=(lambda f: _gcc(f)),
        ):
    return _gcc(filename)


#############################
# GCC preprocessor (platform-specific)

def _gcc(filename, *,
         _get_argv=(lambda: _get_gcc_argv()),
         _run=util.run_cmd,
         ):
    argv = _get_argv()
    argv.extend([
            '-E', filename,
            ])
    output = _run(argv)
    return output


def _get_gcc_argv(*,
                  _open=open,
                  _run=util.run_cmd,
                  ):
    with _open('/tmp/print.mk', 'w') as tmpfile:
        tmpfile.write('print-%:\n')
        #tmpfile.write('\t@echo $* = $($*)\n')
        tmpfile.write('\t@echo $($*)\n')
    argv = ['/usr/bin/make',
            '-f', 'Makefile',
            '-f', '/tmp/print.mk',
            'print-CC',
            'print-PY_CORE_CFLAGS',
            ]
    output = _run(argv)
    gcc, cflags = output.strip().splitlines()
    argv = shlex.split(gcc.strip())
    cflags = shlex.split(cflags.strip())
    return argv + cflags
