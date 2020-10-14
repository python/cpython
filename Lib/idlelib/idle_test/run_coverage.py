import argparse
import os
import pathlib
import subprocess
import sys
import textwrap


def main():
    idlelib_path = find_idlelib_path()
    sources_root_path = idlelib_path.parent.parent
    venv_path, venv_python_path = ensure_venv(sources_root_path)

    module_names = sorted(get_module_names(idlelib_path))

    parser = argparse.ArgumentParser(
        description='Run tests for an idlelib module '
                    'and collect coverage stats',
    )
    parser.add_argument('module', choices=module_names)
    args = parser.parse_args()
    module_name = args.module

    os.chdir(sources_root_path)

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
                '''))
        subprocess.run([
            venv_python_path, '-m', 'coverage',
            'run', '--pylib', f'--source=idlelib.{module_name}',
            f'./Lib/idlelib/idle_test/test_{module_name}.py',
        ])
        subprocess.run([
            venv_python_path, '-m', 'coverage',
            'report', '--show-missing',
        ])
        subprocess.run([
            venv_python_path, '-m', 'coverage',
            'html',
        ])
    finally:
        if orig_coverage_rc:
            with coveragerc_path.open('w', encoding='utf-8') as f:
                f.write(orig_coverage_rc)
        else:
            coveragerc_path.unlink()

    open_cmd = (
        'start' if sys.platform == 'win32' else
        'open' if sys.platform == 'darwin' else
        'xdg-open'
    )
    subprocess.run([
        open_cmd, f'htmlcov/Lib_idlelib_{module_name}_py.html',
    ])


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
    if not venv_path.is_dir():
        subprocess.run([sys.executable, '-m', 'venv', venv_path])
        subprocess.run([venv_python_path, '-m', 'pip', 'install', 'coverage'])
    return venv_path, venv_python_path


if __name__ == '__main__':
    main()
