#./python
"""Run Python tests with multiple installations of OpenSSL

The script

  (1) downloads OpenSSL tar bundle
  (2) extracts it to ../openssl/src/openssl-VERSION/
  (3) compiles OpenSSL
  (4) installs OpenSSL into ../openssl/VERSION/
  (5) forces a recompilation of Python modules using the
      header and library files from ../openssl/VERSION/
  (6) runs Python's test suite

The script must be run with Python's build directory as current working
directory:

    ./python Tools/ssl/test_multiple_versions.py

The script uses LD_RUN_PATH, LD_LIBRARY_PATH, CPPFLAGS and LDFLAGS to bend
search paths for header files and shared libraries. It's known to work on
Linux with GCC 4.x.

(c) 2013 Christian Heimes <christian@python.org>
"""
import logging
import os
import tarfile
import shutil
import subprocess
import sys
from urllib.request import urlopen

log = logging.getLogger("multissl")

OPENSSL_VERSIONS = [
    "0.9.7m", "0.9.8i", "0.9.8l", "0.9.8m", "0.9.8y", "1.0.0k", "1.0.1e"
]
FULL_TESTS = [
    "test_asyncio", "test_ftplib", "test_hashlib", "test_httplib",
    "test_imaplib", "test_nntplib", "test_poplib", "test_smtplib",
    "test_smtpnet", "test_urllib2_localnet", "test_venv"
]
MINIMAL_TESTS = ["test_ssl", "test_hashlib"]
CADEFAULT = True
HERE = os.path.abspath(os.getcwd())
DEST_DIR = os.path.abspath(os.path.join(HERE, os.pardir, "openssl"))


class BuildSSL:
    url_template = "https://www.openssl.org/source/openssl-{}.tar.gz"

    module_files = ["Modules/_ssl.c",
                    "Modules/socketmodule.c",
                    "Modules/_hashopenssl.c"]

    def __init__(self, version, openssl_compile_args=(), destdir=DEST_DIR):
        self._check_python_builddir()
        self.version = version
        self.openssl_compile_args = openssl_compile_args
        # installation directory
        self.install_dir = os.path.join(destdir, version)
        # source file
        self.src_file = os.path.join(destdir, "src",
                                     "openssl-{}.tar.gz".format(version))
        # build directory (removed after install)
        self.build_dir = os.path.join(destdir, "src",
                                      "openssl-{}".format(version))

    @property
    def openssl_cli(self):
        """openssl CLI binary"""
        return os.path.join(self.install_dir, "bin", "openssl")

    @property
    def openssl_version(self):
        """output of 'bin/openssl version'"""
        env = os.environ.copy()
        env["LD_LIBRARY_PATH"] = self.lib_dir
        cmd = [self.openssl_cli, "version"]
        return self._subprocess_output(cmd, env=env)

    @property
    def pyssl_version(self):
        """Value of ssl.OPENSSL_VERSION"""
        env = os.environ.copy()
        env["LD_LIBRARY_PATH"] = self.lib_dir
        cmd = ["./python", "-c", "import ssl; print(ssl.OPENSSL_VERSION)"]
        return self._subprocess_output(cmd, env=env)

    @property
    def include_dir(self):
        return os.path.join(self.install_dir, "include")

    @property
    def lib_dir(self):
        return os.path.join(self.install_dir, "lib")

    @property
    def has_openssl(self):
        return os.path.isfile(self.openssl_cli)

    @property
    def has_src(self):
        return os.path.isfile(self.src_file)

    def _subprocess_call(self, cmd, stdout=subprocess.DEVNULL, env=None,
                         **kwargs):
        log.debug("Call '%s'", " ".join(cmd))
        return subprocess.check_call(cmd, stdout=stdout, env=env, **kwargs)

    def _subprocess_output(self, cmd, env=None, **kwargs):
        log.debug("Call '%s'", " ".join(cmd))
        out = subprocess.check_output(cmd, env=env)
        return out.strip().decode("utf-8")

    def _check_python_builddir(self):
        if not os.path.isfile("python") or not os.path.isfile("setup.py"):
            raise ValueError("Script must be run in Python build directory")

    def _download_openssl(self):
        """Download OpenSSL source dist"""
        src_dir = os.path.dirname(self.src_file)
        if not os.path.isdir(src_dir):
            os.makedirs(src_dir)
        url = self.url_template.format(self.version)
        log.info("Downloading OpenSSL from {}".format(url))
        req = urlopen(url, cadefault=CADEFAULT)
        # KISS, read all, write all
        data = req.read()
        log.info("Storing {}".format(self.src_file))
        with open(self.src_file, "wb") as f:
            f.write(data)

    def _unpack_openssl(self):
        """Unpack tar.gz bundle"""
        # cleanup
        if os.path.isdir(self.build_dir):
            shutil.rmtree(self.build_dir)
        os.makedirs(self.build_dir)

        tf = tarfile.open(self.src_file)
        base = "openssl-{}/".format(self.version)
        # force extraction into build dir
        members = tf.getmembers()
        for member in members:
            if not member.name.startswith(base):
                raise ValueError(member.name)
            member.name = member.name[len(base):]
        log.info("Unpacking files to {}".format(self.build_dir))
        tf.extractall(self.build_dir, members)

    def _build_openssl(self):
        """Now build openssl"""
        log.info("Running build in {}".format(self.install_dir))
        cwd = self.build_dir
        cmd = ["./config", "shared", "--prefix={}".format(self.install_dir)]
        cmd.extend(self.openssl_compile_args)
        self._subprocess_call(cmd, cwd=cwd)
        self._subprocess_call(["make"], cwd=cwd)

    def _install_openssl(self, remove=True):
        self._subprocess_call(["make", "install"], cwd=self.build_dir)
        if remove:
            shutil.rmtree(self.build_dir)

    def install_openssl(self):
        if not self.has_openssl:
            if not self.has_src:
                self._download_openssl()
            else:
                log.debug("Already has src %s", self.src_file)
            self._unpack_openssl()
            self._build_openssl()
            self._install_openssl()
        else:
            log.info("Already has installation {}".format(self.install_dir))
        # validate installation
        version = self.openssl_version
        if self.version not in version:
            raise ValueError(version)

    def touch_pymods(self):
        # force a rebuild of all modules that use OpenSSL APIs
        for fname in self.module_files:
            os.utime(fname)

    def recompile_pymods(self):
        log.info("Using OpenSSL build from {}".format(self.build_dir))
        # overwrite header and library search paths
        env = os.environ.copy()
        env["CPPFLAGS"] = "-I{}".format(self.include_dir)
        env["LDFLAGS"] = "-L{}".format(self.lib_dir)
        # set rpath
        env["LD_RUN_PATH"] = self.lib_dir

        log.info("Rebuilding Python modules")
        self.touch_pymods()
        cmd = ["./python", "setup.py", "build"]
        self._subprocess_call(cmd, env=env)

    def check_pyssl(self):
        version = self.pyssl_version
        if self.version not in version:
            raise ValueError(version)

    def run_pytests(self, *args):
        cmd = ["./python", "-m", "test"]
        cmd.extend(args)
        self._subprocess_call(cmd, stdout=None)

    def run_python_tests(self, *args):
        self.recompile_pymods()
        self.check_pyssl()
        self.run_pytests(*args)


def main(*args):
    builders = []
    for version in OPENSSL_VERSIONS:
        if version in ("0.9.8i", "0.9.8l"):
            openssl_compile_args = ("no-asm",)
        else:
            openssl_compile_args = ()
        builder = BuildSSL(version, openssl_compile_args)
        builder.install_openssl()
        builders.append(builder)

    for builder in builders:
        builder.run_python_tests(*args)
    # final touch
    builder.touch_pymods()


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO,
                        format="*** %(levelname)s %(message)s")
    args = sys.argv[1:]
    if not args:
        args = ["-unetwork", "-v"]
        args.extend(FULL_TESTS)
    main(*args)
