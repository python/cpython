"""distutils.command.bdist_dumb

Implements the Distutils 'bdist_dumb' command (create a "dumb" built
distribution -- i.e., just an archive to be unpacked under $prefix or
$exec_prefix)."""

# created 2000/03/29, Greg Ward

__revision__ = "$Id$"

import os
from distutils.core import Command
from distutils.util import get_platform, create_tree, remove_tree
from distutils.errors import *

class bdist_dumb (Command):

    description = "create a \"dumb\" built distribution"

    user_options = [('format=', 'f',
                     "archive format to create (tar, ztar, gztar, zip)"),
                    ('keep-tree', 'k',
                     "keep the pseudo-installation tree around after " +
                     "creating the distribution archive"),
                   ]

    default_format = { 'posix': 'gztar',
                       'nt': 'zip', }


    def initialize_options (self):
        self.format = None
        self.keep_tree = 0

    # initialize_options()


    def finalize_options (self):
        if self.format is None:
            try:
                self.format = self.default_format[os.name]
            except KeyError:
                raise DistutilsPlatformError, \
                      ("don't know how to create dumb built distributions " +
                       "on platform %s") % os.name

    # finalize_options()


    def run (self):

        self.run_peer ('build')
        install = self.find_peer ('install')
        inputs = install.get_inputs ()
        outputs = install.get_outputs ()
        assert (len (inputs) == len (outputs))

        # First, strip the installation base directory (prefix or
        # exec-prefix) from all the output filenames.
        self.strip_base_dirs (outputs, install)

        # Figure out where to copy them to: "build/bdist" by default; this
        # directory masquerades as prefix/exec-prefix (ie.  we'll make the
        # archive from 'output_dir').
        build_base = self.get_peer_option ('build', 'build_base')
        output_dir = os.path.join (build_base, "bdist")

        # Copy the built files to the pseudo-installation tree.
        self.make_install_tree (output_dir, inputs, outputs)

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        archive_basename = "%s.%s" % (self.distribution.get_fullname(),
                                      get_platform())
        print "output_dir = %s" % output_dir
        print "self.format = %s" % self.format
        self.make_archive (archive_basename, self.format,
                           root_dir=output_dir)

        if not self.keep_tree:
            remove_tree (output_dir, self.verbose, self.dry_run)

    # run()


    def strip_base_dirs (self, outputs, install_cmd):
        # XXX this throws away the prefix/exec-prefix distinction, and
        # means we can only correctly install the resulting archive on a
        # system where prefix == exec-prefix (but at least we can *create*
        # it on one where they differ).  I don't see a way to fix this
        # without either 1) generating two archives, one for prefix and one
        # for exec-prefix, or 2) putting absolute paths in the archive
        # rather than making them relative to one of the prefixes.

        base = install_cmd.install_base + os.sep
        platbase = install_cmd.install_platbase + os.sep
        b_len = len (base)
        pb_len = len (platbase)
        for i in range (len (outputs)):
            if outputs[i][0:b_len] == base:
                outputs[i] = outputs[i][b_len:]
            elif outputs[i][0:pb_len] == platbase:
                outputs[i] = outputs[i][pb_len:]
            else:
                raise DistutilsInternalError, \
                      ("installation output filename '%s' doesn't start " + 
                       "with either install_base ('%s') or " +
                       "install_platbase ('%s')") % \
                      (outputs[i], base, platbase)

    # strip_base_dirs()


    def make_install_tree (self, output_dir, inputs, outputs):

        assert (len(inputs) == len(outputs))

        # Create all the directories under 'output_dir' necessary to
        # put 'outputs' there.
        create_tree (output_dir, outputs,
                     verbose=self.verbose, dry_run=self.dry_run)


        # XXX this bit of logic is duplicated in sdist.make_release_tree():
        # would be nice to factor it out...
        if hasattr (os, 'link'):        # can make hard links on this system
            link = 'hard'
            msg = "making hard links in %s..." % output_dir
        else:                           # nope, have to copy
            link = None
            msg = "copying files to %s..." % output_dir

        for i in range (len(inputs)):
            output = os.path.join (output_dir, outputs[i])
            self.copy_file (inputs[i], output, link=link)

    # make_install_tree ()


# class bdist_dumb
