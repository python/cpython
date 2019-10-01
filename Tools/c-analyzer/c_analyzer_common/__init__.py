import os.path


PKG_ROOT = os.path.dirname(__file__)
DATA_DIR = os.path.dirname(PKG_ROOT)
REPO_ROOT = os.path.dirname(
        os.path.dirname(DATA_DIR))

SOURCE_DIRS = [os.path.join(REPO_ROOT, name) for name in [
        'Include',
        'Python',
        'Parser',
        'Objects',
        'Modules',
        ]]


# Clean up the namespace.
del os
