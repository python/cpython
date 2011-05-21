"""Create the PEP 376-compliant .dist-info directory."""

# Forked from the former install_egg_info command by Josip Djolonga

import csv
import os
import re
import hashlib

from packaging.command.cmd import Command
from packaging import logger
from shutil import rmtree


class install_distinfo(Command):

    description = 'create a .dist-info directory for the distribution'

    user_options = [
        ('distinfo-dir=', None,
         "directory where the the .dist-info directory will be installed"),
        ('installer=', None,
         "the name of the installer"),
        ('requested', None,
         "generate a REQUESTED file"),
        ('no-requested', None,
         "do not generate a REQUESTED file"),
        ('no-record', None,
         "do not generate a RECORD file"),
        ('no-resources', None,
         "do not generate a RESSOURCES list installed file")
    ]

    boolean_options = ['requested', 'no-record', 'no-resources']

    negative_opt = {'no-requested': 'requested'}

    def initialize_options(self):
        self.distinfo_dir = None
        self.installer = None
        self.requested = None
        self.no_record = None
        self.no_resources = None

    def finalize_options(self):
        self.set_undefined_options('install_dist',
                                   'installer', 'requested', 'no_record')

        self.set_undefined_options('install_lib',
                                   ('install_dir', 'distinfo_dir'))

        if self.installer is None:
            # FIXME distutils or packaging?
            # + document default in the option help text above and in install
            self.installer = 'distutils'
        if self.requested is None:
            self.requested = True
        if self.no_record is None:
            self.no_record = False
        if self.no_resources is None:
            self.no_resources = False

        metadata = self.distribution.metadata

        basename = "%s-%s.dist-info" % (
            to_filename(safe_name(metadata['Name'])),
            to_filename(safe_version(metadata['Version'])))

        self.distinfo_dir = os.path.join(self.distinfo_dir, basename)
        self.outputs = []

    def run(self):
        # FIXME dry-run should be used at a finer level, so that people get
        # useful logging output and can have an idea of what the command would
        # have done
        if not self.dry_run:
            target = self.distinfo_dir

            if os.path.isdir(target) and not os.path.islink(target):
                rmtree(target)
            elif os.path.exists(target):
                self.execute(os.unlink, (self.distinfo_dir,),
                             "removing " + target)

            self.execute(os.makedirs, (target,), "creating " + target)

            metadata_path = os.path.join(self.distinfo_dir, 'METADATA')
            logger.info('creating %s', metadata_path)
            self.distribution.metadata.write(metadata_path)
            self.outputs.append(metadata_path)

            installer_path = os.path.join(self.distinfo_dir, 'INSTALLER')
            logger.info('creating %s', installer_path)
            with open(installer_path, 'w') as f:
                f.write(self.installer)
            self.outputs.append(installer_path)

            if self.requested:
                requested_path = os.path.join(self.distinfo_dir, 'REQUESTED')
                logger.info('creating %s', requested_path)
                open(requested_path, 'wb').close()
                self.outputs.append(requested_path)


            if not self.no_resources:
                install_data = self.get_finalized_command('install_data')
                if install_data.get_resources_out() != []:
                    resources_path = os.path.join(self.distinfo_dir,
                                                  'RESOURCES')
                    logger.info('creating %s', resources_path)
                    with open(resources_path, 'wb') as f:
                        writer = csv.writer(f, delimiter=',',
                                            lineterminator='\n',
                                            quotechar='"')
                        for tuple in install_data.get_resources_out():
                            writer.writerow(tuple)

                        self.outputs.append(resources_path)

            if not self.no_record:
                record_path = os.path.join(self.distinfo_dir, 'RECORD')
                logger.info('creating %s', record_path)
                with open(record_path, 'w', encoding='utf-8') as f:
                    writer = csv.writer(f, delimiter=',',
                                        lineterminator='\n',
                                        quotechar='"')

                    install = self.get_finalized_command('install_dist')

                    for fpath in install.get_outputs():
                        if fpath.endswith('.pyc') or fpath.endswith('.pyo'):
                            # do not put size and md5 hash, as in PEP-376
                            writer.writerow((fpath, '', ''))
                        else:
                            size = os.path.getsize(fpath)
                            with open(fpath, 'rb') as fp:
                                hash = hashlib.md5()
                                hash.update(fp.read())
                            md5sum = hash.hexdigest()
                            writer.writerow((fpath, md5sum, size))

                    # add the RECORD file itself
                    writer.writerow((record_path, '', ''))
                    self.outputs.append(record_path)

    def get_outputs(self):
        return self.outputs


# The following functions are taken from setuptools' pkg_resources module.

def safe_name(name):
    """Convert an arbitrary string to a standard distribution name

    Any runs of non-alphanumeric/. characters are replaced with a single '-'.
    """
    return re.sub('[^A-Za-z0-9.]+', '-', name)


def safe_version(version):
    """Convert an arbitrary string to a standard version string

    Spaces become dots, and all other non-alphanumeric characters become
    dashes, with runs of multiple dashes condensed to a single dash.
    """
    version = version.replace(' ', '.')
    return re.sub('[^A-Za-z0-9.]+', '-', version)


def to_filename(name):
    """Convert a project or version name to its filename-escaped form

    Any '-' characters are currently replaced with '_'.
    """
    return name.replace('-', '_')
