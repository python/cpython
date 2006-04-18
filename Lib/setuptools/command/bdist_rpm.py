# This is just a kludge so that bdist_rpm doesn't guess wrong about the
# distribution name and version, if the egg_info command is going to alter
# them, and another kludge to allow you to build old-style non-egg RPMs

from distutils.command.bdist_rpm import bdist_rpm as _bdist_rpm

class bdist_rpm(_bdist_rpm):

    def initialize_options(self):
        _bdist_rpm.initialize_options(self)
        self.no_egg = None

    def run(self):
        self.run_command('egg_info')    # ensure distro name is up-to-date
        _bdist_rpm.run(self)

    def _make_spec_file(self):
        version = self.distribution.get_version()
        rpmversion = version.replace('-','_')
        spec = _bdist_rpm._make_spec_file(self)
        line23 = '%define version '+version
        line24 = '%define version '+rpmversion
        spec  = [
            line.replace(
                "Source0: %{name}-%{version}.tar",
                "Source0: %{name}-%{unmangled_version}.tar"
            ).replace(
                "setup.py install ",
                "setup.py install --single-version-externally-managed "
            ).replace(
                "%setup",
                "%setup -n %{name}-%{unmangled_version}"
            ).replace(line23,line24)
            for line in spec
        ]
        spec.insert(spec.index(line24)+1, "%define unmangled_version "+version)
        return spec































