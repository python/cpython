#-----------------------------------------------------------------------
#
# Copyright (C) 2000, 2001 by Autonomous Zone Industries
# Copyright (C) 2002 Gregory P. Smith
#
# License:      This is free software.  You may use this software for any
#               purpose including modification/redistribution, so long as
#               this header remains intact and that you do not claim any
#               rights of ownership or authorship of this software.  This
#               software has been tested, but no warranty is expressed or
#               implied.
#
#   --  Gregory P. Smith <greg@electricrain.com>

# This provides a simple database table interface built on top of
# the Python BerkeleyDB 3 interface.
#
_cvsid = '$Id$'

import re
import sys
import copy
import xdrlib
import random
import pickle

from bsddb.db import *

# All table names, row names etc. must be ASCII strings
# However, rowids, when represented as strings, are latin-1 encoded
def _E(s):
    return s.encode("ascii")

# XXX(nnorwitz): is this correct? DBIncompleteError is conditional in _bsddb.c
try:
    DBIncompleteError
except NameError:
    class DBIncompleteError(Exception):
        pass

class TableDBError(Exception):
    pass
class TableAlreadyExists(TableDBError):
    pass


class Cond:
    """This condition matches everything"""
    def __call__(self, s):
        return 1

class ExactCond(Cond):
    """Acts as an exact match condition function"""
    def __init__(self, strtomatch, encoding="utf-8"):
        self.strtomatch = strtomatch.encode(encoding)
    def __call__(self, s):
        return s == self.strtomatch

class PrefixCond(Cond):
    """Acts as a condition function for matching a string prefix"""
    def __init__(self, prefix, encoding="utf-8"):
        self.prefix = prefix.encode(encoding)
    def __call__(self, s):
        return s[:len(self.prefix)] == self.prefix

class PostfixCond(Cond):
    """Acts as a condition function for matching a string postfix"""
    def __init__(self, postfix, encoding="utf-8"):
        self.postfix = postfix.encode(encoding)
    def __call__(self, s):
        return s[-len(self.postfix):] == self.postfix

class LikeCond(Cond):
    """
    Acts as a function that will match using an SQL 'LIKE' style
    string.  Case insensitive and % signs are wild cards.
    This isn't perfect but it should work for the simple common cases.
    """
    def __init__(self, likestr, re_flags=re.IGNORECASE, encoding="utf-8"):
        # escape python re characters
        chars_to_escape = '.*+()[]?'
        for char in chars_to_escape :
            likestr = likestr.replace(char, '\\'+char)
        # convert %s to wildcards
        self.likestr = likestr.replace('%', '.*')
        self.re = re.compile('^'+self.likestr+'$', re_flags)
        self.encoding = encoding
    def __call__(self, s):
        return self.re.match(s.decode(self.encoding))

#
# keys used to store database metadata
#
_table_names_key = '__TABLE_NAMES__'  # list of the tables in this db
_columns = '._COLUMNS__'  # table_name+this key contains a list of columns

def _columns_key(table):
    return _E(table + _columns)

#
# these keys are found within table sub databases
#
_data =  '._DATA_.'  # this+column+this+rowid key contains table data
_rowid = '._ROWID_.' # this+rowid+this key contains a unique entry for each
                     # row in the table.  (no data is stored)
_rowid_str_len = 8   # length in bytes of the unique rowid strings

def _data_key(table, col, rowid):
    return _E(table + _data + col + _data) + rowid

def _search_col_data_key(table, col):
    return _E(table + _data + col + _data)

def _search_all_data_key(table):
    return _E(table + _data)

def _rowid_key(table, rowid):
    return _E(table + _rowid) + rowid + _E(_rowid)

def _search_rowid_key(table):
    return _E(table + _rowid)

def contains_metastrings(s) :
    """Verify that the given string does not contain any
    metadata strings that might interfere with dbtables database operation.
    """
    if (s.find(_table_names_key) >= 0 or
        s.find(_columns) >= 0 or
        s.find(_data) >= 0 or
        s.find(_rowid) >= 0):
        # Then
        return 1
    else:
        return 0


class bsdTableDB :
    def __init__(self, filename, dbhome, create=0, truncate=0, mode=0o600,
                 recover=0, dbflags=0):
        """bsdTableDB(filename, dbhome, create=0, truncate=0, mode=0o600)

        Open database name in the dbhome BerkeleyDB directory.
        Use keyword arguments when calling this constructor.
        """
        self.db = None
        myflags = DB_THREAD
        if create:
            myflags |= DB_CREATE
        flagsforenv = (DB_INIT_MPOOL | DB_INIT_LOCK | DB_INIT_LOG |
                       DB_INIT_TXN | dbflags)
        # DB_AUTO_COMMIT isn't a valid flag for env.open()
        try:
            dbflags |= DB_AUTO_COMMIT
        except AttributeError:
            pass
        if recover:
            flagsforenv = flagsforenv | DB_RECOVER
        self.env = DBEnv()
        # enable auto deadlock avoidance
        self.env.set_lk_detect(DB_LOCK_DEFAULT)
        self.env.open(dbhome, myflags | flagsforenv)
        if truncate:
            myflags |= DB_TRUNCATE
        self.db = DB(self.env)
        # this code relies on DBCursor.set* methods to raise exceptions
        # rather than returning None
        self.db.set_get_returns_none(1)
        # allow duplicate entries [warning: be careful w/ metadata]
        self.db.set_flags(DB_DUP)
        self.db.open(filename, DB_BTREE, dbflags | myflags, mode)
        self.dbfilename = filename
        # Initialize the table names list if this is a new database
        txn = self.env.txn_begin()
        try:
            if not self.db.has_key(_E(_table_names_key), txn):
                self.db.put(_E(_table_names_key), pickle.dumps([], 1), txn=txn)
        # Yes, bare except
        except:
            txn.abort()
            raise
        else:
            txn.commit()
        # TODO verify more of the database's metadata?
        self.__tablecolumns = {}

    def __del__(self):
        self.close()

    def close(self):
        if self.db is not None:
            self.db.close()
            self.db = None
        if self.env is not None:
            self.env.close()
            self.env = None

    def checkpoint(self, mins=0):
        try:
            self.env.txn_checkpoint(mins)
        except DBIncompleteError:
            pass

    def sync(self):
        try:
            self.db.sync()
        except DBIncompleteError:
            pass

    def _db_print(self) :
        """Print the database to stdout for debugging"""
        print("******** Printing raw database for debugging ********")
        cur = self.db.cursor()
        try:
            key, data = cur.first()
            while 1:
                print(repr({key: data}))
                next = cur.next()
                if next:
                    key, data = next
                else:
                    cur.close()
                    return
        except DBNotFoundError:
            cur.close()


    def CreateTable(self, table, columns):
        """CreateTable(table, columns) - Create a new table in the database.

        raises TableDBError if it already exists or for other DB errors.
        """
        assert isinstance(columns, list)
        txn = None
        try:
            # checking sanity of the table and column names here on
            # table creation will prevent problems elsewhere.
            if contains_metastrings(table):
                raise ValueError(
                    "bad table name: contains reserved metastrings")
            for column in columns :
                if contains_metastrings(column):
                    raise ValueError(
                        "bad column name: contains reserved metastrings")

            columnlist_key = _columns_key(table)
            if self.db.has_key(columnlist_key):
                raise TableAlreadyExists, "table already exists"

            txn = self.env.txn_begin()
            # store the table's column info
            self.db.put(columnlist_key, pickle.dumps(columns, 1), txn=txn)

            # add the table name to the tablelist
            tablelist = pickle.loads(self.db.get(_E(_table_names_key), txn=txn,
                                                 flags=DB_RMW))
            tablelist.append(table)
            # delete 1st, in case we opened with DB_DUP
            self.db.delete(_E(_table_names_key), txn)
            self.db.put(_E(_table_names_key), pickle.dumps(tablelist, 1), txn=txn)

            txn.commit()
            txn = None
        except DBError as dberror:
            raise TableDBError, dberror.args[1]
        finally:
            if txn:
                txn.abort()
                txn = None

    def ListTableColumns(self, table):
        """Return a list of columns in the given table.
        [] if the table doesn't exist.
        """
        assert isinstance(table, str)
        if contains_metastrings(table):
            raise ValueError, "bad table name: contains reserved metastrings"

        columnlist_key = _columns_key(table)
        if not self.db.has_key(columnlist_key):
            return []
        pickledcolumnlist = self.db.get(columnlist_key)
        if pickledcolumnlist:
            return pickle.loads(pickledcolumnlist)
        else:
            return []

    def ListTables(self):
        """Return a list of tables in this database."""
        pickledtablelist = self.db.get(_E(_table_names_key))
        if pickledtablelist:
            return pickle.loads(pickledtablelist)
        else:
            return []

    def CreateOrExtendTable(self, table, columns):
        """CreateOrExtendTable(table, columns)

        Create a new table in the database.

        If a table of this name already exists, extend it to have any
        additional columns present in the given list as well as
        all of its current columns.
        """
        assert isinstance(columns, list)
        try:
            self.CreateTable(table, columns)
        except TableAlreadyExists:
            # the table already existed, add any new columns
            txn = None
            try:
                columnlist_key = _columns_key(table)
                txn = self.env.txn_begin()

                # load the current column list
                oldcolumnlist = pickle.loads(
                    self.db.get(columnlist_key, txn=txn, flags=DB_RMW))
                # create a hash table for fast lookups of column names in the
                # loop below
                oldcolumnhash = {}
                for c in oldcolumnlist:
                    oldcolumnhash[c] = c

                # create a new column list containing both the old and new
                # column names
                newcolumnlist = copy.copy(oldcolumnlist)
                for c in columns:
                    if c not in oldcolumnhash:
                        newcolumnlist.append(c)

                # store the table's new extended column list
                if newcolumnlist != oldcolumnlist :
                    # delete the old one first since we opened with DB_DUP
                    self.db.delete(columnlist_key, txn)
                    self.db.put(columnlist_key,
                                pickle.dumps(newcolumnlist, 1),
                                txn=txn)

                txn.commit()
                txn = None

                self.__load_column_info(table)
            except DBError as dberror:
                raise TableDBError, dberror.args[1]
            finally:
                if txn:
                    txn.abort()


    def __load_column_info(self, table) :
        """initialize the self.__tablecolumns dict"""
        # check the column names
        try:
            tcolpickles = self.db.get(_columns_key(table))
        except DBNotFoundError:
            raise TableDBError, "unknown table: %r" % (table,)
        if not tcolpickles:
            raise TableDBError, "unknown table: %r" % (table,)
        self.__tablecolumns[table] = pickle.loads(tcolpickles)

    def __new_rowid(self, table, txn) :
        """Create a new unique row identifier"""
        unique = 0
        while not unique:
            # Generate a random 64-bit row ID string
            # (note: this code has <64 bits of randomness
            # but it's plenty for our database id needs!)
            p = xdrlib.Packer()
            p.pack_int(int(random.random()*2147483647))
            p.pack_int(int(random.random()*2147483647))
            newid = p.get_buffer()

            # Guarantee uniqueness by adding this key to the database
            try:
                self.db.put(_rowid_key(table, newid), None, txn=txn,
                            flags=DB_NOOVERWRITE)
            except DBKeyExistError:
                pass
            else:
                unique = 1

        return newid


    def Insert(self, table, rowdict) :
        """Insert(table, datadict) - Insert a new row into the table
        using the keys+values from rowdict as the column values.
        """
        txn = None
        try:
            if not self.db.has_key(_columns_key(table)):
                raise TableDBError, "unknown table"

            # check the validity of each column name
            if table not in self.__tablecolumns:
                self.__load_column_info(table)
            for column in rowdict.keys() :
                if not self.__tablecolumns[table].count(column):
                    raise TableDBError, "unknown column: %r" % (column,)

            # get a unique row identifier for this row
            txn = self.env.txn_begin()
            rowid = self.__new_rowid(table, txn=txn)

            # insert the row values into the table database
            for column, dataitem in rowdict.items():
                # store the value
                self.db.put(_data_key(table, column, rowid), dataitem, txn=txn)

            txn.commit()
            txn = None

        except DBError as dberror:
            # WIBNI we could just abort the txn and re-raise the exception?
            # But no, because TableDBError is not related to DBError via
            # inheritance, so it would be backwards incompatible.  Do the next
            # best thing.
            info = sys.exc_info()
            if txn:
                txn.abort()
                self.db.delete(_rowid_key(table, rowid))
                txn = None
            raise TableDBError, dberror.args[1], info[2]
        finally:
            if txn:
                txn.abort()


    def Modify(self, table, conditions={}, mappings={}):
        """Modify(table, conditions={}, mappings={}) - Modify items in rows matching 'conditions' using mapping functions in 'mappings'

        * table - the table name
        * conditions - a dictionary keyed on column names containing
          a condition callable expecting the data string as an
          argument and returning a boolean.
        * mappings - a dictionary keyed on column names containing a
          condition callable expecting the data string as an argument and
          returning the new string for that column.
        """
        try:
            matching_rowids = self.__Select(table, [], conditions)

            # modify only requested columns
            columns = mappings.keys()
            for rowid in matching_rowids.keys():
                rowid = rowid.encode("latin-1")
                txn = None
                try:
                    for column in columns:
                        txn = self.env.txn_begin()
                        # modify the requested column
                        try:
                            dataitem = self.db.get(
                                _data_key(table, column, rowid),
                                txn)
                            self.db.delete(
                                _data_key(table, column, rowid),
                                txn)
                        except DBNotFoundError:
                             # XXXXXXX row key somehow didn't exist, assume no
                             # error
                            dataitem = None
                        dataitem = mappings[column](dataitem)
                        if dataitem != None:
                            self.db.put(
                                _data_key(table, column, rowid),
                                dataitem, txn=txn)
                        txn.commit()
                        txn = None

                # catch all exceptions here since we call unknown callables
                finally:
                    if txn:
                        txn.abort()

        except DBError as dberror:
            raise TableDBError, dberror.args[1]

    def Delete(self, table, conditions={}):
        """Delete(table, conditions) - Delete items matching the given
        conditions from the table.

        * conditions - a dictionary keyed on column names containing
          condition functions expecting the data string as an
          argument and returning a boolean.
        """
        try:
            matching_rowids = self.__Select(table, [], conditions)

            # delete row data from all columns
            columns = self.__tablecolumns[table]
            for rowid in matching_rowids.keys():
                txn = None
                try:
                    txn = self.env.txn_begin()
                    for column in columns:
                        # delete the data key
                        try:
                            self.db.delete(_data_key(table, column,
                                                     rowid.encode("latin-1")),
                                           txn)
                        except DBNotFoundError:
                            # XXXXXXX column may not exist, assume no error
                            pass

                    try:
                        self.db.delete(_rowid_key(table, rowid.encode("latin-1")), txn)
                    except DBNotFoundError:
                        # XXXXXXX row key somehow didn't exist, assume no error
                        pass
                    txn.commit()
                    txn = None
                finally:
                    if txn:
                        txn.abort()
        except DBError as dberror:
            raise TableDBError, dberror.args[1]


    def Select(self, table, columns, conditions={}):
        """Select(table, columns, conditions) - retrieve specific row data
        Returns a list of row column->value mapping dictionaries.

        * columns - a list of which column data to return.  If
          columns is None, all columns will be returned.
        * conditions - a dictionary keyed on column names
          containing callable conditions expecting the data string as an
          argument and returning a boolean.
        """
        try:
            if table not in self.__tablecolumns:
                self.__load_column_info(table)
            if columns is None:
                columns = self.__tablecolumns[table]
            matching_rowids = self.__Select(table, columns, conditions)
        except DBError as dberror:
            raise TableDBError, dberror.args[1]
        # return the matches as a list of dictionaries
        return matching_rowids.values()


    def __Select(self, table, columns, conditions):
        """__Select() - Used to implement Select and Delete (above)
        Returns a dictionary keyed on rowids containing dicts
        holding the row data for columns listed in the columns param
        that match the given conditions.
        * conditions is a dictionary keyed on column names
        containing callable conditions expecting the data string as an
        argument and returning a boolean.
        """
        # check the validity of each column name
        if table not in self.__tablecolumns:
            self.__load_column_info(table)
        if columns is None:
            columns = self.tablecolumns[table]
        for column in (columns + list(conditions.keys())):
            if not self.__tablecolumns[table].count(column):
                raise TableDBError, "unknown column: %r" % (column,)

        # keyed on rows that match so far, containings dicts keyed on
        # column names containing the data for that row and column.
        matching_rowids = {}
        # keys are rowids that do not match
        rejected_rowids = {}

        # attempt to sort the conditions in such a way as to minimize full
        # column lookups
        def cmp_conditions(atuple, btuple):
            a = atuple[1]
            b = btuple[1]
            if type(a) is type(b):
                if isinstance(a, PrefixCond) and isinstance(b, PrefixCond):
                    # longest prefix first
                    return cmp(len(b.prefix), len(a.prefix))
                if isinstance(a, LikeCond) and isinstance(b, LikeCond):
                    # longest likestr first
                    return cmp(len(b.likestr), len(a.likestr))
                return 0
            if isinstance(a, ExactCond):
                return -1
            if isinstance(b, ExactCond):
                return 1
            if isinstance(a, PrefixCond):
                return -1
            if isinstance(b, PrefixCond):
                return 1
            # leave all unknown condition callables alone as equals
            return 0

        conditionlist = list(conditions.items())
        conditionlist.sort(cmp_conditions)

        # Apply conditions to column data to find what we want
        cur = self.db.cursor()
        column_num = -1
        for column, condition in conditionlist:
            column_num = column_num + 1
            searchkey = _search_col_data_key(table, column)
            # speedup: don't linear search columns within loop
            if column in columns:
                savethiscolumndata = 1  # save the data for return
            else:
                savethiscolumndata = 0  # data only used for selection

            try:
                key, data = cur.set_range(searchkey)
                while key[:len(searchkey)] == searchkey:
                    # extract the rowid from the key
                    rowid = key[-_rowid_str_len:].decode("latin-1")

                    if rowid not in rejected_rowids:
                        # if no condition was specified or the condition
                        # succeeds, add row to our match list.
                        if not condition or condition(data):
                            if rowid not in matching_rowids:
                                matching_rowids[rowid] = {}
                            if savethiscolumndata:
                                matching_rowids[rowid][column] = data
                        else:
                            if rowid in matching_rowids:
                                del matching_rowids[rowid]
                            rejected_rowids[rowid] = rowid

                    key, data = cur.next()

            except DBError as dberror:
                if dberror.args[0] != DB_NOTFOUND:
                    raise
                continue

        cur.close()

        # we're done selecting rows, garbage collect the reject list
        del rejected_rowids

        # extract any remaining desired column data from the
        # database for the matching rows.
        if len(columns) > 0:
            for rowid, rowdata in matching_rowids.items():
                rowid = rowid.encode("latin-1")
                for column in columns:
                    if column in rowdata:
                        continue
                    try:
                        rowdata[column] = self.db.get(
                            _data_key(table, column, rowid))
                    except DBError as dberror:
                        if dberror.args[0] != DB_NOTFOUND:
                            raise
                        rowdata[column] = None

        # return the matches
        return matching_rowids


    def Drop(self, table):
        """Remove an entire table from the database"""
        txn = None
        try:
            txn = self.env.txn_begin()

            # delete the column list
            self.db.delete(_columns_key(table), txn)

            cur = self.db.cursor(txn)

            # delete all keys containing this tables column and row info
            table_key = _search_all_data_key(table)
            while 1:
                try:
                    key, data = cur.set_range(table_key)
                except DBNotFoundError:
                    break
                # only delete items in this table
                if key[:len(table_key)] != table_key:
                    break
                cur.delete()

            # delete all rowids used by this table
            table_key = _search_rowid_key(table)
            while 1:
                try:
                    key, data = cur.set_range(table_key)
                except DBNotFoundError:
                    break
                # only delete items in this table
                if key[:len(table_key)] != table_key:
                    break
                cur.delete()

            cur.close()

            # delete the tablename from the table name list
            tablelist = pickle.loads(
                self.db.get(_E(_table_names_key), txn=txn, flags=DB_RMW))
            try:
                tablelist.remove(table)
            except ValueError:
                # hmm, it wasn't there, oh well, that's what we want.
                pass
            # delete 1st, incase we opened with DB_DUP
            self.db.delete(_E(_table_names_key), txn)
            self.db.put(_E(_table_names_key), pickle.dumps(tablelist, 1), txn=txn)

            txn.commit()
            txn = None

            if table in self.__tablecolumns:
                del self.__tablecolumns[table]

        except DBError as dberror:
            raise TableDBError, dberror.args[1]
        finally:
            if txn:
                txn.abort()
