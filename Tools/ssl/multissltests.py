#!./python
"""Run Python tests against multiple installations of OpenSSL and LibreSSL

The script

  (1) downloads OpenSSL / LibreSSL tar bundle
  (2) extracts it to ./src
  (3) compiles OpenSSL / LibreSSL
  (4) installs OpenSSL / LibreSSL into ../multissl/$LIB/$VERSION/
  (5) forces a recompilation of Python modules using the
      header and library files from ../multissl/$LIB/$VERSION/
  (6) runs Python's test suite

The script must be run with Python's build directory as current working
directory.

The script uses LD_RUN_PATH, LD_LIBRARY_PATH, CPPFLAGS and LDFLAGS to bend
search paths for header files and shared libraries. It's known to work on
Linux with GCC and clang.

Please keep this script compatible with Python 2.7, and 3.4 to 3.7.

(c) 2013-2017 Christian Heimes <christian@python.org>
"""
from __future__ import print_function

import argparse
from datetime import datetime
import logging
import os
try:
    from urllib.request import urlopen
    from urllib.error import HTTPError
except ImportError:
    from urllib2 import urlopen, HTTPError
import re
import shutil
import string
import subprocess
import sys
import tarfile


log = logging.getLogger("multissl")

OPENSSL_OLD_VERSIONS = [
]

OPENSSL_RECENT_VERSIONS = [
    "1.1.1w",
    "3.0.15",
    "3.1.7",
    "3.2.3",
]

LIBRESSL_OLD_VERSIONS = [
]

LIBRESSL_RECENT_VERSIONS = [
]

# store files in ../multissl
HERE = os.path.dirname(os.path.abspath(__file__))
PYTHONROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
MULTISSL_DIR = os.path.abspath(os.path.join(PYTHONROOT, '..', 'multissl'))


parser = argparse.ArgumentParser(
    prog='multissl',
    description=(
        "Run CPython tests with multiple OpenSSL and LibreSSL "
        "versions."
    )
)
parser.add_argument(
    '--debug',
    action='store_true',
    help="Enable debug logging",
)
parser.add_argument(
    '--disable-ancient',
    action='store_true',
    help="Don't test OpenSSL and LibreSSL versions without upstream support",
)
parser.add_argument(
    '--openssl',
    nargs='+',
    default=(),
    help=(
        "OpenSSL versions, defaults to '{}' (ancient: '{}') if no "
        "OpenSSL and LibreSSL versions are given."
    ).format(OPENSSL_RECENT_VERSIONS, OPENSSL_OLD_VERSIONS)
)
parser.add_argument(
    '--libressl',
    nargs='+',
    default=(),
    help=(
        "LibreSSL versions, defaults to '{}' (ancient: '{}') if no "
        "OpenSSL and LibreSSL versions are given."
    ).format(LIBRESSL_RECENT_VERSIONS, LIBRESSL_OLD_VERSIONS)
)
parser.add_argument(
    '--tests',
    nargs='*',
    default=(),
    help="Python tests to run, defaults to all SSL related tests.",
)
parser.add_argument(
    '--base-directory',
    default=MULTISSL_DIR,
    help="Base directory for OpenSSL / LibreSSL sources and builds."
)
parser.add_argument(
    '--no-network',
    action='store_false',
    dest='network',
    help="Disable network tests."
)
parser.add_argument(
    '--steps',
    choices=['library', 'modules', 'tests'],
    default='tests',
    help=(
        "Which steps to perform. 'library' downloads and compiles OpenSSL "
        "or LibreSSL. 'module' also compiles Python modules. 'tests' builds "
        "all and runs the test suite."
    )
)
parser.add_argument(
    '--system',
    default='',
    help="Override the automatic system type detection."
)
parser.add_argument(
    '--force',
    action='store_true',
    dest='force',
    help="Force build and installation."
)
parser.add_argument(
    '--keep-sources',
    action='store_true',
    dest='keep_sources',
    help="Keep original sources for debugging."
)


class AbstractBuilder(object):
    library = None
    url_templates = None
    src_template = None
    build_template = None
    depend_target = None
    install_target = 'install'
    jobs = os.cpu_count()

    module_files = (
        os.path.join(PYTHONROOT, "Modules/_ssl.c"),
        os.path.join(PYTHONROOT, "Modules/_hashopenssl.c"),
    )
    module_libs = ("_ssl", "_hashlib")

    def __init__(self, version, args):
        self.version = version
        self.args = args
        # installation directory
        self.install_dir = os.path.join(
            os.path.join(args.base_directory, self.library.lower()), version
        )
        # source file
        self.src_dir = os.path.join(args.base_directory, 'src')
        self.src_file = os.path.join(
            self.src_dir, self.src_template.format(version))
        # build directory (removed after install)
        self.build_dir = os.path.join(
            self.src_dir, self.build_template.format(version))
        self.system = args.system

    def __str__(self):
        return "<{0.__class__.__name__} for {0.version}>".format(self)

    def __eq__(self, other):
        if not isinstance(other, AbstractBuilder):
            return NotImplemented
        return (
            self.library == other.library
            and self.version == other.version
        )

    def __hash__(self):
        return hash((self.library, self.version))

    @property
    def short_version(self):
        """Short version for OpenSSL download URL"""
        return None

    @property
    def openssl_cli(self):
        """openssl CLI binary"""
        return os.path.join(self.install_dir, "bin", "openssl")

    @property
    def openssl_version(self):
        """output of 'bin/openssl version'"""
        cmd = [self.openssl_cli, "version"]
        return self._subprocess_output(cmd)

    @property
    def pyssl_version(self):
        """Value of ssl.OPENSSL_VERSION"""
        cmd = [
            sys.executable,
            '-c', 'import ssl; print(ssl.OPENSSL_VERSION)'
        ]
        return self._subprocess_output(cmd)

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

    def _subprocess_call(self, cmd, env=None, **kwargs):
        log.debug("Call '{}'".format(" ".join(cmd)))
        return subprocess.check_call(cmd, env=env, **kwargs)

    def _subprocess_output(self, cmd, env=None, **kwargs):
        log.debug("Call '{}'".format(" ".join(cmd)))
        if env is None:
            env = os.environ.copy()
            env["LD_LIBRARY_PATH"] = self.lib_dir
        out = subprocess.check_output(cmd, env=env, **kwargs)
        return out.strip().decode("utf-8")

    def _download_src(self):
        """Download sources"""
        src_dir = os.path.dirname(self.src_file)
        if not os.path.isdir(src_dir):
            os.makedirs(src_dir)
        data = None
        for url_template in self.url_templates:
            url = url_template.format(v=self.version, s=self.short_version)
            log.info("Downloading from {}".format(url))
            try:
                req = urlopen(url)
                # KISS, read all, write all
                data = req.read()
            except HTTPError as e:
                log.error(
                    "Download from {} has from failed: {}".format(url, e)
                )
            else:
                log.info("Successfully downloaded from {}".format(url))
                break
        if data is None:
            raise ValueError("All download URLs have failed")
        log.info("Storing {}".format(self.src_file))
        with open(self.src_file, "wb") as f:
            f.write(data)

    def _unpack_src(self):
        """Unpack tar.gz bundle"""
        # cleanup
        if os.path.isdir(self.build_dir):
            shutil.rmtree(self.build_dir)
        os.makedirs(self.build_dir)

        tf = tarfile.open(self.src_file)
        name = self.build_template.format(self.version)
        base = name + '/'
        # force extraction into build dir
        members = tf.getmembers()
        for member in list(members):
            if member.name == name:
                members.remove(member)
            elif not member.name.startswith(base):
                raise ValueError(member.name, base)
            member.name = member.name[len(base):].lstrip('/')
        log.info("Unpacking files to {}".format(self.build_dir))
        tf.extractall(self.build_dir, members)

    def _build_src(self, config_args=()):
        """Now build openssl"""
        log.info("Running build in {}".format(self.build_dir))
        cwd = self.build_dir
        cmd = [
            "./config", *config_args,
            "shared", "--debug",
            "--prefix={}".format(self.install_dir)
        ]
        # cmd.extend(["no-deprecated", "--api=1.1.0"])
        env = os.environ.copy()
        # set rpath
        env["LD_RUN_PATH"] = self.lib_dir
        if self.system:
            env['SYSTEM'] = self.system
        self._subprocess_call(cmd, cwd=cwd, env=env)
        if self.depend_target:
            self._subprocess_call(
                ["make", "-j1", self.depend_target], cwd=cwd, env=env
            )
        self._subprocess_call(["make", f"-j{self.jobs}"], cwd=cwd, env=env)

    def _make_install(self):
        self._subprocess_call(
            ["make", "-j1", self.install_target],
            cwd=self.build_dir
        )
        self._post_install()
        if not self.args.keep_sources:
            shutil.rmtree(self.build_dir)

    def _post_install(self):
        pass

    def install(self):
        log.info(self.openssl_cli)
        if not self.has_openssl or self.args.force:
            if not self.has_src:
                self._download_src()
            else:
                log.debug("Already has src {}".format(self.src_file))
            self._unpack_src()
            self._build_src()
            self._make_install()
        else:
            log.info("Already has installation {}".format(self.install_dir))
        # validate installation
        version = self.openssl_version
        if self.version not in version:
            raise ValueError(version)

    def recompile_pymods(self):
        log.warning("Using build from {}".format(self.build_dir))
        # force a rebuild of all modules that use OpenSSL APIs
        for fname in self.module_files:
            os.utime(fname, None)
        # remove all build artefacts
        for root, dirs, files in os.walk('build'):
            for filename in files:
                if filename.startswith(self.module_libs):
                    os.unlink(os.path.join(root, filename))

        # overwrite header and library search paths
        env = os.environ.copy()
        env["CPPFLAGS"] = "-I{}".format(self.include_dir)
        env["LDFLAGS"] = "-L{}".format(self.lib_dir)
        # set rpath
        env["LD_RUN_PATH"] = self.lib_dir

        log.info("Rebuilding Python modules")
        cmd = [sys.executable, os.path.join(PYTHONROOT, "setup.py"), "build"]
        self._subprocess_call(cmd, env=env)
        self.check_imports()

    def check_imports(self):
        cmd = [sys.executable, "-c", "import _ssl; import _hashlib"]
        self._subprocess_call(cmd)

    def check_pyssl(self):
        version = self.pyssl_version
        if self.version not in version:
            raise ValueError(version)

    def run_python_tests(self, tests, network=True):
        if not tests:
            cmd = [
                sys.executable,
                os.path.join(PYTHONROOT, 'Lib/test/ssltests.py'),
                '-j0'
            ]
        elif sys.version_info < (3, 3):
            cmd = [sys.executable, '-m', 'test.regrtest']
        else:
            cmd = [sys.executable, '-m', 'test', '-j0']
        if network:
            cmd.extend(['-u', 'network', '-u', 'urlfetch'])
        cmd.extend(['-w', '-r'])
        cmd.extend(tests)
        self._subprocess_call(cmd, stdout=None)


class BuildOpenSSL(AbstractBuilder):
    library = "OpenSSL"
    url_templates = (
        "https://github.com/openssl/openssl/releases/download/openssl-{v}/openssl-{v}.tar.gz",
        "https://www.openssl.org/source/openssl-{v}.tar.gz",
        "https://www.openssl.org/source/old/{s}/openssl-{v}.tar.gz"
    )
    src_template = "openssl-{}.tar.gz"
    build_template = "openssl-{}"
    # only install software, skip docs
    install_target = 'install_sw'
    depend_target = 'depend'

    def _post_install(self):
        if self.version.startswith("3."):
            self._post_install_3xx()

    def _build_src(self, config_args=()):
        if self.version.startswith("3."):
            config_args += ("enable-fips",)
        super()._build_src(config_args)

    def _post_install_3xx(self):
        # create ssl/ subdir with example configs
        # Install FIPS module
        self._subprocess_call(
            ["make", "-j1", "install_ssldirs", "install_fips"],
            cwd=self.build_dir
        )
        if not os.path.isdir(self.lib_dir):
            # 3.0.0-beta2 uses lib64 on 64 bit platforms
            lib64 = self.lib_dir + "64"
            os.symlink(lib64, self.lib_dir)

    @property
    def short_version(self):
        """Short version for OpenSSL download URL"""
        mo = re.search(r"^(\d+)\.(\d+)\.(\d+)", self.version)
        parsed = tuple(int(m) for m in mo.groups())
        if parsed < (1, 0, 0):
            return "0.9.x"
        if parsed >= (3, 0, 0):
            # OpenSSL 3.0.0 -> /old/3.0/
            parsed = parsed[:2]
        return ".".join(str(i) for i in parsed)


class BuildLibreSSL(AbstractBuilder):
    library = "LibreSSL"
    url_templates = (
        "https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-{v}.tar.gz",
    )
    src_template = "libressl-{}.tar.gz"
    build_template = "libressl-{}"


def configure_make():
    if not os.path.isfile('Makefile'):
        log.info('Running ./configure')
        subprocess.check_call([
            './configure', '--config-cache', '--quiet',
            '--with-pydebug'
        ])

    log.info('Running make')
    subprocess.check_call(['make', '--quiet'])


def main():
    args = parser.parse_args()
    if not args.openssl and not args.libressl:
        args.openssl = list(OPENSSL_RECENT_VERSIONS)
        args.libressl = list(LIBRESSL_RECENT_VERSIONS)
        if not args.disable_ancient:
            args.openssl.extend(OPENSSL_OLD_VERSIONS)
            args.libressl.extend(LIBRESSL_OLD_VERSIONS)

    logging.basicConfig(
        level=logging.DEBUG if args.debug else logging.INFO,
        format="*** %(levelname)s %(message)s"
    )

    start = datetime.now()

    if args.steps in {'modules', 'tests'}:
        for name in ['setup.py', 'Modules/_ssl.c']:
            if not os.path.isfile(os.path.join(PYTHONROOT, name)):
                parser.error(
                    "Must be executed from CPython build dir"
                )
        if not os.path.samefile('python', sys.executable):
            parser.error(
                "Must be executed with ./python from CPython build dir"
            )
        # check for configure and run make
        configure_make()

    # download and register builder
    builds = []

    for version in args.openssl:
        build = BuildOpenSSL(
            version,
            args
        )
        build.install()
        builds.append(build)

    for version in args.libressl:
        build = BuildLibreSSL(
            version,
            args
        )
        build.install()
        builds.append(build)

    if args.steps in {'modules', 'tests'}:
        for build in builds:
            try:
                build.recompile_pymods()
                build.check_pyssl()
                if args.steps == 'tests':
                    build.run_python_tests(
                        tests=args.tests,
                        network=args.network,
                    )
            except Exception as e:
                log.exception("%s failed", build)
                print("{} failed: {}".format(build, e), file=sys.stderr)
                sys.exit(2)

    log.info("\n{} finished in {}".format(
            args.steps.capitalize(),
            datetime.now() - start
        ))
    print('Python: ', sys.version)
    if args.steps == 'tests':
        if args.tests:
            print('Executed Tests:', ' '.join(args.tests))
        else:
            print('Executed all SSL tests.')

    print('OpenSSL / LibreSSL versions:')
    for build in builds:
        print("    * {0.library} {0.version}".format(build))


if __name__ == "__main__":
    main()
