import os.path
import subprocess
import sys

import reindent


def n_files_str(count):
    """Return 'N file(s)' with the proper plurality on 'file'."""
    return "{} file{}".format(count, "s" if count != 1 else "")


def status(message, modal=False, info=None):
    """Decorator to output status info to stdout."""
    def decorated_fxn(fxn):
        def call_fxn(*args, **kwargs):
            sys.stdout.write(message + ' ... ')
            sys.stdout.flush()
            result = fxn(*args, **kwargs)
            if not modal and not info:
                print("done")
            elif info:
                print(info(result))
            else:
                print("yes" if result else "NO")
            return result
        return call_fxn
    return decorated_fxn


@status("Getting the list of files that have been added/changed",
            info=lambda x: n_files_str(len(x)))
def changed_files():
    """Run ``svn status`` and return a set of files that have been
    changed/added.
    """
    cmd = 'svn status --quiet --non-interactive --ignore-externals'
    svn_st = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    svn_st.wait()
    output = (x.decode().rstrip().rsplit(None, 1)[-1]
              for x in svn_st.stdout if x[0] in b'AM')
    return set(path for path in output if os.path.isfile(path))


def report_modified_files(file_paths):
    count = len(file_paths)
    if count == 0:
        return n_files_str(count)
    else:
        lines = ["{}:".format(n_files_str(count))]
        for path in file_paths:
            lines.append("  {}".format(path))
        return "\n".join(lines)


@status("Fixing whitespace", info=report_modified_files)
def normalize_whitespace(file_paths):
    """Make sure that the whitespace for .py files have been normalized."""
    reindent.makebackup = False  # No need to create backups.
    fixed = []
    for path in (x for x in file_paths if x.endswith('.py')):
        if reindent.check(path):
            fixed.append(path)
    return fixed


@status("Docs modified", modal=True)
def docs_modified(file_paths):
    """Report if any file in the Doc directory has been changed."""
    return bool(file_paths)


@status("Misc/ACKS updated", modal=True)
def credit_given(file_paths):
    """Check if Misc/ACKS has been changed."""
    return 'Misc/ACKS' in file_paths


@status("Misc/NEWS updated", modal=True)
def reported_news(file_paths):
    """Check if Misc/NEWS has been changed."""
    return 'Misc/NEWS' in file_paths


def main():
    file_paths = changed_files()
    python_files = [fn for fn in file_paths if fn.endswith('.py')]
    c_files = [fn for fn in file_paths if fn.endswith(('.c', '.h'))]
    docs = [fn for fn in file_paths if fn.startswith('Doc')]
    special_files = {'Misc/ACKS', 'Misc/NEWS'} & set(file_paths)
    # PEP 8 whitespace rules enforcement.
    normalize_whitespace(python_files)
    # Docs updated.
    docs_modified(docs)
    # Misc/ACKS changed.
    credit_given(special_files)
    # Misc/NEWS changed.
    reported_news(special_files)

    # Test suite run and passed.
    print()
    print("Did you run the test suite?")


if __name__ == '__main__':
    main()
