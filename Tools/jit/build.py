"""Build an experimental just-in-time compiler for CPython."""

import argparse
import pathlib
import shlex
import sys
import typing

import _targets


def _write_target_dispatcher(
    output: pathlib.Path,
    targets: typing.Iterable[_targets._Target[typing.Any, typing.Any]],
    comment: str,
    header_prefix: str,
) -> None:
    lines = [f"// {comment}\n"]
    guard = "#if"
    for target in targets:
        lines.append(f"{guard} {target.condition}\n")
        lines.append(f'#include "{header_prefix}-{target.triple}.h"\n')
        guard = "#elif"
    lines.append("#else\n")
    lines.append('#error "unexpected target"\n')
    lines.append("#endif\n")
    body = "".join(lines)
    # Don't touch the file if it hasn't changed (so we don't trigger a rebuild):
    if not output.is_file() or output.read_text() != body:
        output.write_text(body)


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
    parser.add_argument(
        "--llvm-tools-install-dir", help="Installation location of LLVM tools"
    )
    args = parser.parse_args()
    for target in args.target:
        target.debug = args.debug
        target.force = args.force
        target.verbose = args.verbose
        target.cflags = args.cflags
        target.pyconfig_dir = args.pyconfig_dir
        if args.llvm_version:
            target.llvm_version = args.llvm_version
        if args.llvm_tools_install_dir:
            target.llvm_tools_install_dir = args.llvm_tools_install_dir
        # Build this target's stencils, shim object, and target-specific
        # unwind metadata before writing the generic dispatcher headers below.
        target.build(
            comment=comment,
            force=args.force,
            jit_stencils=args.output_dir / f"jit_stencils-{target.triple}.h",
            jit_shim_object=args.output_dir / f"jit_shim-{target.triple}.o",
            jit_unwind_info=args.output_dir / f"jit_unwind_info-{target.triple}.h",
        )
    # Write the target dispatcher that includes the right stencil header for
    # the platform compiling Python/jit.c.
    _write_target_dispatcher(
        args.output_dir / "jit_stencils.h",
        args.target,
        comment,
        "jit_stencils",
    )
    # Write the matching dispatcher for generated JIT unwind constants.
    _write_target_dispatcher(
        args.output_dir / "jit_unwind_info.h",
        args.target,
        comment,
        "jit_unwind_info",
    )
