#!/bin/sh

/Library/Frameworks/Python.framework/Versions/@PYVER@/bin/python@PYVER@ << "EOF"

# install_certifi.py
#
# sample script to install or update a set of default Root Certificates
# for the ssl module.  Uses the certificates provided by the certifi package:
#       https://pypi.org/project/certifi/

import os
import os.path
import platform
import ssl
import stat
import subprocess
import sys

STAT_0o775 = (
    stat.S_IRUSR
    | stat.S_IWUSR
    | stat.S_IXUSR
    | stat.S_IRGRP
    | stat.S_IWGRP
    | stat.S_IXGRP
    | stat.S_IROTH
    | stat.S_IXOTH
)


def main():
    pip_call = [sys.executable, "-E", "-s", "-m", "pip"]
    macos_release = tuple([int(n) for n in platform.mac_ver()[0].split(".")[0:2]])
    old_macos = macos_release < (10, 13)
    if old_macos:
        pip_version_string = subprocess.check_output(pip_call + ["-V"]).decode().strip()
        # Silence warning to user to upgrade pip
        pip_call.append("--disable-pip-version-check")
        pip_version = tuple(
            [int(n) for n in pip_version_string.split()[1].split(".")[0:2]]
        )
        if pip_version >= (24, 2):
            print(
                f" -- WARNING: this version of pip may not work on this older version of macOS.\n"
                f"      found {pip_version_string}\n"
                f"      (See https://github.com/pypa/pip/issues/12901 for more information.)\n"
                f"      Attempting to revert to an older version of pip.\n"
                f" -- pip install --use-deprecated=legacy-certs pip==24.1.2\n"
            )
            subprocess.check_call(
                pip_call + ["install", "--use-deprecated=legacy-certs", "pip==24.1.2"]
            )

    openssl_dir, openssl_cafile = os.path.split(
        ssl.get_default_verify_paths().openssl_cafile
    )
    print(" -- pip install --upgrade certifi")
    subprocess.check_call(pip_call + ["install", "--upgrade", "certifi"])

    import certifi

    # change working directory to the default SSL directory
    os.chdir(openssl_dir)
    relpath_to_certifi_cafile = os.path.relpath(certifi.where())
    print(" -- removing any existing file or link")
    try:
        os.remove(openssl_cafile)
    except FileNotFoundError:
        pass
    print(" -- creating symlink to certifi certificate bundle")
    os.symlink(relpath_to_certifi_cafile, openssl_cafile)
    print(" -- setting permissions")
    os.chmod(openssl_cafile, STAT_0o775)
    print(" -- update complete")
    if old_macos:
        print(
            f" -- WARNING: Future releases of this Python installer may not support this older macOS version.\n"
        )


if __name__ == "__main__":
    try:
        main()
    except subprocess.SubprocessError:
        print(" -- WARNING: Install Certificates failed")
        sys.exit(1)
EOF
