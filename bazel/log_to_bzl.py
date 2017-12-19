#!/usr/bin/env python3


'''
Generate the data file that can be loaded by the BUILD file of cpython.


The log file must be generated using the following command:
    ./configure
    make &> build.log

You MUST NOT use -j with the `make` program.



This script relies on the order in which the targets are builts.  For easy
reference, list an excerpt here:

        # A CC target that compiles a source file under Programs/ contains the
        # main() function and should be excluded from the sources of the cpython
        # library.
	CC      Programs/python.c
	CC      Parser/acceler.c

        # These are the .c files composing the libpython3.7m.a library.
        ...

        # Link the cpython library.
	AR      libpython3.7m.a

        # Link the python executable.
	LINK    python

        # From here on, the pattern would be one or more CC target(s) followed
        # by a DYLIB target.  It means that the CC targets are used to build the
        # DYLIB target, i.e., the python module.  This is also why the build.log
        # must not be generated with `make -j X` where X > 1.
        #
        # The only special case that the _math.o object file is linked to both
        # the cmath module and and the math module.
	CC      Modules/_math.c
	CC      Modules/_struct.c
	DYLIB   _struct.cpython-37m-x86_64-linux-gnu.so
        ...

        # Compile the main() function for _testembed.  But this is unused.
	CC      Programs/_testembed.c

This script depends on the above described order.  If the above order is no
longer valid, this script will be broken.
'''


import argparse
import jinja2
import os
import re
import sys
import subprocess

from parse_build_log import parse_cpython_build_log

THIS_DIR = os.path.dirname(os.path.abspath(__file__))

def generate_bzl(parsed_results):
    ''' Generate the content of the cpython.bzl file.

    The cpython library has two primary libraries: crypto and ssl.  This genrule
    should build these two libraries.

    Args:
        parsed_results: The parsed results of the build log.  The format is
            described in the __doc__ string of the parse_cpython_build_log()
            function in parse_build_log.py.  Note that parsed_results is a
            generator.  This forces this method to complete everything with one
            pass.

    Returns:
        A python3 string representing the content of the bazel rule file for
        building cpython.
    '''
    cpython_srcs = []
    module = dict(srcs=set(), includes=set(), textual_hdrs=set())
    modules_data = []
    # Has linked the python binary or not.
    finished_python = False
    for ele in parsed_results:
        type = ele['type']
        target = ele['target']
        cmd = ele['cmd']
        if not finished_python:
            if type == 'CC':
                if target.startswith('Programs/'):
                    # This target contains the main() function.  Should be excluded
                    # from the cpython library sources.
                    continue
                elif target == 'Modules/config.c':
                    # This file is generated.  Should not include here.
                    continue
                else:
                    cpython_srcs.append(target)
            elif type == 'LINK':
                if target != 'python':
                    raise RuntimeError(
                            "unknow LINK target '%s', expecting 'python'" %
                            target)
                finished_python = True
            else:
                pass
        else:
            if type == 'CC':
                if target == 'Moduels/_math.c':
                    continue

                # These special treatments are found via:
                #       git grep -n '#include ".*\.c"'
                if target == 'Modules/_blake2/blake2b_impl.c':
                    module['textual_hdrs'] |= set([
                        'Modules/_blake2/impl/blake2b-ref.c',
                        'Modules/_blake2/impl/blake2b.c',
                    ])
                    module['includes'] |= set([
                        'Modules/_blake2',
                    ])
                if target == 'Modules/_blake2/blake2s_impl.c':
                    module['textual_hdrs'] |= set([
                        'Modules/_blake2/impl/blake2s-ref.c',
                        'Modules/_blake2/impl/blake2s.c',
                    ])
                    module['includes'] |= set([
                        'Modules/_blake2',
                    ])
                elif target == 'Modules/_sha3/sha3module.c':
                    module['textual_hdrs'] |= set([
                        'Modules/_sha3/kcp/KeccakHash.c',
                        'Modules/_sha3/kcp/KeccakP-1600-inplace32BI.c',
                        'Modules/_sha3/kcp/KeccakP-1600-opt64.c',
                        'Modules/_sha3/kcp/KeccakSponge.c',
                        # KeccakSponge.c includes:
                        'Modules/_sha3/kcp/KeccakSponge.inc',
                        # KeccakP-1600-opt64.c includes:
                        'Modules/_sha3/kcp/KeccakP-1600-64.macros',
                        # KeccakP-1600-opt64.c includes:
                        'Modules/_sha3/kcp/KeccakP-1600-unrolling.macros',
                    ])
                    module['includes'] |= set([
                        'Modules/_sha3',
                    ])
                elif target == 'Modules/expat/xmltok.c':
                    module['textual_hdrs'] |= set([
                        'Modules/expat/xmltok_impl.c',
                        'Modules/expat/xmltok_ns.c',
                    ])
                    module['includes'] |= set([
                        'Modules/expat',
                    ])
                elif target == 'Modules/_decimal/_decimal.c':
                    module['includes'] |= set([
                        'Modules/_decimal/libmpdec',
                    ])


                module['srcs'].add(target)
            elif type == 'DYLIB':
                # Build files in modules_data into this target as a python
                # module.
                #
                # For example:
                #       DYLIB    _struct.cpython-37m-x86_64-linux-gnu.so
                #
                #
                pattern = r'^([_a-z0-9]+)\.cpython-\d\dm?-x86(_64)?-linux-gnu\.so'
                match = re.match(pattern, target)
                module_name = match.groups()[0]
                # Some .c files #include other .c files.  All such .c files
                # should be included in the source.  They can be found via:
                #        git grep -n '#include ".+\.c"'
                if module_name in ['cmath', 'math']:
                    module['srcs'].add('Modules/_math.c')

                module['includes'] = sorted(list(module['includes']))
                module['srcs'] = sorted(list(module['srcs']))
                module['textual_hdrs'] = sorted(list(module['textual_hdrs']))
                modules_data.append((module_name, module))
                module = dict(srcs=set(), includes=set(), textual_hdrs=set())
    data = {
        'cpython': {
            'srcs': sorted(cpython_srcs),
        },
        'modules_data': sorted(modules_data, key=lambda x: x[0]),
    }
    template_fname = os.path.join(THIS_DIR, 'generated_data.bzl.j2')
    template = jinja2.Template(open(template_fname).read())
    return template.render(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
            formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    def __file_type__(s):
        if s is '-':
            return sys.stdin
        else:
            return open(s, 'r')
    parser.add_argument('build_log',
            type=__file_type__,
            help='''the build log file to parse, '-' indicates using stdin as
            the input file''')
    parser.add_argument('--format',
            choices={'human', 'bazel'},
            default='human',
            help='''human: output a human readable format of the build log;
            bazel: output the .bzl rule file.''')

    args = parser.parse_args()

    parsed_results = parse_cpython_build_log(args.build_log)
    if args.format == 'human':
        for e in parsed_results:
            print('\t%s\t%s' % (e['type'], e['target']))
    else:
        print(generate_bzl(parsed_results))



if __name__ == '__main__':
    main()
