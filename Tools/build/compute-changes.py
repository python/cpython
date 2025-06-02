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
GITHUB_CODEOWNERS_PATH = Path(".github/CODEOWNERS")
GITHUB_WORKFLOWS_PATH = Path(".github/workflows")

CONFIGURATION_FILE_NAMES = frozenset({
    ".pre-commit-config.yaml",
    ".ruff.toml",
    "mypy.ini",
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


@dataclass(kw_only=True, slots=True)
class Outputs:
    run_ci_fuzz: bool = False
    run_docs: bool = False
    run_tests: bool = False
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
        outputs = Outputs(run_tests=True, run_windows_tests=True)
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


def process_changed_files(changed_files: Set[Path]) -> Outputs:
    run_tests = False
    run_ci_fuzz = False
    run_docs = False
    run_windows_tests = False
    run_windows_msi = False

    for file in changed_files:
        # Documentation files
        doc_or_misc = file.parts[0] in {"Doc", "Misc"}
        doc_file = file.suffix in SUFFIXES_DOCUMENTATION or doc_or_misc

        if file.parent == GITHUB_WORKFLOWS_PATH:
            if file.name == "build.yml":
                run_tests = run_ci_fuzz = True
            if file.name == "reusable-docs.yml":
                run_docs = True
            if file.name == "reusable-windows-msi.yml":
                run_windows_msi = True

        if not (
            doc_file
            or file == GITHUB_CODEOWNERS_PATH
            or file.name in CONFIGURATION_FILE_NAMES
        ):
            run_tests = True

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

    return Outputs(
        run_ci_fuzz=run_ci_fuzz,
        run_docs=run_docs,
        run_tests=run_tests,
        run_windows_tests=run_windows_tests,
        run_windows_msi=run_windows_msi,
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
        f.write(f"run-ci-fuzz={bool_lower(outputs.run_ci_fuzz)}\n")
        f.write(f"run-docs={bool_lower(outputs.run_docs)}\n")
        f.write(f"run-tests={bool_lower(outputs.run_tests)}\n")
        f.write(f"run-windows-tests={bool_lower(outputs.run_windows_tests)}\n")
        f.write(f"run-windows-msi={bool_lower(outputs.run_windows_msi)}\n")


def bool_lower(value: bool, /) -> str:
    return "true" if value else "false"


if __name__ == "__main__":
    compute_changes()
