
# http://python.org/sf/1413192
#
# This test relies on the variable names, see the bug report for details.
# The problem was that the env was deallocated prior to the txn.

from bsddb import db

env_name = '.'

env = db.DBEnv()
env.open(env_name, db.DB_CREATE | db.DB_INIT_TXN)
the_txn = env.txn_begin()

map = db.DB(env)
map.open('xxx.db', "p", db.DB_HASH, db.DB_CREATE, 0666, txn=the_txn)
