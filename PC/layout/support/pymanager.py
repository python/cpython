from .constants import *

URL_BASE = "https://www.python.org/ftp/python/"

XYZ_VERSION = f"{VER_MAJOR}.{VER_MINOR}.{VER_MICRO}"
WIN32_VERSION = f"{VER_MAJOR}.{VER_MINOR}.{VER_MICRO}.{VER_FIELD4}"
FULL_VERSION = f"{VER_MAJOR}.{VER_MINOR}.{VER_MICRO}{VER_SUFFIX}"


def _not_empty(n, key=None):
    result = []
    for i in n:
        if key:
            i_l = i[key]
        else:
            i_l = i
        if not i_l:
            continue
        result.append(i)
    return result


def _unique(n, key=None):
    seen = set()
    result = []
    for i in n:
        if key:
            i_l = i[key]
        else:
            i_l = i
        if not i_l:
            continue
        i_l = i_l.casefold()
        if i_l not in seen:
            seen.add(i_l)
            result.append(i)
    return result


def calculate_install_json(ns, *, for_embed=False, for_test=False):
    TARGET = "python.exe"
    TARGETW = "pythonw.exe"

    SYS_ARCH = {
        "win32": "32bit",
        "amd64": "64bit",
        # Unfortunate, but this is how it's spec'd
        "arm64": "64bit",
    }[ns.arch]
    TAG_ARCH = {
        "win32": "-32",
        "amd64": "-64",
        "arm64": "-arm64",
    }[ns.arch]

    COMPANY = "PythonCore"
    DISPLAY_NAME = "Python"
    TAG_SUFFIX = ""
    ALIAS_PREFIX = "python"
    ALIAS_WPREFIX = "pythonw"
    FILE_PREFIX = "python-"
    FILE_SUFFIX = f"-{ns.arch}"
    DISPLAY_TAGS = [{
        "win32": "32-bit",
        "amd64": "",
        "arm64": "ARM64",
    }[ns.arch]]

    if for_test:
        # Packages with the test suite come under a different Company
        COMPANY = "PythonTest"
        DISPLAY_TAGS.append("with tests")
        FILE_SUFFIX = f"-test-{ns.arch}"
    if for_embed:
        # Embeddable distro comes under a different Company
        COMPANY = "PythonEmbed"
        TARGETW = None
        # Bit unwieldly, but we don't expect people to use the aliases anyway.
        # You should 'install --target' the embeddable distro.
        ALIAS_PREFIX = "embed-python"
        DISPLAY_TAGS.append("embeddable")
        FILE_SUFFIX = f"-embed-{ns.arch}"
    if ns.include_freethreaded:
        # Free-threaded distro comes with a tag suffix
        TAG_SUFFIX = "t"
        TARGET = f"python{VER_MAJOR}.{VER_MINOR}t.exe"
        TARGETW = f"pythonw{VER_MAJOR}.{VER_MINOR}t.exe"
        DISPLAY_TAGS.append("freethreaded")
        FILE_SUFFIX = f"t-{ns.arch}"

    FULL_TAG = f"{VER_MAJOR}.{VER_MINOR}.{VER_MICRO}{VER_SUFFIX}{TAG_SUFFIX}"
    FULL_ARCH_TAG = f"{FULL_TAG}{TAG_ARCH}"
    XY_TAG = f"{VER_MAJOR}.{VER_MINOR}{TAG_SUFFIX}"
    XY_ARCH_TAG = f"{XY_TAG}{TAG_ARCH}"
    if not VER_SUFFIX:
        X_TAG = f"{VER_MAJOR}{TAG_SUFFIX}"
        X_ARCH_TAG = f"{X_TAG}{TAG_ARCH}"
    else:
        X_TAG = X_ARCH_TAG = ""

    ID_TAG = XY_TAG if TAG_ARCH == "-64" else XY_ARCH_TAG

    DISPLAY_SUFFIX = ", ".join(i for i in DISPLAY_TAGS if i)
    if DISPLAY_SUFFIX:
        DISPLAY_SUFFIX = f" ({DISPLAY_SUFFIX})"
    DISPLAY_VERSION = f"{XYZ_VERSION}{VER_SUFFIX}{DISPLAY_SUFFIX}"

    STD_RUN_FOR = []
    STD_ALIAS = []
    STD_PEP514 = []
    STD_START = []
    STD_UNINSTALL = []

    INSTALL_TAGS = [
        FULL_TAG,
        FULL_ARCH_TAG,
        XY_TAG,
        XY_ARCH_TAG,
        X_TAG,
        X_ARCH_TAG,
    ]
    if VER_SUFFIX:
        # X_TAG and XY_TAG doesn't include VER_SUFFIX, so create -dev versions
        INSTALL_TAGS.extend([
            f"{XY_TAG}-dev" if XY_TAG else "",
            f"{XY_TAG}-dev{TAG_ARCH}" if XY_TAG else "",
            f"{X_TAG}-dev" if X_TAG else "",
            f"{X_TAG}-dev{TAG_ARCH}" if X_TAG else "",
        ])

    # Generate run-for and alias entries for each target
    for base in [
        {"target": TARGET},
        {"target": TARGETW, "windowed": 1},
    ]:
        if not base["target"]:
            continue
        STD_RUN_FOR.extend([
            {**base, "tag": FULL_ARCH_TAG},
            {**base, "tag": FULL_TAG},
        ])
        if XY_TAG:
            STD_RUN_FOR.extend([
                {**base, "tag": XY_ARCH_TAG},
                {**base, "tag": XY_TAG},
            ])
            STD_ALIAS.extend([
                {**base, "name": f"{ALIAS_PREFIX}{XY_TAG}.exe"},
                {**base, "name": f"{ALIAS_PREFIX}{XY_ARCH_TAG}.exe"},
            ])
        if X_TAG:
            STD_RUN_FOR.extend([
                {**base, "tag": X_ARCH_TAG},
                {**base, "tag": X_TAG},
            ])
            STD_ALIAS.extend([
                {**base, "name": f"{ALIAS_PREFIX}{X_TAG}.exe"},
                {**base, "name": f"{ALIAS_PREFIX}{X_ARCH_TAG}.exe"},
            ])

    STD_PEP514.append({
        "kind": "pep514",
        "Key": rf"{COMPANY}\{ID_TAG}",
        "DisplayName": f"{DISPLAY_NAME} {DISPLAY_VERSION}",
        "SupportUrl": "https://www.python.org/",
        "SysArchitecture": SYS_ARCH,
        "SysVersion": VER_DOT,
        "Version": FULL_VERSION,
        "InstallPath": {
            "": "%PREFIX%",
            "ExecutablePath": f"%PREFIX%{TARGET}",
        },
        "Help": {
            "Online Python Documentation": {
                "": f"https://docs.python.org/{VER_DOT}/"
            },
        },
    })

    STD_START.append({
        "kind": "start",
        "Name": f"{DISPLAY_NAME} {VER_DOT}{DISPLAY_SUFFIX}",
        "Items": [
            {
                "Name": f"{DISPLAY_NAME} {VER_DOT}{DISPLAY_SUFFIX}",
                "Target": f"%PREFIX%{TARGET}",
                "Icon": f"%PREFIX%{TARGET}",
            },
            {
                "Name": f"{DISPLAY_NAME} {VER_DOT} Online Documentation",
                "Icon": r"%SystemRoot%\System32\SHELL32.dll",
                "IconIndex": 13,
                "Target": f"https://docs.python.org/{VER_DOT}/",
            },
        ],
    })

    if ns.include_idle:
        STD_START[0]["Items"].append({
            "Name": f"IDLE {VER_DOT}{DISPLAY_SUFFIX}",
            "Target": f"%PREFIX%{TARGETW or TARGET}",
            "Arguments": r'"%PREFIX%Lib\idlelib\idle.pyw"',
            "Icon": r"%PREFIX%Lib\idlelib\Icons\idle.ico",
            "IconIndex": 0,
        })
        STD_START[0]["Items"].append({
            "Name": f"PyDoc {VER_DOT}{DISPLAY_SUFFIX}",
            "Target": f"%PREFIX%{TARGET}",
            "Arguments": "-m pydoc -b",
            "Icon": r"%PREFIX%Lib\idlelib\Icons\idle.ico",
            "IconIndex": 0,
        })

    if STD_PEP514:
        if TARGETW:
            STD_PEP514[0]["InstallPath"]["WindowedExecutablePath"] = f"%PREFIX%{TARGETW}"
        if ns.include_html_doc:
            STD_PEP514[0]["Help"]["Main Python Documentation"] = {
                "": rf"%PREFIX%Doc\html\index.html",
            }
            STD_START[0]["Items"].append({
                "Name": f"{DISPLAY_NAME} {VER_DOT} Manuals{DISPLAY_SUFFIX}",
                "Target": r"%PREFIX%Doc\html\index.html",
            })
        elif ns.include_chm:
            STD_PEP514[0]["Help"]["Main Python Documentation"] = {
                "": rf"%PREFIX%Doc\{PYTHON_CHM_NAME}",
            }
            STD_START[0]["Items"].append({
                "Name": f"{DISPLAY_NAME} {VER_DOT} Manuals{DISPLAY_SUFFIX}",
                "Target": "%WINDIR%hhc.exe",
                "Arguments": rf"%PREFIX%Doc\{PYTHON_CHM_NAME}",
            })

    STD_UNINSTALL.append({
        "kind": "uninstall",
        # Other settings will pick up sensible defaults
        "Publisher": "Python Software Foundation",
        "HelpLink": f"https://docs.python.org/{VER_DOT}/",
    })

    data = {
        "schema": 1,
        "id": f"{COMPANY.lower()}-{ID_TAG}",
        "sort-version": FULL_VERSION,
        "company": COMPANY,
        "tag": FULL_TAG if TAG_ARCH == "-64" else FULL_ARCH_TAG,
        "install-for": _unique(INSTALL_TAGS),
        "run-for": _not_empty(STD_RUN_FOR, "tag"),
        "alias": _not_empty(STD_ALIAS, "name"),
        "shortcuts": [
            *STD_PEP514,
            *STD_START,
            *STD_UNINSTALL,
        ],
        "display-name": f"{DISPLAY_NAME} {DISPLAY_VERSION}",
        "executable": rf".\{TARGET}",
        "url": f"{URL_BASE}{XYZ_VERSION}/{FILE_PREFIX}{FULL_VERSION}{FILE_SUFFIX}.zip"
    }

    return data
