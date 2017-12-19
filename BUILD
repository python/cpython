licenses(['notice'])

package(default_visibility=['//visibility:public'])

load(':bazel/generated_data.bzl', 'CPYTHON_SRCS', 'CPYTHON_COPTS',
     'CPYTHON_MODULES_DATA', 'CPYTHON_SOABI', 'CPYTHON_MODULE_COPTS',)


# NOTE:
#   Do not run the generated binary, bazel-bin/python, with `bazel run`.  The
#   python program loads a few basic modules such as encoding at startup.  You
#   must set PYTHONPATH to a directory that has those modules.  Usually, the
#   Lib/ directory is good enough.  So you can start running the
#   bazel-bin/python program with
#       PYTHONPATH=Lib bazel-bin/python
#
#   See the doc above the genrule()s in this file for how to load core modules.
cc_binary(
    name = 'python',
    srcs = [
        'Programs/python.c',
    ],
    deps = [
        ':cpython',
    ],
    copts = CPYTHON_COPTS,
    linkopts = [
        '-ldl',
        '-lm',
        '-lutil',
        '-pthread',

        # What is this -rdyanmic?
        #
        #   In gcc, -rdynamic passes the flag --export-dynamic to the ELF
        #   linker.  This instructs the linker to add all symbols, not only
        #   those that are used, to the dynamic symbol table.
        #
        #   In clang, when used with -fuse-ld=lld, -rdynamic causes the
        #   llvm-based linker, ld.lld to export all symbols, not only those that
        #   are used, to the dynamic symbol table.
        #
        #
        # Why is it needed here?
        #
        #   Without this flag, `import math` will report the errors such as
        #   below:
        #       ImportError: .../math.cpython.so: undefined symbol: PyExc_MemoryError
        #
        #
        # How did I find this flag?
        #
        #   The normal `./configure && make` shows that the linking command that
        #   produces the python program has '-Xlinker -export-dynamic', which is
        #   exactly the same as -rdynamic.
        '-rdynamic',
    ],
    data = glob([
        'Lib/**/*.py',
    ]) + [
        ':%s.%s.so' % (module_name, CPYTHON_SOABI) \
                for module_name, _ in CPYTHON_MODULES_DATA
    ],
)

GENERATED_SRCS = [
    'Modules/config.c',
    'pyconfig.h',
]

cc_library(
    name = 'cpython',
    srcs = CPYTHON_SRCS + glob([
        'Modules/**/*.h',
        'Objects/*.h',
        'Objects/*/*.h',
        'Parser/*.h',
        'Python/*.h',
        'Python/clinic/*.h',
    ]) + GENERATED_SRCS,
    hdrs = glob([
        'Include/Python.h',
    ]),
    textual_hdrs = glob([
        'Objects/typeslots.inc',
        'Include/**/*.h',

        # Found via:
        #       git grep -n '#include ".*\.c"'
        # Though, in that output, these two are the last two files.  Other files
        # belong to the building of modules.
        'Modules/getaddrinfo.c',
        'Modules/getnameinfo.c',
    ], exclude = [
        'Include/Python.h'
    ]),
    includes = [
        'Include',
    ] + ([ '.' ] if package_name() else []),
    copts = CPYTHON_COPTS + [
        '-I$(GENDIR)',
    ],
)

genrule(
    name = 'generated_srcs',
    srcs = glob([
        '**/*.in',
        'Include/object.h',
        'Modules/Setup.dist',
        'Modules/makesetup',
        'aclocal.m4',
        'config.guess',
        'config.sub',
        'configure.ac',
        'install-sh',
        'pyconfig.h.in',
    ]),
    tools = [
        'configure',
    ],
    outs = GENERATED_SRCS,
    cmd = ' && '.join([
        'cd ./%s' % package_name(),
        './configure &> /dev/null',
        'cd - &> /dev/null',
    ] + [
        'mv ./%s/%s $(location %s)' % (package_name(), f, f) \
                for f in GENERATED_SRCS
    ]),
)

[cc_library(
    name = '%s.%s-internal' % (module_name, CPYTHON_SOABI),
    visibility = [
        '//visibility:private',
    ],
    srcs = module['srcs'],
    textual_hdrs = module['textual_hdrs'],
    copts = CPYTHON_MODULE_COPTS + [
        '-I./%s/%s' % (package_name(), f) for f in module['includes']
    ],
    linkopts = [
        '-pthread',
        '-lm',
    ],
    deps = [
        # TODO (zhongming): Depending on :cpython is probably an overkill.
        # Should find a way to separate the dependencies better.
        ':cpython',
    ],
) for module_name, module in CPYTHON_MODULES_DATA]


# These are the core python modules.  In a python shell, when you import one of
# these modules, e.g., `import math`, it searches the the paths in sys.path for
# files and packages with name `math`, or dynamic library files with the
# following name:
#       <MODULE_NAME>.<CPYTHON_SOABI>.so
#
# The <MODULE_NAME> is the name of the library to import.  For example, `import
# math` would have a MODULE_NAME of 'math'.
#
# The <CPYTHON_SOABI> is a formatted string such as this:
#       cpython-37m-x86_64-linux-gnu
# CPYTHON_SOABI is defined as a constant in the bazel/generated_data.bzl
# file.
#
# The 'python' program knows what CPYTHON_SOABI to use via the SO_ABI macro at
# compile time.  It only recognizes .so files with the above described naming
# conventions.  The .so and .a files generated by the cc_library() rule does not
# follow that convention, thus necessitating wrapper rules like this.
#
# At runtime, you need to add the directory containing these .so files to
# PYTHONPATH.
[genrule(
    name = '%s.%s-genrule' % (module_name, CPYTHON_SOABI),
    outs = [
        '%s.%s.so' % (module_name, CPYTHON_SOABI),
    ],
    srcs = [
        ':%s.%s-internal' % (module_name, CPYTHON_SOABI),
    ],
    cmd = ' && '.join([
        'mkdir -p $$(dirname $@)',
        #'cp $(location :{0}.{1}) $({0}.{1}.so)' \
        r'''for f in $(locations :{}.{}-internal); do \
                [[ $$f == *.so ]] && cp $$f $@; \
        done'''.format(module_name, CPYTHON_SOABI)
    ]),
) for module_name, module in CPYTHON_MODULES_DATA]
