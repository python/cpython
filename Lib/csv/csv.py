from _csv import Error, __version__, writer, reader, register_dialect, \
                 unregister_dialect, get_dialect, list_dialects, \
                 QUOTE_MINIMAL, QUOTE_ALL, QUOTE_NONNUMERIC, QUOTE_NONE, \
                 __doc__

__all__ = [ "QUOTE_MINIMAL", "QUOTE_ALL", "QUOTE_NONNUMERIC", "QUOTE_NONE",
            "Error", "Dialect", "excel", "excel_tab", "reader", "writer",
            "register_dialect", "get_dialect", "list_dialects",
            "unregister_dialect", "__version__", "DictReader", "DictWriter" ]

class Dialect:
    _name = ""
    _valid = False
    # placeholders
    delimiter = None
    quotechar = None
    escapechar = None
    doublequote = None
    skipinitialspace = None
    lineterminator = None
    quoting = None

    def __init__(self):
        if self.__class__ != Dialect:
            self._valid = True
        errors = self._validate()
        if errors != []:
            raise Error, "Dialect did not validate: %s" % ", ".join(errors)

    def _validate(self):
        errors = []
        if not self._valid:
            errors.append("can't directly instantiate Dialect class")

        if self.delimiter is None:
            errors.append("delimiter character not set")
        elif (not isinstance(self.delimiter, str) or
              len(self.delimiter) > 1):
            errors.append("delimiter must be one-character string")

        if self.quotechar is None:
            if self.quoting != QUOTE_NONE:
                errors.append("quotechar not set")
        elif (not isinstance(self.quotechar, str) or
              len(self.quotechar) > 1):
            errors.append("quotechar must be one-character string")

        if self.lineterminator is None:
            errors.append("lineterminator not set")
        elif not isinstance(self.lineterminator, str):
            errors.append("lineterminator must be a string")

        if self.doublequote not in (True, False):
            errors.append("doublequote parameter must be True or False")

        if self.skipinitialspace not in (True, False):
            errors.append("skipinitialspace parameter must be True or False")

        if self.quoting is None:
            errors.append("quoting parameter not set")

        if self.quoting is QUOTE_NONE:
            if (not isinstance(self.escapechar, (unicode, str)) or
                len(self.escapechar) > 1):
                errors.append("escapechar must be a one-character string or unicode object")

        return errors

class excel(Dialect):
    delimiter = ','
    quotechar = '"'
    doublequote = True
    skipinitialspace = False
    lineterminator = '\r\n'
    quoting = QUOTE_MINIMAL
register_dialect("excel", excel)

class excel_tab(excel):
    delimiter = '\t'
register_dialect("excel-tab", excel_tab)


class DictReader:
    def __init__(self, f, fieldnames, restkey=None, restval=None,
                 dialect="excel", *args):
        self.fieldnames = fieldnames    # list of keys for the dict
        self.restkey = restkey          # key to catch long rows
        self.restval = restval          # default value for short rows
        self.reader = reader(f, dialect, *args)

    def __iter__(self):
        return self

    def next(self):
        row = self.reader.next()
        # unlike the basic reader, we prefer not to return blanks,
        # because we will typically wind up with a dict full of None
        # values
        while row == []:
            row = self.reader.next()
        d = dict(zip(self.fieldnames, row))
        lf = len(self.fieldnames)
        lr = len(row)
        if lf < lr:
            d[self.restkey] = row[lf:]
        elif lf > lr:
            for key in self.fieldnames[lr:]:
                d[key] = self.restval
        return d


class DictWriter:
    def __init__(self, f, fieldnames, restval="", extrasaction="raise",
                 dialect="excel", *args):
        self.fieldnames = fieldnames    # list of keys for the dict
        self.restval = restval          # for writing short dicts
        if extrasaction.lower() not in ("raise", "ignore"):
            raise ValueError, \
                  ("extrasaction (%s) must be 'raise' or 'ignore'" %
                   extrasaction)
        self.extrasaction = extrasaction
        self.writer = writer(f, dialect, *args)

    def _dict_to_list(self, rowdict):
        if self.extrasaction == "raise":
            for k in rowdict.keys():
                if k not in self.fieldnames:
                    raise ValueError, "dict contains fields not in fieldnames"
        return [rowdict.get(key, self.restval) for key in self.fieldnames]

    def writerow(self, rowdict):
        return self.writer.writerow(self._dict_to_list(rowdict))

    def writerows(self, rowdicts):
        rows = []
        for rowdict in rowdicts:
            rows.append(self._dict_to_list(rowdict))
        return self.writer.writerows(rows)
