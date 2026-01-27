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
        "-o",
        "--output-dir",
        help="where to output generated files",
        required=True,
        type=lambda p: pathlib.Path(p).resolve(),
    )
    parser.add_argument(
        "-p",
        "--pyconfig-dir",
        help="where to find pyconfig.h",
        required=True,
        type=lambda p: pathlib.Path(p).resolve(),
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="echo commands as they are run"
    )
    parser.add_argument(
        "--cflags", help="additional flags to pass to the compiler", default=""
    )
    parser.add_argument("--llvm-version", help="LLVM version to use")
    args = parser.parse_args()
    for target in args.target:
        target.debug = args.debug
        target.force = args.force
        target.verbose = args.verbose
        target.cflags = args.cflags
        target.pyconfig_dir = args.pyconfig_dir
        if args.llvm_version:
            target.llvm_version = args.llvm_version
        target.build(
            comment=comment,
            force=args.force,
            jit_stencils=args.output_dir / f"jit_stencils-{target.triple}.h",
        )
    jit_stencils_h = args.output_dir / "jit_stencils.h"
    lines = [f"// {comment}\n"]
    guard = "#if"
    for target in args.target:
        lines.append(f"{guard} {target.condition}\n")
        lines.append(f'#include "jit_stencils-{target.triple}.h"\n')
        guard = "#elif"
    lines.append("#else\n")
    lines.append('#error "unexpected target"\n')
    lines.append("#endif\n")
    body = "".join(lines)
    # Don't touch the file if it hasn't changed (so we don't trigger a rebuild):
    if not jit_stencils_h.is_file() or jit_stencils_h.read_text() != body:
        jit_stencils_h.write_text(body)
