"""
dialect = Sniffer().sniff(file('csv/easy.csv'))
print "delimiter", dialect.delimiter
print "quotechar", dialect.quotechar
print "skipinitialspace", dialect.skipinitialspace
"""

from csv import csv
import re

# ------------------------------------------------------------------------------
class Sniffer:
    """
    "Sniffs" the format of a CSV file (i.e. delimiter, quotechar)
    Returns a csv.Dialect object.
    """
    def __init__(self, sample = 16 * 1024):
        # in case there is more than one possible delimiter
        self.preferred = [',', '\t', ';', ' ', ':']

        # amount of data (in bytes) to sample
        self.sample = sample


    def sniff(self, fileobj):
        """
        Takes a file-like object and returns a dialect (or None)
        """

        self.fileobj = fileobj

        data = fileobj.read(self.sample)

        quotechar, delimiter, skipinitialspace = self._guessQuoteAndDelimiter(data)
        if delimiter is None:
            delimiter, skipinitialspace = self._guessDelimiter(data)

        class Dialect(csv.Dialect):
            _name = "sniffed"
            lineterminator = '\r\n'
            quoting = csv.QUOTE_MINIMAL
            # escapechar = ''
            doublequote = False
        Dialect.delimiter = delimiter
        Dialect.quotechar = quotechar
        Dialect.skipinitialspace = skipinitialspace

        self.dialect = Dialect
        return self.dialect


    def hasHeaders(self):
        return self._hasHeaders(self.fileobj, self.dialect)


    def register_dialect(self, name = 'sniffed'):
        csv.register_dialect(name, self.dialect)


    def _guessQuoteAndDelimiter(self, data):
        """
        Looks for text enclosed between two identical quotes
        (the probable quotechar) which are preceded and followed
        by the same character (the probable delimiter).
        For example:
                         ,'some text',
        The quote with the most wins, same with the delimiter.
        If there is no quotechar the delimiter can't be determined
        this way.
        """

        matches = []
        for restr in ('(?P<delim>[^\w\n"\'])(?P<space> ?)(?P<quote>["\']).*?(?P=quote)(?P=delim)', # ,".*?",
                      '(?:^|\n)(?P<quote>["\']).*?(?P=quote)(?P<delim>[^\w\n"\'])(?P<space> ?)',   #  ".*?",
                      '(?P<delim>>[^\w\n"\'])(?P<space> ?)(?P<quote>["\']).*?(?P=quote)(?:$|\n)',  # ,".*?"
                      '(?:^|\n)(?P<quote>["\']).*?(?P=quote)(?:$|\n)'):                            #  ".*?" (no delim, no space)
            regexp = re.compile(restr, re.S | re.M)
            matches = regexp.findall(data)
            if matches:
                break

        if not matches:
            return ('', None, 0) # (quotechar, delimiter, skipinitialspace)

        quotes = {}
        delims = {}
        spaces = 0
        for m in matches:
            n = regexp.groupindex['quote'] - 1
            key = m[n]
            if key:
                quotes[key] = quotes.get(key, 0) + 1
            try:
                n = regexp.groupindex['delim'] - 1
                key = m[n]
            except KeyError:
                continue
            if key:
                delims[key] = delims.get(key, 0) + 1
            try:
                n = regexp.groupindex['space'] - 1
            except KeyError:
                continue
            if m[n]:
                spaces += 1

        quotechar = reduce(lambda a, b, quotes = quotes:
                           (quotes[a] > quotes[b]) and a or b, quotes.keys())

        if delims:
            delim = reduce(lambda a, b, delims = delims:
                           (delims[a] > delims[b]) and a or b, delims.keys())
            skipinitialspace = delims[delim] == spaces
            if delim == '\n': # most likely a file with a single column
                delim = ''
        else:
            # there is *no* delimiter, it's a single column of quoted data
            delim = ''
            skipinitialspace = 0

        return (quotechar, delim, skipinitialspace)


    def _guessDelimiter(self, data):
        """
        The delimiter /should/ occur the same number of times on
        each row. However, due to malformed data, it may not. We don't want
        an all or nothing approach, so we allow for small variations in this
        number.
          1) build a table of the frequency of each character on every line.
          2) build a table of freqencies of this frequency (meta-frequency?),
             e.g.  "x occurred 5 times in 10 rows, 6 times in 1000 rows,
             7 times in 2 rows"
          3) use the mode of the meta-frequency to determine the /expected/
             frequency for that character
          4) find out how often the character actually meets that goal
          5) the character that best meets its goal is the delimiter
        For performance reasons, the data is evaluated in chunks, so it can
        try and evaluate the smallest portion of the data possible, evaluating
        additional chunks as necessary.
        """

        data = filter(None, data.split('\n'))

        ascii = [chr(c) for c in range(127)] # 7-bit ASCII

        # build frequency tables
        chunkLength = min(10, len(data))
        iteration = 0
        charFrequency = {}
        modes = {}
        delims = {}
        start, end = 0, min(chunkLength, len(data))
        while start < len(data):
            iteration += 1
            for line in data[start:end]:
                for char in ascii:
                    metafrequency = charFrequency.get(char, {})
                    freq = line.strip().count(char) # must count even if frequency is 0
                    metafrequency[freq] = metafrequency.get(freq, 0) + 1 # value is the mode
                    charFrequency[char] = metafrequency

            for char in charFrequency.keys():
                items = charFrequency[char].items()
                if len(items) == 1 and items[0][0] == 0:
                    continue
                # get the mode of the frequencies
                if len(items) > 1:
                    modes[char] = reduce(lambda a, b: a[1] > b[1] and a or b, items)
                    # adjust the mode - subtract the sum of all other frequencies
                    items.remove(modes[char])
                    modes[char] = (modes[char][0], modes[char][1]
                                   - reduce(lambda a, b: (0, a[1] + b[1]), items)[1])
                else:
                    modes[char] = items[0]

            # build a list of possible delimiters
            modeList = modes.items()
            total = float(chunkLength * iteration)
            consistency = 1.0 # (rows of consistent data) / (number of rows) = 100%
            threshold = 0.9  # minimum consistency threshold
            while len(delims) == 0 and consistency >= threshold:
                for k, v in modeList:
                    if v[0] > 0 and v[1] > 0:
                        if (v[1]/total) >= consistency:
                            delims[k] = v
                consistency -= 0.01

            if len(delims) == 1:
                delim = delims.keys()[0]
                skipinitialspace = data[0].count(delim) == data[0].count("%c " % delim)
                return (delim, skipinitialspace)

            # analyze another chunkLength lines
            start = end
            end += chunkLength

        if not delims:
            return ('', 0)

        # if there's more than one, fall back to a 'preferred' list
        if len(delims) > 1:
            for d in self.preferred:
                if d in delims.keys():
                    skipinitialspace = data[0].count(d) == data[0].count("%c " % d)
                    return (d, skipinitialspace)

        # finally, just return the first damn character in the list
        delim = delims.keys()[0]
        skipinitialspace = data[0].count(delim) == data[0].count("%c " % delim)
        return (delim, skipinitialspace)


    def _hasHeaders(self, fileobj, dialect):
        # Creates a dictionary of types of data in each column. If any column
        # is of a single type (say, integers), *except* for the first row, then the first
        # row is presumed to be labels. If the type can't be determined, it is assumed to
        # be a string in which case the length of the string is the determining factor: if
        # all of the rows except for the first are the same length, it's a header.
        # Finally, a 'vote' is taken at the end for each column, adding or subtracting from
        # the likelihood of the first row being a header.

        def seval(item):
            """
            Strips parens from item prior to calling eval in an attempt to make it safer
            """
            return eval(item.replace('(', '').replace(')', ''))

        fileobj.seek(0) # rewind the fileobj - this might not work for some file-like objects...

        reader = csv.reader(fileobj,
                            delimiter = dialect.delimiter,
                            quotechar = dialect.quotechar,
                            skipinitialspace = dialect.skipinitialspace)

        header = reader.next() # assume first row is header

        columns = len(header)
        columnTypes = {}
        for i in range(columns): columnTypes[i] = None

        checked = 0
        for row in reader:
            if checked > 20: # arbitrary number of rows to check, to keep it sane
                break
            checked += 1

            if len(row) != columns:
                continue # skip rows that have irregular number of columns

            for col in columnTypes.keys():
                try:
                    try:
                        # is it a built-in type (besides string)?
                        thisType = type(seval(row[col]))
                    except OverflowError:
                        # a long int?
                        thisType = type(seval(row[col] + 'L'))
                        thisType = type(0) # treat long ints as int
                except:
                    # fallback to length of string
                    thisType = len(row[col])

                if thisType != columnTypes[col]:
                    if columnTypes[col] is None: # add new column type
                        columnTypes[col] = thisType
                    else: # type is inconsistent, remove column from consideration
                        del columnTypes[col]

        # finally, compare results against first row and "vote" on whether it's a header
        hasHeader = 0
        for col, colType in columnTypes.items():
            if type(colType) == type(0): # it's a length
                if len(header[col]) != colType:
                    hasHeader += 1
                else:
                    hasHeader -= 1
            else: # attempt typecast
                try:
                    eval("%s(%s)" % (colType.__name__, header[col]))
                except:
                    hasHeader += 1
                else:
                    hasHeader -= 1

        return hasHeader > 0
