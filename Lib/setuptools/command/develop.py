from setuptools.command.easy_install import easy_install
from distutils.util import convert_path
from pkg_resources import Distribution, PathMetadata, normalize_path
from distutils import log
from distutils.errors import *
import sys, os

class develop(easy_install):
    """Set up package for development"""

    description = "install package in 'development mode'"

    user_options = easy_install.user_options + [
        ("uninstall", "u", "Uninstall this source package"),
    ]

    boolean_options = easy_install.boolean_options + ['uninstall']

    command_consumes_arguments = False  # override base

    def run(self):
        if self.uninstall:
            self.multi_version = True
            self.uninstall_link()
        else:
            self.install_for_development()
        self.warn_deprecated_options()

    def initialize_options(self):
        self.uninstall = None
        easy_install.initialize_options(self)










    def finalize_options(self):
        ei = self.get_finalized_command("egg_info")
        if ei.broken_egg_info:
            raise DistutilsError(
            "Please rename %r to %r before using 'develop'"
            % (ei.egg_info, ei.broken_egg_info)
            )
        self.args = [ei.egg_name]       
        easy_install.finalize_options(self)
        self.egg_link = os.path.join(self.install_dir, ei.egg_name+'.egg-link')
        self.egg_base = ei.egg_base
        self.egg_path = os.path.abspath(ei.egg_base)

        # Make a distribution for the package's source
        self.dist = Distribution(
            normalize_path(self.egg_path),
            PathMetadata(self.egg_path, os.path.abspath(ei.egg_info)),
            project_name = ei.egg_name
        )

    def install_for_development(self):
        # Ensure metadata is up-to-date
        self.run_command('egg_info')

        # Build extensions in-place
        self.reinitialize_command('build_ext', inplace=1)
        self.run_command('build_ext')

        self.install_site_py()  # ensure that target dir is site-safe

        # create an .egg-link in the installation dir, pointing to our egg
        log.info("Creating %s (link to %s)", self.egg_link, self.egg_base)
        if not self.dry_run:
            f = open(self.egg_link,"w")
            f.write(self.egg_path)
            f.close()

        # postprocess the installed distro, fixing up .pth, installing scripts,
        # and handling requirements
        self.process_distribution(None, self.dist)

    def uninstall_link(self):
        if os.path.exists(self.egg_link):
            log.info("Removing %s (link to %s)", self.egg_link, self.egg_base)
            contents = [line.rstrip() for line in file(self.egg_link)]
            if contents != [self.egg_path]:
                log.warn("Link points to %s: uninstall aborted", contents)
                return
            if not self.dry_run:
                os.unlink(self.egg_link)
        if not self.dry_run:
            self.update_pth(self.dist)  # remove any .pth link to us
        if self.distribution.scripts:
            # XXX should also check for entry point scripts!
            log.warn("Note: you must uninstall or replace scripts manually!")


    def install_egg_scripts(self, dist):
        if dist is not self.dist:
            # Installing a dependency, so fall back to normal behavior
            return easy_install.install_egg_scripts(self,dist)

        # create wrapper scripts in the script dir, pointing to dist.scripts

        # new-style...
        self.install_wrapper_scripts(dist)  

        # ...and old-style
        for script_name in self.distribution.scripts or []:
            script_path = os.path.abspath(convert_path(script_name))
            script_name = os.path.basename(script_path)
            f = open(script_path,'rU')
            script_text = f.read()
            f.close()
            self.install_script(dist, script_name, script_text, script_path)







