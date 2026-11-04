__lazy_modules__ = [
    "shlex",
    "shutil",
    "_shared",
]

import shlex
import shutil

import _build
import _shared


@_build.subdir("wasi_build_path")
def test(context, _working_dir):
    _build.call(["make", "test"], env=_build.updated_env(), context=context)


@_build.subdir("wasi_build_path")
def buildbottest(context, _working_dir):
    if context.unpack:
        archive = context.archive_path
        if not archive.is_file():
            raise FileNotFoundError(
                f"Release archive not found at {archive}; "
                "run `package` or `release` first"
            )
        if context.archive_dir.exists():
            _shared.log("🧹", f"Deleting {context.archive_dir} ...")
            shutil.rmtree(context.archive_dir)
        _build.call(
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
            context=context,
        )

    runner = context.dist_python_interpreter
    if not runner.is_file():
        raise FileNotFoundError(
            f"Packaged WASI interpreter not found at {runner}; "
            "run `gather`, `package`, or `release` first"
        )

    test_opts = shlex.join(["--python", shlex.quote(str(runner))])
    _build.call(
        [
            "make",
            "buildbottest",
            "TESTPYTHON=$(RUNSHARED) $(PYTHON_FOR_FREEZE) $(TESTPYTHONOPTS)",
            f"TESTOPTS=$(EXTRATESTOPTS) {test_opts}",
        ],
        env=_build.updated_env(),
        context=context,
    )
