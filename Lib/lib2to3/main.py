"""
Main program for 2to3.
"""

import sys
import os
import logging
import shutil
import optparse

from . import refactor


class StdoutRefactoringTool(refactor.RefactoringTool):
    """
    Prints output to stdout.
    """

    def __init__(self, fixers, options, explicit, nobackups):
        self.nobackups = nobackups
        super(StdoutRefactoringTool, self).__init__(fixers, options, explicit)

    def log_error(self, msg, *args, **kwargs):
        self.errors.append((msg, args, kwargs))
        self.logger.error(msg, *args, **kwargs)

    def write_file(self, new_text, filename, old_text):
        if not self.nobackups:
            # Make backup
            backup = filename + ".bak"
            if os.path.lexists(backup):
                try:
                    os.remove(backup)
                except os.error, err:
                    self.log_message("Can't remove backup %s", backup)
            try:
                os.rename(filename, backup)
            except os.error, err:
                self.log_message("Can't rename %s to %s", filename, backup)
        # Actually write the new file
        super(StdoutRefactoringTool, self).write_file(new_text,
                                                      filename, old_text)
        if not self.nobackups:
            shutil.copymode(backup, filename)

    def print_output(self, lines):
        for line in lines:
            print line


def main(fixer_pkg, args=None):
    """Main program.

    Args:
        fixer_pkg: the name of a package where the fixers are located.
        args: optional; a list of command line arguments. If omitted,
              sys.argv[1:] is used.

    Returns a suggested exit status (0, 1, 2).
    """
    # Set up option parser
    parser = optparse.OptionParser(usage="2to3 [options] file|dir ...")
    parser.add_option("-d", "--doctests_only", action="store_true",
                      help="Fix up doctests only")
    parser.add_option("-f", "--fix", action="append", default=[],
                      help="Each FIX specifies a transformation; default: all")
    parser.add_option("-x", "--nofix", action="append", default=[],
                      help="Prevent a fixer from being run.")
    parser.add_option("-l", "--list-fixes", action="store_true",
                      help="List available transformations (fixes/fix_*.py)")
    parser.add_option("-p", "--print-function", action="store_true",
                      help="Modify the grammar so that print() is a function")
    parser.add_option("-v", "--verbose", action="store_true",
                      help="More verbose logging")
    parser.add_option("-w", "--write", action="store_true",
                      help="Write back modified files")
    parser.add_option("-n", "--nobackups", action="store_true", default=False,
                      help="Don't write backups for modified files.")

    # Parse command line arguments
    refactor_stdin = False
    options, args = parser.parse_args(args)
    if not options.write and options.nobackups:
        parser.error("Can't use -n without -w")
    if options.list_fixes:
        print "Available transformations for the -f/--fix option:"
        for fixname in refactor.get_all_fix_names(fixer_pkg):
            print fixname
        if not args:
            return 0
    if not args:
        print >>sys.stderr, "At least one file or directory argument required."
        print >>sys.stderr, "Use --help to show usage."
        return 2
    if "-" in args:
        refactor_stdin = True
        if options.write:
            print >>sys.stderr, "Can't write to stdin."
            return 2

    # Set up logging handler
    level = logging.DEBUG if options.verbose else logging.INFO
    logging.basicConfig(format='%(name)s: %(message)s', level=level)

    # Initialize the refactoring tool
    rt_opts = {"print_function" : options.print_function}
    avail_fixes = set(refactor.get_fixers_from_package(fixer_pkg))
    unwanted_fixes = set(fixer_pkg + ".fix_" + fix for fix in options.nofix)
    explicit = set()
    if options.fix:
        all_present = False
        for fix in options.fix:
            if fix == "all":
                all_present = True
            else:
                explicit.add(fixer_pkg + ".fix_" + fix)
        requested = avail_fixes.union(explicit) if all_present else explicit
    else:
        requested = avail_fixes.union(explicit)
    fixer_names = requested.difference(unwanted_fixes)
    rt = StdoutRefactoringTool(sorted(fixer_names), rt_opts, sorted(explicit),
                               options.nobackups)

    # Refactor all files and directories passed as arguments
    if not rt.errors:
        if refactor_stdin:
            rt.refactor_stdin()
        else:
            rt.refactor(args, options.write, options.doctests_only)
        rt.summarize()

    # Return error status (0 if rt.errors is zero)
    return int(bool(rt.errors))
