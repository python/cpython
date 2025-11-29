import re
from pathlib import Path
import sys
import _colorize

SIMPLE_FUNCTION_REGEX = re.compile(r"PyAPI_FUNC(.+) (\w+)\(")
SIMPLE_MACRO_REGEX = re.compile(r"# *define *(\w+)(\(.+\))? ")
SIMPLE_INLINE_REGEX = re.compile(r"static inline .+( |\n)(\w+)")
SIMPLE_DATA_REGEX = re.compile(r"PyAPI_DATA\(.+\) (\w+)")

_CPYTHON = Path(__file__).parent.parent.parent
INCLUDE = _CPYTHON / "Include"
C_API_DOCS = _CPYTHON / "Doc" / "c-api"
IGNORED = (_CPYTHON / "Tools" / "c-api-docs-check" / "ignored_c_api.txt").read_text().split("\n")

for index, line in enumerate(IGNORED):
    if line.startswith("#"):
        IGNORED.pop(index)


def is_documented(name: str) -> bool:
    """
    Is a name present in the C API documentation?
    """
    if name in IGNORED:
        return True

    for path in C_API_DOCS.iterdir():
        if path.is_dir():
            continue
        if path.suffix != ".rst":
            continue

        text = path.read_text(encoding="utf-8")
        if name in text:
            return True

    return False


def scan_file_for_missing_docs(filename: str, text: str) -> list[str]:
    """
    Scan a header file for undocumented C API functions.
    """
    undocumented: list[str] = []
    colors = _colorize.get_colors()

    for function in SIMPLE_FUNCTION_REGEX.finditer(text):
        name = function.group(2)
        if not name.startswith("Py"):
            continue

        if not is_documented(name):
            undocumented.append(name)

    for macro in SIMPLE_MACRO_REGEX.finditer(text):
        name = macro.group(1)
        if not name.startswith("Py"):
            continue

        if "(" in name:
            name = name[: name.index("(")]

        if not is_documented(name):
            undocumented.append(name)

    for inline in SIMPLE_INLINE_REGEX.finditer(text):
        name = inline.group(2)
        if not name.startswith("Py"):
            continue

        if not is_documented(name):
            undocumented.append(name)

    for data in SIMPLE_DATA_REGEX.finditer(text):
        name = data.group(1)
        if not name.startswith("Py"):
            continue

        if not is_documented(name):
            undocumented.append(name)

    # Remove duplicates and sort alphabetically to keep the output non-deterministic
    undocumented = list(set(undocumented))
    undocumented.sort()

    if undocumented:
        print(f"{filename} {colors.RED}BAD{colors.RESET}")
        for name in undocumented:
            print(f"{colors.BOLD_RED}UNDOCUMENTED:{colors.RESET} {name}")

        return undocumented
    else:
        print(f"{filename} {colors.GREEN}OK{colors.RESET}")

    return []


def main() -> None:
    print("Scanning for undocumented C API functions...")
    files = [*INCLUDE.iterdir(), *(INCLUDE / "cpython").iterdir()]
    all_missing: list[str] = []
    for file in files:
        if file.is_dir():
            continue
        assert file.exists()
        text = file.read_text(encoding="utf-8")
        missing = scan_file_for_missing_docs(str(file.relative_to(INCLUDE)), text)
        all_missing += missing

    if all_missing != []:
        s = "s" if len(all_missing) != 1 else ""
        print(f"-- {len(all_missing)} missing function{s} --")
        for name in all_missing:
            print(f" - {name}")
        print()
        print(
            "Found some undocumented C API!",
            "Python requires documentation on all public C API functions.",
            "If these function(s) were not meant to be public, please prefix "
            "them with a leading underscore (_PySomething_API) or move them to "
            "the internal C API (pycore_*.h files).",
            "",
            "In exceptional cases, certain functions can be ignored by adding "
            "them to Tools/c-api-docs-check/ignored_c_api.txt",
            "If this is a mistake and this script should not be failing, please "
            "create an issue and tag Peter (@ZeroIntensity) on it.",
            sep="\n",
        )
        sys.exit(1)
    else:
        print("Nothing found :)")
        sys.exit(0)


if __name__ == "__main__":
    main()
