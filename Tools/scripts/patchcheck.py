import os.path
import subprocess
import sys

import reindent


def status(message, modal=False, info=None):
    """Decorator to output status info to stdout."""
    def decorated_fxn(fxn):
        def call_fxn(*args, **kwargs):
            sys.stdout.write(message + ' ... ')
            sys.stdout.flush()
            result = fxn(*args, **kwargs)
            if not modal and not info:
                print "done"
            elif info:
                print info(result)
            else:
                if result:
                    print "yes"
                else:
                    print "NO"
            return result
        return call_fxn
    return decorated_fxn

@status("Getting the list of files that have been added/changed",
            info=lambda x: "%s files" % len(x))
def changed_files():
    """Run ``svn status`` and return a set of files that have been
    changed/added."""
    cmd = 'svn status --quiet --non-interactive --ignore-externals'
    svn_st = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    svn_st.wait()
    output = [line.strip() for line in svn_st.stdout.readlines()]
    files = set()
    for line in output:
        if not line[0] in ('A', 'M'):
            continue
        line_parts = line.split()
        path = line_parts[-1]
        if os.path.isfile(path):
            files.add(path)
    return files

@status("Fixing whitespace", info=lambda x: "%s files" % x)
def normalize_whitespace(file_paths):
    """Make sure that the whitespace for .py files have been normalized."""
    reindent.makebackup = False  # No need to create backups.
    result = map(reindent.check, (x for x in file_paths if x.endswith('.py')))
    return sum(result)

@status("Docs modified", modal=True)
def docs_modified(file_paths):
    """Report if any files in the Docs directory."""
    for path in file_paths:
        if path.startswith("Doc"):
            return True
    return False

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
    # PEP 7/8 verification.
    normalize_whitespace(file_paths)
    # Docs updated.
    docs_modified(file_paths)
    # Misc/ACKS changed.
    credit_given(file_paths)
    # Misc/NEWS changed.
    reported_news(file_paths)

    # Test suite run and passed.
    print
    print "Did you run the test suite?"


if __name__ == '__main__':
    main()
