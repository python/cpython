"""Determine which GitHub Actions workflows to run.

Called by ``.github/workflows/reusable-context.yml``.
We only want to run tests on PRs when related files are changed,
or when someone triggers a manual workflow run.
This improves developer experience by not doing (slow)
unnecessary work in GHA, and saves CI resources.
"""

from __future__ import annotations

import os
import subprocess
from dataclasses import dataclass
from pathlib import Path

TYPE_CHECKING = False
if TYPE_CHECKING:
    from collections.abc import Set

GITHUB_DEFAULT_BRANCH = os.environ["GITHUB_DEFAULT_BRANCH"]
GITHUB_WORKFLOWS_PATH = Path(".github/workflows")

RUN_TESTS_IGNORE = frozenset({
    Path("Tools/check-c-api-docs/ignored_c_api.txt"),
    Path(".github/CODEOWNERS"),
    Path(".pre-commit-config.yaml"),
    Path(".ruff.toml"),
    Path("mypy.ini"),
})

UNIX_BUILD_SYSTEM_FILE_NAMES = frozenset({
    Path("aclocal.m4"),
    Path("config.guess"),
    Path("config.sub"),
    Path("configure"),
    Path("configure.ac"),
    Path("install-sh"),
    Path("Makefile.pre.in"),
    Path("Modules/makesetup"),
    Path("Modules/Setup"),
    Path("Modules/Setup.bootstrap.in"),
    Path("Modules/Setup.stdlib.in"),
    Path("Tools/build/regen-configure.sh"),
})

SUFFIXES_C_OR_CPP = frozenset({".c", ".h", ".cpp"})
SUFFIXES_DOCUMENTATION = frozenset({".rst", ".md"})

ANDROID_DIRS = frozenset({"Android"})
IOS_DIRS = frozenset({"Apple", "iOS"})
MACOS_DIRS = frozenset({"Mac"})
WASI_DIRS = frozenset({Path("Tools", "wasm")})


@dataclass(kw_only=True, slots=True)
class Outputs:
    run_android: bool = False
    run_ci_fuzz: bool = False
    run_docs: bool = False
    run_ios: bool = False
    run_macos: bool = False
    run_tests: bool = False
    run_ubuntu: bool = False
    run_wasi: bool = False
    run_windows_msi: bool = False
    run_windows_tests: bool = False


def compute_changes() -> None:
    target_branch, head_ref = git_refs()
    if os.environ.get("GITHUB_EVENT_NAME", "") == "pull_request":
        # Getting changed files only makes sense on a pull request
        files = get_changed_files(target_branch, head_ref)
        outputs = process_changed_files(files)
    else:
        # Otherwise, just run the tests
        outputs = Outputs(
            run_android=True,
            run_ios=True,
            run_macos=True,
            run_tests=True,
            run_ubuntu=True,
            run_wasi=True,
            run_windows_tests=True,
        )
    outputs = process_target_branch(outputs, target_branch)

    if outputs.run_tests:
        print("Run tests")
    if outputs.run_windows_tests:
        print("Run Windows tests")

    if outputs.run_ci_fuzz:
        print("Run CIFuzz tests")
    else:
        print("Branch too old for CIFuzz tests; or no C files were changed")

    if outputs.run_docs:
        print("Build documentation")

    if outputs.run_windows_msi:
        print("Build Windows MSI")

    print(outputs)

    write_github_output(outputs)


def git_refs() -> tuple[str, str]:
    target_ref = os.environ.get("CCF_TARGET_REF", "")
    target_ref = target_ref.removeprefix("refs/heads/")
    print(f"target ref: {target_ref!r}")

    head_ref = os.environ.get("CCF_HEAD_REF", "")
    head_ref = head_ref.removeprefix("refs/heads/")
    print(f"head ref: {head_ref!r}")
    return f"origin/{target_ref}", head_ref


def get_changed_files(
    ref_a: str = GITHUB_DEFAULT_BRANCH, ref_b: str = "HEAD"
) -> Set[Path]:
    """List the files changed between two Git refs, filtered by change type."""
    args = ("git", "diff", "--name-only", f"{ref_a}...{ref_b}", "--")
    print(*args)
    changed_files_result = subprocess.run(
        args, stdout=subprocess.PIPE, check=True, encoding="utf-8"
    )
    changed_files = changed_files_result.stdout.strip().splitlines()
    return frozenset(map(Path, filter(None, map(str.strip, changed_files))))


def get_file_platform(file: Path) -> str | None:
    if not file.parts:
        return None
    first_part = file.parts[0]
    if first_part in MACOS_DIRS:
        return "macos"
    if first_part in IOS_DIRS:
        return "ios"
    if first_part in ANDROID_DIRS:
        return "android"
    if len(file.parts) >= 2 and Path(*file.parts[:2]) in WASI_DIRS: # Tools/wasm/
        return "wasi"
    return None


def process_changed_files(changed_files: Set[Path]) -> Outputs:
    run_tests = False
    run_ci_fuzz = False
    run_docs = False
    run_windows_tests = False
    run_windows_msi = False

    platforms_changed = set()
    has_platform_specific_change = True

    for file in changed_files:
        # Documentation files
        doc_or_misc = file.parts[0] in {"Doc", "Misc"}
        doc_file = file.suffix in SUFFIXES_DOCUMENTATION or doc_or_misc

        if file.parent == GITHUB_WORKFLOWS_PATH:
            if file.name == "build.yml":
                run_tests = run_ci_fuzz = True
                has_platform_specific_change = False
            if file.name == "reusable-docs.yml":
                run_docs = True
            if file.name == "reusable-windows-msi.yml":
                run_windows_msi = True
            if file.name == "reusable-macos.yml":
                platforms_changed.add("macos")
            if file.name == "reusable-wasi.yml":
                platforms_changed.add("wasi")

        if not doc_file and file not in RUN_TESTS_IGNORE:
            run_tests = True

            platform = get_file_platform(file)
            if platform is not None:
                platforms_changed.add(platform)
            else:
                has_platform_specific_change = False
                if file not in UNIX_BUILD_SYSTEM_FILE_NAMES:
                    run_windows_tests = True

        # The fuzz tests are pretty slow so they are executed only for PRs
        # changing relevant files.
        if file.suffix in SUFFIXES_C_OR_CPP:
            run_ci_fuzz = True
        if file.parts[:2] in {
            ("configure",),
            ("Modules", "_xxtestfuzz"),
        }:
            run_ci_fuzz = True

        # Check for changed documentation-related files
        if doc_file:
            run_docs = True

        # Check for changed MSI installer-related files
        if file.parts[:2] == ("Tools", "msi"):
            run_windows_msi = True

    # Check which platform specific tests to run
    if run_tests:
        if not has_platform_specific_change or not platforms_changed:
            run_android = True
            run_ios = True
            run_macos = True
            run_ubuntu = True
            run_wasi = True
        else:
            run_android = "android" in platforms_changed
            run_ios = "ios" in platforms_changed
            run_macos = "macos" in platforms_changed
            run_ubuntu = False
            run_wasi = "wasi" in platforms_changed
    else:
        run_android = False
        run_ios = False
        run_macos = False
        run_ubuntu = False
        run_wasi = False

    return Outputs(
        run_android=run_android,
        run_ci_fuzz=run_ci_fuzz,
        run_docs=run_docs,
        run_ios=run_ios,
        run_macos=run_macos,
        run_tests=run_tests,
        run_ubuntu=run_ubuntu,
        run_wasi=run_wasi,
        run_windows_msi=run_windows_msi,
        run_windows_tests=run_windows_tests,
    )


def process_target_branch(outputs: Outputs, git_branch: str) -> Outputs:
    if not git_branch:
        outputs.run_tests = True

    # CIFuzz / OSS-Fuzz compatibility with older branches may be broken.
    if git_branch != GITHUB_DEFAULT_BRANCH:
        outputs.run_ci_fuzz = False

    if os.environ.get("GITHUB_EVENT_NAME", "").lower() == "workflow_dispatch":
        outputs.run_docs = True
        outputs.run_windows_msi = True

    return outputs


def write_github_output(outputs: Outputs) -> None:
    # https://docs.github.com/en/actions/writing-workflows/choosing-what-your-workflow-does/store-information-in-variables#default-environment-variables
    # https://docs.github.com/en/actions/writing-workflows/choosing-what-your-workflow-does/workflow-commands-for-github-actions#setting-an-output-parameter
    if "GITHUB_OUTPUT" not in os.environ:
        print("GITHUB_OUTPUT not defined!")
        return

    with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as f:
        f.write(f"run-android={bool_lower(outputs.run_android)}\n")
        f.write(f"run-ci-fuzz={bool_lower(outputs.run_ci_fuzz)}\n")
        f.write(f"run-docs={bool_lower(outputs.run_docs)}\n")
        f.write(f"run-ios={bool_lower(outputs.run_ios)}\n")
        f.write(f"run-macos={bool_lower(outputs.run_macos)}\n")
        f.write(f"run-tests={bool_lower(outputs.run_tests)}\n")
        f.write(f"run-ubuntu={bool_lower(outputs.run_ubuntu)}\n")
        f.write(f"run-wasi={bool_lower(outputs.run_wasi)}\n")
        f.write(f"run-windows-msi={bool_lower(outputs.run_windows_msi)}\n")
        f.write(f"run-windows-tests={bool_lower(outputs.run_windows_tests)}\n")


def bool_lower(value: bool, /) -> str:
    return "true" if value else "false"


if __name__ == "__main__":
    compute_changes()
