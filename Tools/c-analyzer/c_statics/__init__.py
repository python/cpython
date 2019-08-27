import os.path


PKG_ROOT = os.path.dirname(__file__)
DATA_DIR = os.path.dirname(PKG_ROOT)
REPO_ROOT = os.path.dirname(
        os.path.dirname(DATA_DIR))

KNOWN_FILE = os.path.join(DATA_DIR, 'known.tsv')
IGNORED_FILE = os.path.join(DATA_DIR, 'ignored.tsv')

SOURCE_DIRS = [os.path.join(REPO_ROOT, name) for name in [
        'Include',
        'Python',
        'Parser',
        'Objects',
        'Modules',
        ]]


# Clean up the namespace.
del os
