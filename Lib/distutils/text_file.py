"""text_file

provides the TextFile class, which gives an interface to text files
that (optionally) takes care of stripping comments, ignoring blank
lines, and joining lines with backslashes."""

# created 1999/01/12, Greg Ward

__revision__ = "$Id$"

from types import *
import os, string, re


class TextFile:
    filename = None
    file = None
    current_line = None

    default_options = { 'strip_comments': 1,
                        'comment_re':     re.compile (r'\s*#.*'),
                        'skip_blanks':    1,
                        'join_lines':     0,
                        'lstrip_ws':      0,
                        'rstrip_ws':      1,
                      }

    def __init__ (self, filename=None, **options):

        # set values for all options -- either from client option hash
        # or fallback to default_options
        for opt in self.default_options.keys():
            if options.has_key (opt):
                if opt == 'comment_re' and type (options[opt]) is StringType:
                    self.comment_re = re.compile (options[opt])
                else:
                    setattr (self, opt, options[opt])

            else:
                setattr (self, opt, self.default_options[opt])

        # sanity check client option hash
        for opt in options.keys():
            if not self.default_options.has_key (opt):
                raise KeyError, "invalid TextFile option '%s'" % opt

        self.filename = filename
        if self.filename:
            self.open ()
        

    def open (self, filename=None):
        if not self.filename:
            if not filename:
                raise RuntimeError, "must provide a filename somehow"

            self.filename = filename

        self.file = open (self.filename, 'r')
        self.current_line = 0


    def close (self):
        self.file.close ()
        self.file = None
        self.filename = None
        self.current_line = None


    def readline (self):

        buildup_line = ''

        while 1:
            # read the line, optionally strip comments
            line = self.file.readline()
            if self.strip_comments and line:
                line = self.comment_re.sub ('', line)

            # did previous line end with a backslash? then accumulate
            if self.join_lines and buildup_line:
                # oops: end of file
                if not line:
                    self.warn ("continuation line immediately precedes "
                               "end-of-file")
                    return buildup_line

                line = buildup_line + line

                # careful: pay attention to line number when incrementing it
                if type (self.current_line) is ListType:
                    self.current_line[1] = self.current_line[1] + 1
                else:
                    self.current_line = [self.current_line, self.current_line+1]
            # just an ordinary line, read it as usual
            else:
                if not line:
                    return None

                # still have to be careful about incrementing the line number!
                if type (self.current_line) is ListType:
                    self.current_line = self.current_line[1] + 1
                else:
                    self.current_line = self.current_line + 1
                

            # strip whitespace however the client wants (leading and
            # trailing, or one or the other, or neither)
            if self.lstrip_ws and self.rstrip_ws:
                line = string.strip (line)
            else:
                if self.lstrip_ws:
                    line = string.lstrip (line)
                if self.rstrip_ws:
                    line = string.rstrip (line)

            # blank line (whether we rstrip'ed or not)? skip to next line
            # if appropriate
            if line == '' or line == '\n' and self.skip_blanks:
                continue

            if self.join_lines:
                if line[-1] == '\\':
                    buildup_line = line[:-1]
                    continue

                if line[-2:] == '\\\n':
                    buildup_line = line[0:-2] + '\n'
                    continue

            # well, I guess there's some actual content there: return it
            return line

    # end readline


    def readlines (self):
        lines = []
        while 1:
            line = self.readline()
            if line is None:
                return lines
            lines.append (line)


if __name__ == "__main__":
    test_data = """# test file

line 3 \\
continues on next line
"""

    # result 1: no fancy options
    result1 = map (lambda x: x + "\n", string.split (test_data, "\n")[0:-1])

    # result 2: just strip comments
    result2 = ["\n", "\n", "line 3 \\\n", "continues on next line\n"]

    # result 3: just strip blank lines
    result3 = ["# test file\n", "line 3 \\\n", "continues on next line\n"]

    # result 4: default, strip comments, blank lines, and trailing whitespace
    result4 = ["line 3 \\", "continues on next line"]

    # result 5: full processing, strip comments and blanks, plus join lines
    result5 = ["line 3 continues on next line"]

    def test_input (count, description, file, expected_result):
        result = file.readlines ()
        # result = string.join (result, '')
        if result == expected_result:
            print "ok %d (%s)" % (count, description)
        else:
            print "not ok %d (%s):" % (count, description)
            print "** expected:"
            print expected_result
            print "** received:"
            print result
            

    filename = "test.txt"
    out_file = open (filename, "w")
    out_file.write (test_data)
    out_file.close ()

    in_file = TextFile (filename, strip_comments=0, skip_blanks=0,
                   lstrip_ws=0, rstrip_ws=0)
    test_input (1, "no processing", in_file, result1)

    in_file = TextFile (filename, strip_comments=1, skip_blanks=0,
                   lstrip_ws=0, rstrip_ws=0)
    test_input (2, "strip comments", in_file, result2)

    in_file = TextFile (filename, strip_comments=0, skip_blanks=1,
                   lstrip_ws=0, rstrip_ws=0)
    test_input (3, "strip blanks", in_file, result3)

    in_file = TextFile (filename)
    test_input (4, "default processing", in_file, result4)

    in_file = TextFile (filename, strip_comments=1, skip_blanks=1,
                        join_lines=1, rstrip_ws=1)
    test_input (5, "full processing", in_file, result5)

    os.remove (filename)
    
