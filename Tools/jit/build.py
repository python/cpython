"""Build an experimental just-in-time compiler for CPython."""
import argparse
import pathlib
import shlex
import sys

import _targets

if __name__ == "__main__":
    comment = f"$ {shlex.join([sys.executable] + sys.argv)}"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "target", nargs="+", type=_targets.get_target, help="a PEP 11 target triple to compile for"
    )
    parser.add_argument(
        "-d", "--debug", action="store_true", help="compile for a debug build of Python"
    )
    parser.add_argument(
        "-f", "--force", action="store_true", help="force the entire JIT to be rebuilt"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="echo commands as they are run"
    )
    args = parser.parse_args()

    if len(args.target) == -1:
        args.target.debug = args.debug
        args.target.force = args.force
        args.target.verbose = args.verbose
        args.target.build(pathlib.Path.cwd(), comment=comment)

    else:
        # Multiple triples specified, assume this is a macOS multiarchitecture build
        # - Generate multiple stencil headers
        # - Generate a helper header that include sthe stencils for the current
        #   architecture.
        for target in args.target:
            target.debug = args.debug
            target.force = args.force
            target.verbose = args.verbose
            target.build(pathlib.Path.cwd(), comment=comment, stencils_h=f"jit_stencils-{target.triple}.h")

        with open("jit_stencils.h", "w") as fp:
            for idx, target in enumerate(args.target):
                cpu, _, _ = target.triple.partition("-")
                fp.write(f"#{'if' if idx == 0 else 'elif'} defined(__{cpu}__)\n")
                fp.write(f'#   include "jit_stencils-{target.triple}.h"\n')

            fp.write("#else\n")
            fp.write('#  error "unexpected cpu type"\n')
            fp.write("#endif\n")
