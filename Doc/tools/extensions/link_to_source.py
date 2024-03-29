import ast
import os
import pathlib
import re
import shlex
import subprocess
import sys
from collections import defaultdict
from pathlib import Path
from subprocess import PIPE


SRC_DIR = pathlib.Path(__file__).resolve().parent.parent.parent.parent
LIB_DIR = SRC_DIR / "Lib"


def run(command):
    return subprocess.run(
        shlex.split(command), text=True, capture_output=True, check=True, cwd=SRC_DIR
    ).stdout.strip()


def to_remote_https_url(url):
    """Convert git@github.com:python/cpython.git to https://github.com/python/cpython"""
    if url.startswith("https://"):
        url = url.removesuffix(".git")
    elif url.startswith("git@"):
        match = re.match(r"git@(?P<domain>.+):(?P<owner>.+)/(?P<repo>.+).git", url)
        if not match:
            raise ValueError(f"couldn't parse remote origin git url as ssh: {url}")
        return f"https://{match['domain']}/{match['owner']}/{match['repo']}"
    elif not url:
        raise ValueError(
            f'failed to get remote git repo url with "git config --get remote.origin.url"'
        )
    raise ValueError(f'unknown url from "git config --get remote.origin.url": {url}')


commit_hash = run("git rev-parse HEAD")
if not commit_hash:
    raise ValueError(f"failed to get commit hash of latest git commit")

remote_origin_url = run("git config --get remote.origin.url")
source_url = to_remote_https_url(remote_origin_url) + "/blob/" + commit_hash + "/"


# There are a few [-clinic input]'s (with a dash)
CLINIC_INPUT_START = "/*[clinic input]"
CLINIC_INPUT_END = "[clinic start generated code]*/"

links = {}
for cfile in sorted(SRC_DIR.rglob("*.c")):
    filename = str(cfile.relative_to(SRC_DIR))
    with open(cfile) as f:
        lines = f.read().splitlines()
    lines = list(reversed(list(enumerate(lines, start=1))))

    while lines:
        lineno, line = lines.pop()
        if line == CLINIC_INPUT_START:
            while True:
                lineno, next_line = lines.pop()
                # Sometimes there are blank lines before the name starts
                if line == CLINIC_INPUT_START and not next_line.strip():
                    continue
                # Once we've hit the first non-blank line, we need to skip over
                # any @decorator lines. We do this by reading more lines until
                # the next line is either a blank line or an indented line
                # which is the start of the arguments
                if (
                    not next_line.strip()
                    # In statisticsmodule.c the args are indented with 3 spaces
                    or next_line.startswith(" ")
                    or next_line == CLINIC_INPUT_END
                ):
                    break
                line = next_line
            # msvcmodule.c contains empty Argument Clinic blocks
            if line == CLINIC_INPUT_START:
                continue
            name = line.split()[0] if line else ""

            while next_line != CLINIC_INPUT_END and lines:
                lineno, next_line = lines.pop()
            # The C function name is the 3rd line in the generated code
            for _ in range(3):
                lineno, line = lines.pop()

            # Ignore module declarations and C function definitions, e.g.
            # class list "PyListObject *" "&PyList_Type"
            if name and name not in ["class", "module"]:
                # TODO: all other clinic object names are prefixed with their
                # module name except this one.
                if filename == "Modules/_struct.c":
                    name = "_struct." + name
                links[name] = filename, lineno, line


def should_skip(path):
    skip_dirs = ["test", "tests", "idlelib", "idle_test"]
    return path.name.startswith("test_") or any(d in path.parts for d in skip_dirs)


def to_import_path(filename):
    filename = filename.with_suffix("").relative_to(LIB_DIR)
    short_filename = filename.parent if filename.name == "__init__" else filename
    return ".".join(short_filename.parts), ".".join(filename.parts)


# A dict mapping a name to a list of names it's re-exported as.
# If module "a" does "import b as c" then you'll have {'b': ['a.c']}
re_exported_as = defaultdict(list)

for pyfile in sorted(LIB_DIR.rglob("*.py")):
    if should_skip(pyfile.relative_to(LIB_DIR)):
        continue

    filename = str(pyfile.relative_to(SRC_DIR))
    short_import_path, import_path = to_import_path(pyfile)

    with open(pyfile) as f:
        contents = f.read()
    # Using splitlines() would give us line numbers that differ from GitHub's
    # because some Python files contain form feed (\x0c) characters.
    lines = dict(enumerate(contents.split("\n"), start=1))
    try:
        module = ast.parse(contents)
    except SyntaxError:
        continue

    # Only process top level nodes.
    # Definitions and imports in if statements are not processed.
    module_links = {}
    for node in module.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    module_links[target.id] = filename, target.lineno, lines[target.lineno]
                elif isinstance(target, (ast.Tuple, ast.List)):
                    for sub_target in target.elts:
                        if isinstance(sub_target, ast.Name):
                            module_links[sub_target.id] = filename, sub_target.lineno, lines[sub_target.lineno]
                        elif isinstance(sub_target, ast.Starred) and isinstance(sub_target.value, ast.Name):
                            module_links[sub_target.value.id] = filename, sub_target.value.lineno, lines[sub_target.value.lineno]
        elif isinstance(node, ast.ClassDef):
            class_ = node
            module_links[class_.name] = filename, class_.lineno, lines[class_.lineno]
            name = class_.name + '.'
            # Get the class's methods
            for class_node in class_.body:
                if isinstance(class_node, ast.FunctionDef):
                    method = class_node
                    module_links[name + method.name] = filename, method.lineno, lines[method.lineno]
                if isinstance(node, ast.Assign):
                    for target in node.targets:
                        if isinstance(target, ast.Name):
                            module_links[name + target.id] = filename, target.lineno, lines[target.lineno]
                        elif isinstance(target, (ast.Tuple, ast.List)):
                            for sub_target in target.elts:
                                if isinstance(sub_target, ast.Name):
                                    module_links[name + sub_target.id] = filename, sub_target.lineno, lines[sub_target.lineno]
                                elif isinstance(sub_target, ast.Starred) and isinstance(sub_target.value, ast.Name):
                                    module_links[name + sub_target.value.id] = filename, sub_target.value.lineno, lines[sub_target.value.lineno]
        elif isinstance(node, ast.FunctionDef):
            module_links[node.name] = filename, node.lineno, lines[node.lineno]
        elif isinstance(node, ast.ImportFrom):
            imported_path = node.module
            # Convert relative import paths to absolute
            # If you have this import statement in asyncio/__init__.py
            # from .base_events import *
            # we need to turn ".base_events" into "asyncio.base_events"
            if node.level:
                # What the import statement imports
                imported_path = "" if imported_path is None else "." + imported_path
                # Import path of the file where the import statement is in
                imported_from = ".".join(import_path.split(".")[: -node.level])
                imported_path = imported_from + imported_path

            for alias in node.names:
                # alias.name might be "*"
                imported = imported_path + "." + alias.name
                exported_as_name = alias.asname if alias.asname else alias.name
                exported_as = short_import_path + "." + exported_as_name
                re_exported_as[imported].append(exported_as)
        # TODO: variable definitions?

    for name, link in module_links.items():
        full_name = short_import_path + "." + name
        # prefer the C definition we already added
        if full_name not in links:
            links[full_name] = link


# import json
# print(json.dumps(re_exported_as, indent=4))

for name, exported_as_names in re_exported_as.items():
    # Check that if one of the values ends with .* that all values do
    # This should never not happen.
    all_symbols = exported_as_names + [name]
    if "*" in name:
        if not all((s.count("*") == 1 and s.endswith("*")) for s in all_symbols):
            raise ValueError
    else:
        if not all("*" not in n for n in all_symbols):
            raise ValueError

    # If a.C is exported in b as b.C we need to re-export a.C.d and a.C.e, etc.
    # If a.C.* is exported then the only difference is that we don't re-export it as b.C
    # but b.C.d etc. are also re-exported
    if "*" in name:
        name = name.rstrip("*")
        exported_as_names = [n.strip("*") for n in exported_as_names]
    else:
        if name in links:
            for exported_as in exported_as_names:
                if exported_as not in links:
                    links[exported_as] = links[name]
        name = name + "."
        exported_as_names = [n + "." for n in exported_as_names]

    # TODO: is .startswith() too simple? does it add imports it shouldn't?
    #
    # This could miss modules that are exported 2 levels away from where
    # they're defined. e.g. if module b exports module a as b.a and module c then
    # re-exports module a as c.b.a, we won't see it unless we happen to process
    # the modules in that order: if module c gets processed before module a, it
    # won't re-export module a.* as c.b.a.*
    # TODO: check if the above comment is true
    # TODO: this is O(n^2)
    original_names = [n for n in links if n.startswith(name)]
    for original_name in original_names:
        for exported_as in exported_as_names:
            exported_name = original_name.replace(name, exported_as, 1)
            # Don't overwrite already existing definitions with imports.
            # tkinter/tix.py does "from tkinter import *" which imports
            # tkinter.Tk but then defines its own Tk class.
            # This assumes imports come before class definitions that
            # re-define them.
            if exported_name not in links:
                links[exported_name] = links[original_name]


def linkcode_resolve(domain, info):
    if domain != "py" or not info["fullname"]:
        return None

    lookup = info["fullname"]
    if info["module"]:
        lookup = info["module"] + "." + lookup

    if lookup in links:
        filename, lineno, line = links[lookup]
        # print(f"{lookup: <70} {filename: <45} {lineno: <5} {line}")
        return f"{source_url}{filename}#L{lineno}"
    # print(lookup)
    return None
