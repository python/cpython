__lazy_modules__ = [
    "shlex",
    "shutil",
    "_shared",
]

import shlex
import shutil

import _shared


def dist_test_args(context):
    runner = context.dist_python_interpreter
    if not runner.is_file():
        raise FileNotFoundError(
            f"Packaged WASI interpreter not found at {runner}; "
            "run `gather`, `package`, or `release` first"
        )

    test_opts = shlex.join(["--python", shlex.quote(str(runner))])
    return [
        "TESTPYTHON=$(RUNSHARED) $(PYTHON_FOR_FREEZE) $(TESTPYTHONOPTS)",
        f"TESTOPTS=$(EXTRATESTOPTS) {test_opts}",
    ]


@_shared.subdir("wasi_build_path")
def test(context, _working_dir):
    match context.ci_mode:
        case "fast":
            make_target = "test"
        case "slow":
            make_target = "buildbottest"
        case _:
            raise ValueError(f"Unknown CI mode {context.ci_mode}")
    command = ["make", make_target]
    match context.target:
        case "archive":
            archive = context.archive_path
            if not archive.is_file():
                raise FileNotFoundError(
                    f"Release archive not found at {archive}; "
                    "run `package` or `release` first"
                )
            if context.archive_dir.exists():
                _shared.log("🧹", f"Deleting {context.archive_dir} ...")
                shutil.rmtree(context.archive_dir)
            context.call(
                [
                    "tar",
                    "--extract",
                    "--file",
                    archive,
                    "--directory",
                    context.dist_path,
                    "--",
                    context.archive_dir.name,
                ],
            )
            command.extend(dist_test_args(context))
        case "dist":
            if not context.dist_python_interpreter.is_file():
                raise FileNotFoundError(
                    f"Python not found at {context.dist_python_interpreter}"
                )
            command.extend(dist_test_args(context))
        case "build":
            pass
        case _:
            raise ValueError(f"Unknown target {context.target}")

    context.call(command, env=_shared.updated_env())
