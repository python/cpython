"""Build an experimental just-in-time compiler for CPython."""

import argparse
import pathlib
import shlex
import sys

import _targets

if __name__ == "__main__":
    comment = f"$ {shlex.join([pathlib.Path(sys.executable).name] + sys.argv)}"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "target",
        nargs="+",
        type=_targets.get_target,
        help="a PEP 11 target triple to compile for",
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
    for target in args.target:
        target.debug = args.debug
        target.force = args.force
        target.verbose = args.verbose
        target.build(
            pathlib.Path.cwd(),
            comment=comment,
            stencils_h=f"jit_stencils-{target.triple}.h",
            force=args.force,
        )

    with open("jit_stencils.h", "w") as fp:
        for idx, target in enumerate(args.target):
            fp.write(f"#{'if' if idx == 0 else 'elif'} {target.condition}\n")
            fp.write(f'#include "jit_stencils-{target.triple}.h"\n')

        fp.write("#else\n")
        fp.write('#error "unexpected target"\n')
        fp.write("#endif\n")
