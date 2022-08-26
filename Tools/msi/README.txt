Quick Build Info
================

For testing, the installer should be built with the Tools/msi/build.bat
script:

    build.bat [-x86] [-x64] [-ARM64] [--doc]

For an official release, the installer should be built with the
Tools/msi/buildrelease.bat script and environment variables:

    set PYTHON=<path to Python 3.8 or later>
    set SPHINXBUILD=<path to sphinx-build.exe>
    set PATH=<path to Git (git.exe)>;%PATH%

    buildrelease.bat [-x86] [-x64] [-ARM64] [-D] [-B]
        [-o <output directory>] [-c <certificate name>]

See the Building the Installer section for more information.

Overview
========

Python is distributed on Windows as an installer that will configure the
user's system. This allows users to have a functioning copy of Python
without having to build it themselves.

The main tasks of the installer are:

* copy required files into the expected layout
* configure system settings so the installation can be located by
  other programs
* add entry points for modifying, repairing and uninstalling Python
* make it easy to launch Python, its documentation, and IDLE

Each of these is discussed in a later section of this document.

Structure of the Installer
==========================

The installer is structured as a 'layout', which consists of a number of
CAB and MSI files and a single EXE.

The EXE is the main entry point into the installer. It contains the UI
and command-line logic, as well as the ability to locate and optionally
download other parts of the layout.

Each MSI contains the logic required to install a component or feature
of Python. These MSIs should not be launched directly by users. MSIs can
be embedded into the EXE or automatically downloaded as needed.

Each CAB contains the files making up a Python installation. CABs are
embedded into their associated MSI and are never seen by users.

MSIs are only required when the related feature or component is being
installed. When components are not selected for installation, the
associated MSI is not downloaded. This allows the installer to offer
options to install debugging symbols and binaries without increasing
the initial download size by separating them into their own MSIs.

Building the Installer
======================

Before building the installer, download extra build dependencies using
Tools\msi\get_externals.bat. (Note that this is in addition to the
similarly named file in PCbuild.)

One of the dependencies used in builds is WiX, a toolset that lets developers
create installers for Windows Installer, the Windows installation engine. WiX
has a dependency on the Microsoft .NET Framework Version 3.5 (which may not be
configured on recent versions of Windows, such as Windows 10). If you are
building on a recent Windows version, use the Control Panel (Programs | Programs
and Features | Turn Windows Features on or off) and ensure that the entry
".NET Framework 3.5 (includes .NET 2.0 and 3.0)" is enabled.

For testing, the installer should be built with the Tools/msi/build.bat
script:

    build.bat [-x86] [-x64] [-ARM64] [--doc] [--test-marker] [--pack]

This script will build the required configurations of Python and
generate an installer layout in PCbuild/(win32|amd64)/en-us.

Specify -x86, -x64 and/or -ARM64 to build for each platform. If none are
specified, both x64 and x86 will be built. Currently, both the debug and
release versions of Python are required for the installer.

Specify --doc to include the documentation files. Ensure %PYTHON% and
%SPHINXBUILD% are set when passing this option.

Specify --test-marker to build an installer that works side-by-side with
an official Python release. All registry keys and install locations will
include an extra marker to avoid overwriting files. This marker is
currently an 'x' prefix, but may change at any time.

Specify --pack to build an installer that does not require all MSIs to
be available alongside. This takes longer, but is easier to share.


For an official release, the installer should be built with the
Tools/msi/buildrelease.bat script:

    set PYTHON=<path to Python 2.7 or 3.4>
    set SPHINXBUILD=<path to sphinx-build.exe>
    set PATH=<path to Git (git.exe)>;%PATH%

    buildrelease.bat [-x86] [-x64] [-ARM64] [-D] [-B]
        [-o <output directory>] [-c <certificate name>]

Specify -x86, -x64 and/or -ARM64 to build for each platform. If none are
specified, both x64 and x86 will be built. Currently, both the debug and
release versions of Python are required for the installer.

Specify -D to skip rebuilding the documentation. The documentation is
required for a release and the build will fail if it is not available.
Ensure %PYTHON% and %SPHINXBUILD% are set if you omit this option.

Specify -B to skip rebuilding Python. This is useful to only rebuild the
installer layout after a previous call to buildrelease.bat.

Specify -o to set an output directory. The installer layouts will be
copied to platform-specific subdirectories of this path.

Specify -c to choose a code-signing certificate to be used for all the
signable binaries in Python as well as each file making up the
installer. Official releases of Python must be signed.


If WiX is not found on your system, it will be automatically downloaded
and extracted to the externals/ directory.

To manually build layouts of the installer, build one of the projects in
the bundle folder.

    msbuild bundle\snapshot.wixproj
    msbuild bundle\releaseweb.wixproj
    msbuild bundle\releaselocal.wixproj
    msbuild bundle\full.wixproj

snapshot.wixproj produces a test installer versioned based on the date.

releaseweb.wixproj produces a release installer that does not embed any
of the layout.

releaselocal.wixproj produces a release installer that embeds the files
required for a default installation.

full.wixproj produces a test installer that embeds the entire layout.

The following properties may be passed when building these projects.

  /p:BuildForRelease=(true|false)
    When true, adds extra verification to ensure a complete installer is
    produced. Defaults to false.

  /p:ReleaseUri=(any URI)
    Used to generate unique IDs for the installers to allow side-by-side
    installation. Forks of Python can use the same installer infrastructure
    by providing a unique URI for this property. It does not need to be an
    active internet address. Defaults to $(ComputerName).

    Official releases use https://www.python.org/(architecture name)

  /p:DownloadUrlBase=(any URI)
    Specifies the base of a URL where missing parts of the installer layout
    can be downloaded from. The build version and architecture will be
    appended to create the full address. If omitted, missing components will
    not be automatically downloaded.

  /p:DownloadUrl=(any URI)
    Specifies the full URL where missing parts of the installer layout can
    be downloaded from. Should normally include '{2}', which will be
    substituted for the filename. If omitted, missing components will not be
    automatically downloaded. If specified, this value overrides
    DownloadUrlBase.

  /p:SigningCertificate=(certificate name)
    Specifies the certificate to sign the installer layout with. If omitted,
    the layout will not be signed.

  /p:RebuildAll=(true|false)
    When true, rebuilds all of the MSIs making up the layout. Defaults to
    true.

Uploading the Installer
=======================

For official releases, the uploadrelease.bat script should be used.

You will require PuTTY so that plink.exe and pscp.exe can be used, and your
SSH key can be activated in pageant.exe. PuTTY should be either on your path
or in %ProgramFiles(x86)%\PuTTY.

To include signatures for each uploaded file, you will need gpg2.exe on your
path or have run get_externals.bat. You may also need to "gpg2.exe --import"
your key before running the upload script.

    uploadrelease.bat --host <host> --user <username> [--dry-run] [--no-gpg]

The host is the URL to the server. This can be provided by the Release
Manager. You should be able to SSH to this address.

The username is your own username, which you have permission to SSH into
the server containing downloads.

Use --dry-run to display the generated upload commands without executing
them. Signatures for each file will be generated but not uploaded unless
--no-gpg is also passed.

Use --no-gpg to suppress signature generation and upload.

The default target directory (which appears in uploadrelease.proj) is
correct for official Python releases, but may be overridden with
--target <path> for other purposes. This path should generally not include
any version specifier, as that will be added automatically.

Modifying the Installer
=======================

The code for the installer is divided into three main groups: packages,
the bundle and the bootstrap application.

Packages
--------

Packages appear as subdirectories of Tools/msi (other than the bundle/
directory). The project file is a .wixproj and the build output is a
single MSI. Packages are built with the WiX Toolset. Some project files
share source files and use preprocessor directives to enable particular
features. These are typically used to keep the sources close when the
files are related, but produce multiple independent packages.

A package is the smallest element that may be independently installed or
uninstalled (as used in this installer). For example, the test suite has
its own package, as users can choose to add or remove it after the
initial installation.

All the files installed by a single package should be related, though
some packages may not install any files. For example, the pip package
executes the ensurepip package, but does not add or remove any of its
own files. (It is represented as a package because of its
installed/uninstalled nature, as opposed to the "precompile standard
library" option, for example.) Dependencies between packages are handled
by the bundle, but packages should detect when dependencies are missing
and raise an error.

Packages that include a lot of files may use an InstallFiles element in
the .wixproj file to generate sources. See lib/lib.wixproj for an
example, and msi.targets and csv_to_wxs.py for the implementation. This
element is also responsible for generating the code for cleaning up and
removing __pycache__ folders in any directory containing .py files.

All packages are built with the Tools/msi/common.wxs file, and so any
directory or property in this file may be referenced. Of particular
interest:

  REGISTRYKEY (property)
    The registry key for the current installation.

  InstallDirectory (directory)
    The root install directory for the current installation. Subdirectories
    are also specified in this file (DLLs, Lib, etc.)

  MenuDir (directory)
    The Start Menu folder for the current installation.

  UpgradeTable (property)
    Every package should reference this property to include upgrade
    information.

  OptionalFeature (Component)
    Packages that may be enabled or disabled should reference this component
    and have an OPTIONAL_FEATURES entry in the bootstrap application to
    properly handle Modify and Upgrade.

The .wxl_template file is specially handled by the build system for this
project to perform {{substitutions}} as defined in msi.targets. They
should be included in projects as <WxlTemplate> items, where .wxl files
are normally included as <EmbeddedResource> items.

Bundle
------

The bundle is compiled to the main EXE entry point that for most users
will represent the Python installer. It is built from Tools/msi/bundle
with packages references in Tools/msi/bundle/packagegroups.

Build logic for the bundle is in bundle.targets, but should be invoked
through one of the .wixproj files as described in Building the
Installer.

The UI is separated between Default.thm (UI layout), Default.wxl
(strings), bundle.wxs (properties) and the bootstrap application.
Bundle.wxs also contains the chain, which is the list of packages to
install and the order they should be installed in. These refer to named
package groups in bundle/packagegroups.

Each package group specifies one or more packages to install. Most
packages require two separate entries to support both per-user and
all-users installations. Because these reuse the same package, it does
not increase the overall size of the package.

Package groups refer to payload groups, which allow better control over
embedding and downloading files than the default settings. Whether files
are embedded and where they are downloaded from depends on settings
created by the project files.

Package references can include install conditions that determine when to
install the package. When a package is a dependency for others, the
condition should be crafted to ensure it is installed.

MSI packages are installed or uninstalled based on their current state
and the install condition. This makes them most suitable for features
that are clearly present or absent from the user's machine.

EXE packages are executed based on a customisable condition that can be
omitted. This makes them suitable for pre- or post-install tasks that
need to run regardless of whether features have been added or removed.

Bootstrap Application
---------------------

The bootstrap application is a C++ application that controls the UI and
installation. While it does not directly compile into the main EXE of
the installer, it forms the main active component. Most of the
installation functionality is provided by WiX, and so the bootstrap
application is predominantly responsible for the code behind the UI that
is defined in the Default.thm file. The bootstrap application code is in
bundle/bootstrap and is built automatically when building the bundle.

Installation Layout
===================

There are two installation layouts for Python on Windows, with the only
differences being supporting files. A layout is selected implicitly
based on whether the install is for all users of the machine or just for
the user performing the installation.

The default installation location when installing for all users is
"%ProgramFiles%\Python3X" for the 64-bit interpreter and
"%ProgramFiles(x86)%\Python3X-32" for the 32-bit interpreter. (Note that
the latter path is equivalent to "%ProgramFiles%\Python3X-32" when
running a 32-bit version of Windows.) This location requires
administrative privileges to install or later modify the installation.

The default installation location when installing for the current user
is "%LocalAppData%\Programs\Python\Python3X" for the 64-bit interpreter
and "%LocalAppData%\Programs\Python\Python3X-32" for the 32-bit
interpreter. Only the current user can access this location. This
provides a suitable level of protection against malicious modification
of Python's files.

(Default installation locations are set in Tools\msi\bundle\bundle.wxs.)

Within this install directory is the following approximate layout:

.\python[w].exe The core executable files
.\python3x.dll  The core interpreter
.\python3.dll   The stable ABI reference
.\DLLs          Stdlib extensions (*.pyd) and dependencies
.\Doc           Documentation (*.html)
.\include       Development headers (*.h)
.\Lib           Standard library
.\Lib\test      Test suite
.\libs          Development libraries (*.lib)
.\Scripts       Launcher scripts (*.exe, *.py)
.\tcl           Tcl dependencies (*.dll, *.tcl and others)
.\Tools         Tool scripts (*.py)

When installed for all users, the following files are installed to
"%SystemRoot%" (typically "C:\Windows") to ensure they are always
available on PATH. (See Launching Python below.) For the current user,
they are installed in "%LocalAppData%\Programs\Python\PyLauncher".

.\py[w].exe         PEP 397 launcher


System Settings
===============

On installation, registry keys are created so that other applications
can locate and identify installations of Python. The locations of these
keys vary based on the install type.

For 64-bit interpreters installed for all users, the root key is:
    HKEY_LOCAL_MACHINE\Software\Python\PythonCore\3.X

For 32-bit interpreters installed for all users on a 64-bit operating
system, the root key is:
    HKEY_LOCAL_MACHINE\Software\Wow6432Node\Python\PythonCore\3.X-32

For 32-bit interpreters installed for all users on a 32-bit operating
system, the root key is:
    HKEY_LOCAL_MACHINE\Software\Python\PythonCore\3.X-32

For 64-bit interpreters installed for the current user:
    HKEY_CURRENT_USER\Software\Python\PythonCore\3.X

For 32-bit interpreters installed for the current user:
    HKEY_CURRENT_USER\Software\Python\PythonCore\3.X-32

When the core Python executables are installed, a key "InstallPath" is
created within the root key with its default value set to the
executable's install directory. A value named "ExecutablePath" is added
with the full path to the main Python interpreter, and a key
"InstallGroup" is created with its default value set to the product
name "Python 3.X".

When the Python standard library is installed, a key "PythonPath" is
created within the root key with its default value set to the full path
to the Lib folder followed by the path to the DLLs folder, separated by
a semicolon.

When the documentation is installed, a key "Help" is created within the
root key, with a subkey "Main Python Documentation" with its default
value set to the full path to the main index.html file.


The py.exe launcher is installed as part of a regular Python install,
but using a separate mechanism that allows it to more easily span
versions of Python. As a result, it has different root keys for its
registry entries:

When installed for all users on a 64-bit operating system, the
launcher's root key is:
    HKEY_LOCAL_MACHINE\Software\Wow6432Node\Python\Launcher

When installed for all users on a 32-bit operating system, the
launcher's root key is:
    HKEY_LOCAL_MACHINE\Software\Python\Launcher

When installed for the current user:
    HKEY_CURRENT_USER\Software\Python\Launcher

When the launcher is installed, a key "InstallPath" is created within
its root key with its default value set to the launcher's install
directory. File associations are also created for .py, .pyw, .pyc and
.pyo files.

Launching Python
================

When a feature offering user entry points in the Start Menu is
installed, a folder "Python 3.X" is created. Every shortcut should be
created within this folder, and each shortcut should include the version
and platform to allow users to identify the shortcut in a search results
page.

The core Python executables creates a shortcut "Python 3.X (32-bit)" or
"Python 3.X (64-bit)" depending on the interpreter.

The documentation creates a shortcut "Python 3.X 32-bit Manuals" or
"Python 3.X 64-bit Manuals". The documentation is identical for all
platforms, but the shortcuts need to be separate to avoid uninstallation
conflicts.

Installing IDLE creates a shortcut "IDLE (Python 3.X 32-bit)" or "IDLE
(Python 3.X 64-bit)" depending on the interpreter.


For users who often launch Python from a Command Prompt, an option is
provided to add the directory containing python.exe to the user or
system PATH variable. If the option is selected, the install directory
and the Scripts directory will be added at the start of the system PATH
for an all users install and the user PATH for a per-user install.

When the user only has one version of Python installed, this will behave
as expected. However, because Windows searches the system PATH before
the user PATH, users cannot override a system-wide installation of
Python on their PATH. Further, because the installer can only prepend to
the path, later installations of Python will take precedence over
earlier installations, regardless of interpreter version.

Because it is not possible to automatically create a sensible PATH
configuration, users are recommended to use the py.exe launcher and
manually modify their PATH variable to add Scripts directories in their
preferred order. System-wide installations of Python should consider not
modifying PATH, or using an alternative technology to modify their
users' PATH variables.


The py.exe launcher is recommended because it uses a consistent and
sensible search order for Python installations. User installations are
preferred over system-wide installs, and later versions are preferred
regardless of installation order (with the exception that py.exe
currently prefers 2.x versions over 3.x versions without the -3 command
line argument).

For both 32-bit and 64-bit interpreters, the 32-bit version of the
launcher is installed. This ensures that the search order is always
consistent (as the 64-bit launcher is subtly different from the 32-bit
launcher) and also avoids the need to install it multiple times. Future
versions of Python will upgrade the launcher in-place, using Windows
Installer's upgrade functionality to avoid conflicts with earlier
installed versions.

When installed, file associations are created for .py, .pyc and .pyo
files to launch with py.exe and .pyw files to launch with pyw.exe. This
makes Python files respect shebang lines by default and also avoids
conflicts between multiple Python installations.


Repair, Modify and Uninstall
============================

After installation, Python may be modified, repaired or uninstalled by
running the original EXE again or via the Programs and Features applet
(formerly known as Add or Remove Programs).

Modifications allow features to be added or removed. The install
directory and kind (all users/single user) cannot be modified. Because
Windows Installer caches installation packages, removing features will
not require internet access unless the package cache has been corrupted
or deleted. Adding features that were not previously installed and are
not embedded or otherwise available will require internet access.

Repairing will rerun the installation for all currently installed
features, restoring files and registry keys that have been modified or
removed. This operation generally will not redownload any files unless
the cached packages have been corrupted or deleted.

Removing Python will clean up all the files and registry keys that were
created by the installer, as well as __pycache__ folders that are
explicitly handled by the installer. Python packages installed later
using a tool like pip will not be removed. Some components may be
installed by other installers and these will not be removed if another
product has a dependency on them.

