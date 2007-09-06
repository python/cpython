# http://bugs.python.org/issue1413192
#
# See the bug report for details.
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

# Wrap test operation in a class so we can control destruction rather than
# waiting for the controlling Python executable to exit

class Context:

    def __init__(self):
        self.env = db.DBEnv()
        self.env.open(env_name,
                      db.DB_CREATE | db.DB_INIT_TXN | db.DB_INIT_MPOOL)
        self.the_txn = self.env.txn_begin()

        self.map = db.DB(self.env)
        self.map.open('xxx.db', "p",
                      db.DB_HASH, db.DB_CREATE, 0o666, txn=self.the_txn)
        del self.env
        del self.the_txn


context = Context()
del context

# try not to leave a turd
try:
    shutil.rmtree(env_name)
except EnvironmentError:
    pass
