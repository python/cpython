"""Install platform-independent data files."""

# Contributed by Bastian Kleineidam

import os
from shutil import Error
from sysconfig import get_paths, format_value
from packaging import logger
from packaging.util import convert_path
from packaging.command.cmd import Command


class install_data(Command):

    description = "install platform-independent data files"

    user_options = [
        ('install-dir=', 'd',
         "base directory for installing data files "
         "(default: installation base dir)"),
        ('root=', None,
         "install everything relative to this alternate root directory"),
        ('force', 'f', "force installation (overwrite existing files)"),
        ]

    boolean_options = ['force']

    def initialize_options(self):
        self.install_dir = None
        self.outfiles = []
        self.data_files_out = []
        self.root = None
        self.force = False
        self.data_files = self.distribution.data_files
        self.warn_dir = True

    def finalize_options(self):
        self.set_undefined_options('install_dist',
                                   ('install_data', 'install_dir'),
                                   'root', 'force')

    def run(self):
        self.mkpath(self.install_dir)
        for _file in self.data_files.items():
            destination = convert_path(self.expand_categories(_file[1]))
            dir_dest = os.path.abspath(os.path.dirname(destination))

            self.mkpath(dir_dest)
            try:
                out = self.copy_file(_file[0], dir_dest)[0]
            except Error as e:
                logger.warning('%s: %s', self.get_command_name(), e)
                out = destination

            self.outfiles.append(out)
            self.data_files_out.append((_file[0], destination))

    def expand_categories(self, path_with_categories):
        local_vars = get_paths()
        local_vars['distribution.name'] = self.distribution.metadata['Name']
        expanded_path = format_value(path_with_categories, local_vars)
        expanded_path = format_value(expanded_path, local_vars)
        if '{' in expanded_path and '}' in expanded_path:
            logger.warning(
                '%s: unable to expand %s, some categories may be missing',
                self.get_command_name(), path_with_categories)
        return expanded_path

    def get_source_files(self):
        return list(self.data_files)

    def get_inputs(self):
        return list(self.data_files)

    def get_outputs(self):
        return self.outfiles

    def get_resources_out(self):
        return self.data_files_out
