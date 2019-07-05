from . import util


def iter_clean_lines(lines):
    incomment = False
    for line in lines:
        # Deal with comments.
        if incomment:
            _, sep, line = line.partition('*/')
            if sep:
                incomment = False
            continue
        line, _, _ = line.partition('//')
        line, sep, remainder = line.partition('/*')
        if sep:
            _, sep, after = remainder.partition('*/')
            if not sep:
                incomment = True
                continue
            line += ' ' + after

        # Ignore blank lines and leading/trailing whitespace.
        line = line.strip()
        if not line:
            continue

        yield line


def iter_lines(filename, *,
               preprocess=True,
               _get_build=(lambda: _get_build()),
               _preprocess=(lambda *a, **k: _preprocess(*a, **k)),
               ):
    gcc, cflags = _get_build()
    content = _preprocess(filename, gcc, cflags)
    return iter(content.splitlines())


def _get_gcc_and_cflags(*,
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
    gcc, *gccflags = shlex.split(gcc)
    cflags = gccflags + shlex.split(cflags)
    return gcc, cflags


#############################
# GCC preprocessor (platform-specific)

def _preprocess(filename, gcc, cflags, *,
                _run=util.run_cmd,
                ):
    argv = [gcc]
    argv.extend(cflags)
    argv.extend([
            '-E', filename,
            ])
    output = _run(argv)
    return output
