#!/usr/bin/env python3
import argparse
import contextlib
import os
import pathlib
import subprocess
import sys
import textwrap


def main():
    idlelib_path = find_idlelib_path()
    sources_root_path = idlelib_path.parent.parent

    module_names = sorted(get_module_names(idlelib_path))

    parser = argparse.ArgumentParser(
        description='Run tests for an idlelib module '
                    'and collect coverage stats',
    )
    parser.add_argument('module', choices=['all'] + module_names)
    parser.add_argument(
        '--no-html', action='store_true',
        help='Do not create an HTML coverage report and open it in a browser')
    args = parser.parse_args()

    venv_path, venv_python_path = ensure_venv(sources_root_path)
    os.chdir(sources_root_path)

    with coveragerc_replacement():
        run_tests_with_coverage(module_name=args.module,
                                venv_python_path=venv_python_path)
        generate_coverage_report(venv_python_path=venv_python_path,
                                 no_html=args.no_html)

    if not args.no_html:
        print('Coverage report available at htmlcov/index.html')


def listfiles(path):
    return [ent.name for ent in os.scandir(path) if ent.is_file()]


def listdirs(path):
    return [ent.name for ent in os.scandir(path) if ent.is_dir()]


def find_idlelib_path():
    curpath = pathlib.Path(os.getcwd())
    if curpath.name == 'idlelib':
        idlelib_path = curpath
    elif curpath.parent.name == 'idlelib':
        idlelib_path = curpath.parent
    elif 'idlelib' in listdirs(curpath):
        idlelib_path = curpath.joinpath('idlelib')
    elif (
        'Lib' in listdirs(curpath) and
        curpath.joinpath('Lib', 'idlelib').is_dir()
    ):
        idlelib_path = curpath.joinpath('Lib', 'idlelib')
    else:
        raise Exception('Failed to find idlelib path.\n'
                        'Run in the base source dir or in Lib/idlelib.')
    return idlelib_path


def get_module_names(idlelib_path):
    test_module_names = {
        filename[len('test_'):-len('.py')]
        for filename in listfiles(idlelib_path.joinpath('idle_test'))
        if filename.startswith('test_') and filename.endswith('.py')
    }
    module_names = {
        filename[:-len('.py')]
        for filename in listfiles(idlelib_path)
        if filename.endswith('.py')
    }
    return set(test_module_names) & set(module_names)


def ensure_venv(sources_root_path):
    venv_path = sources_root_path.joinpath(
        'Lib', 'idlelib', 'idle_test', 'coverage_venv')
    venv_python_path = (
        venv_path.joinpath('Scripts', 'python.exe')
        if sys.platform == 'win32' else
        venv_path.joinpath('bin', 'python')
    )
    built_python_path = sources_root_path.joinpath(
        'python.bat' if sys.platform == 'win32' else
        'python.exe' if sys.platform == 'darwin' else
        'python'
    )
    if not venv_path.is_dir():
        subprocess.run([built_python_path, '-m', 'venv', venv_path])
        subprocess.run([venv_python_path, '-m', 'pip', 'install', 'coverage'])
    return venv_path, venv_python_path


@contextlib.contextmanager
def coveragerc_replacement():
    coveragerc_path = pathlib.Path('.coveragerc')
    orig_coverage_rc = None
    if coveragerc_path.is_file():
        with coveragerc_path.open('r', encoding='utf-8') as f:
            orig_coverage_rc = f.read()
    try:
        with coveragerc_path.open('w', encoding='utf-8') as f:
            f.write(textwrap.dedent('''\
                [run]
                branch = True
                cover_pylib = True

                [report]
                # Regexes for lines to exclude from consideration
                exclude_lines =
                    # Don't complain if non-runnable code isn't run:
                    if 0:
                    if False:
                    if __name__ == .__main__.:

                    .*# htest #
                    if not _utest:
                    if _htest:
                show_missing = True
                '''))
        yield
    finally:
        if orig_coverage_rc:
            with coveragerc_path.open('w', encoding='utf-8') as f:
                f.write(orig_coverage_rc)
        else:
            coveragerc_path.unlink()


def run_tests_with_coverage(*, module_name, venv_python_path):
    if module_name == 'all':
        subprocess.run([
            venv_python_path, '-m', 'coverage', 'run',
            '--source=idlelib',
            '-m', 'test', '-ugui', 'test_idle',
        ])
    else:
        subprocess.run([
            venv_python_path, '-m', 'coverage', 'run',
            f'--source=idlelib.{module_name}',
            f'./Lib/idlelib/idle_test/test_{module_name}.py',
        ])
        if module_name in ['pyshell', 'run']:
            # also run the tests in test_warning.py
            subprocess.run([
                venv_python_path, '-m', 'coverage', 'run', '-a',
                f'--source=idlelib.{module_name}',
                './Lib/idlelib/idle_test/test_warning.py',
            ])


def generate_coverage_report(*, venv_python_path, no_html):
    subprocess.run([venv_python_path, '-m', 'coverage', 'report'])
    if not no_html:
        subprocess.run([venv_python_path, '-m', 'coverage', 'html'])


if __name__ == '__main__':
    if sys.version_info < (3,):
        print('Running this script with Python 2 is not supported',
              file=sys.stderr)
        sys.exit(1)
    main()
