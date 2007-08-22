
# http://python.org/sf/1413192
#
# This test relies on the variable names, see the bug report for details.
# The problem was that the env was deallocated prior to the txn.

import shutil
import tempfile
try:
    # For Pythons w/distutils and add-on pybsddb
    from bsddb3 import db
except ImportError:
    # For Python >= 2.3 builtin bsddb distribution
    from bsddb import db

env_name = tempfile.mkdtemp()

env = db.DBEnv()
env.open(env_name, db.DB_CREATE | db.DB_INIT_TXN | db.DB_INIT_MPOOL)
the_txn = env.txn_begin()

map = db.DB(env)
map.open('xxx.db',
         "p", db.DB_HASH, db.DB_CREATE, 0o666, txn=the_txn)

# try not to leave a turd (won't help Windows since files are still open)
try:
    shutil.rmtree(env_name)
except EnvironmentError:
    pass
