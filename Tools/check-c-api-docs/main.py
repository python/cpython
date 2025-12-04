import re
from pathlib import Path
import sys
import _colorize
import textwrap

SIMPLE_FUNCTION_REGEX = re.compile(r"PyAPI_FUNC(.+) (\w+)\(")
SIMPLE_MACRO_REGEX = re.compile(r"# *define *(\w+)(\(.+\))? ")
SIMPLE_INLINE_REGEX = re.compile(r"static inline .+( |\n)(\w+)")
SIMPLE_DATA_REGEX = re.compile(r"PyAPI_DATA\(.+\) (\w+)")

CPYTHON = Path(__file__).parent.parent.parent
INCLUDE = CPYTHON / "Include"
C_API_DOCS = CPYTHON / "Doc" / "c-api"
IGNORED = (
    (CPYTHON / "Tools" / "check-c-api-docs" / "ignored_c_api.txt")
    .read_text()
    .split("\n")
)

for index, line in enumerate(IGNORED):
    if line.startswith("#"):
        IGNORED.pop(index)

MISTAKE = """
If this is a mistake and this script should not be failing, create an
issue and tag Peter (@ZeroIntensity) on it.\
"""


def found_undocumented(singular: bool) -> str:
    some = "an" if singular else "some"
    s = "" if singular else "s"
    these = "this" if singular else "these"
    them = "it" if singular else "them"
    were = "was" if singular else "were"

    return (
        textwrap.dedent(
            f"""
    Found {some} undocumented C API{s}!

    Python requires documentation on all public C API symbols, macros, and types.
    If {these} API{s} {were} not meant to be public, prefix {them} with a
    leading underscore (_PySomething_API) or move {them} to the internal C API
    (pycore_*.h files).

    In exceptional cases, certain APIs can be ignored by adding them to
    Tools/check-c-api-docs/ignored_c_api.txt
    """
        )
        + MISTAKE
    )


def found_ignored_documented(singular: bool) -> str:
    some = "a" if singular else "some"
    s = "" if singular else "s"
    them = "it" if singular else "them"
    were = "was" if singular else "were"
    they = "it" if singular else "they"

    return (
        textwrap.dedent(
            f"""
    Found {some} C API{s} listed in Tools/c-api-docs-check/ignored_c_api.txt, but
    {they} {were} found in the documentation. To fix this, remove {them} from
    ignored_c_api.txt.
    """
        )
        + MISTAKE
    )


def is_documented(name: str) -> bool:
    """
    Is a name present in the C API documentation?
    """
    for path in C_API_DOCS.iterdir():
        if path.is_dir():
            continue
        if path.suffix != ".rst":
            continue

        text = path.read_text(encoding="utf-8")
        if name in text:
            return True

    return False


def scan_file_for_docs(filename: str, text: str) -> tuple[list[str], list[str]]:
    """
    Scan a header file for  C API functions.
    """
    undocumented: list[str] = []
    documented_ignored: list[str] = []
    colors = _colorize.get_colors()

    def check_for_name(name: str) -> None:
        documented = is_documented(name)
        if documented and (name in IGNORED):
            documented_ignored.append(name)
        elif not documented and (name not in IGNORED):
            undocumented.append(name)

    for function in SIMPLE_FUNCTION_REGEX.finditer(text):
        name = function.group(2)
        if not name.startswith("Py"):
            continue

        check_for_name(name)

    for macro in SIMPLE_MACRO_REGEX.finditer(text):
        name = macro.group(1)
        if not name.startswith("Py"):
            continue

        if "(" in name:
            name = name[: name.index("(")]

        check_for_name(name)

    for inline in SIMPLE_INLINE_REGEX.finditer(text):
        name = inline.group(2)
        if not name.startswith("Py"):
            continue

        check_for_name(name)

    for data in SIMPLE_DATA_REGEX.finditer(text):
        name = data.group(1)
        if not name.startswith("Py"):
            continue

        check_for_name(name)

    # Remove duplicates and sort alphabetically to keep the output deterministic
    undocumented = list(set(undocumented))
    undocumented.sort()

    if undocumented or documented_ignored:
        print(f"{filename} {colors.RED}BAD{colors.RESET}")
        for name in undocumented:
            print(f"{colors.BOLD_RED}UNDOCUMENTED:{colors.RESET} {name}")
        for name in documented_ignored:
            print(f"{colors.BOLD_YELLOW}DOCUMENTED BUT IGNORED:{colors.RESET} {name}")
    else:
        print(f"{filename} {colors.GREEN}OK{colors.RESET}")

    return undocumented, documented_ignored


def main() -> None:
    print("Scanning for undocumented C API functions...")
    files = [*INCLUDE.iterdir(), *(INCLUDE / "cpython").iterdir()]
    all_missing: list[str] = []
    all_found_ignored: list[str] = []

    for file in files:
        if file.is_dir():
            continue
        assert file.exists()
        text = file.read_text(encoding="utf-8")
        missing, ignored = scan_file_for_docs(str(file.relative_to(INCLUDE)), text)
        all_found_ignored += ignored
        all_missing += missing

    fail = False
    to_check = [
        (all_missing, "missing", found_undocumented(len(all_missing) == 1)),
        (
            all_found_ignored,
            "documented but ignored",
            found_ignored_documented(len(all_found_ignored) == 1),
        ),
    ]
    for name_list, what, message in to_check:
        if not name_list:
            continue

        s = "s" if len(name_list) != 1 else ""
        print(f"-- {len(name_list)} {what} C API{s} --")
        for name in name_list:
            print(f" - {name}")
        print(message)
        fail = True

    sys.exit(1 if fail else 0)


if __name__ == "__main__":
    main()
