##########################
#  User-modifiable configs
##########################

#  Is the resulting package and the installed binary named "python" or
#  "python2"?
#WARNING: Commenting out doesn't work.  Last line is what's used.
%define config_binsuffix none
%define config_binsuffix 2

#  Build tkinter?  "auto" enables it if /usr/bin/wish exists.
#WARNING: Commenting out doesn't work.  Last line is what's used.
%define config_tkinter no
%define config_tkinter yes
%define config_tkinter auto

#  Use pymalloc?  The last line (commented or not) determines wether
#  pymalloc is used.
#WARNING: Commenting out doesn't work.  Last line is what's used.
%define config_pymalloc yes
%define config_pymalloc no

#  Enable IPV6?
#WARNING: Commenting out doesn't work.  Last line is what's used.
%define config_ipv6 yes
%define config_ipv6 no

#################################
#  End of user-modifiable configs
#################################

%define name python
%define version 2.2
%define libvers 2.2
%define release 2
%define __prefix /usr

#  kludge to get around rpm <percent>define weirdness
%define ipv6 %(if [ "%{config_ipv6}" = yes ]; then echo --enable-ipv6; else echo --disable-ipv6; fi)
%define pymalloc %(if [ "%{config_pymalloc}" = yes ]; then echo --with-pymalloc; else echo --without-pymalloc; fi)
%define binsuffix %(if [ "%{config_binsuffix}" = none ]; then echo ; else echo "%{config_binsuffix}"; fi)
%define include_tkinter %(if [ \\( "%{config_tkinter}" = auto -a -f /usr/bin/wish \\) -o "%{config_tkinter}" = yes ]; then echo 1; else echo 0; fi)

Summary: An interpreted, interactive, object-oriented programming language.
Name: %{name}%{binsuffix}
Version: %{version}
Release: %{release}
Copyright: Modified CNRI Open Source License
Group: Development/Languages
Source: Python-%{version}.tgz
Source1: html-%{version}.tar.bz2
Source2: info-%{version}.tar.bz2
Patch0: Python-2.1-pythonpath.patch
Patch1: Python-2.1-expat.patch
BuildRoot: /var/tmp/%{name}-%{version}-root
BuildPrereq: expat-devel
BuildPrereq: db1-devel
BuildPrereq: gdbm-devel
Prefix: %{__prefix}
Packager: Sean Reifschneider <jafo-rpms@tummy.com>

%description
Python is an interpreted, interactive, object-oriented programming
language.  It incorporates modules, exceptions, dynamic typing, very high
level dynamic data types, and classes. Python combines remarkable power
with very clear syntax. It has interfaces to many system calls and
libraries, as well as to various window systems, and is extensible in C or
C++. It is also usable as an extension language for applications that need
a programmable interface.  Finally, Python is portable: it runs on many
brands of UNIX, on PCs under Windows, MS-DOS, and OS/2, and on the
Mac.

%package devel
Summary: The libraries and header files needed for Python extension development.
Prereq: python%{binsuffix} = %{PACKAGE_VERSION}
Group: Development/Libraries

%description devel
The Python programming language's interpreter can be extended with
dynamically loaded extensions and can be embedded in other programs.
This package contains the header files and libraries needed to do
these types of tasks.

Install python-devel if you want to develop Python extensions.  The
python package will also need to be installed.  You'll probably also
want to install the python-docs package, which contains Python
documentation.

%if %{include_tkinter}
%package tkinter
Summary: A graphical user interface for the Python scripting language.
Group: Development/Languages
Prereq: python%{binsuffix} = %{PACKAGE_VERSION}-%{release}

%description tkinter
The Tkinter (Tk interface) program is an graphical user interface for
the Python scripting language.

You should install the tkinter package if you'd like to use a graphical
user interface for Python programming.
%endif

%package tools
Summary: A collection of development tools included with Python.
Group: Development/Tools
Prereq: python%{binsuffix} = %{PACKAGE_VERSION}-%{release}

%description tools
The Python package includes several development tools that are used
to build python programs.  This package contains a selection of those
tools, including the IDLE Python IDE.

Install python-tools if you want to use these tools to develop
Python programs.  You will also need to install the python and
tkinter packages.

%package docs
Summary: Python-related documentation.
Group: Development/Documentation

%description docs
Documentation relating to the Python programming language in HTML and info
formats.

%changelog
* Sun Dec 23 2001 Sean Reifschneider <jafo-rpms@tummy.com>
[Release 2.2-2]
- Added -docs package.
- Added "auto" config_tkinter setting which only enables tk if
  /usr/bin/wish exists.

* Sat Dec 22 2001 Sean Reifschneider <jafo-rpms@tummy.com>
[Release 2.2-1]
- Updated to 2.2.
- Changed the extension to "2" from "2.2".

* Tue Nov 18 2001 Sean Reifschneider <jafo-rpms@tummy.com>
[Release 2.2c1-1]
- Updated to 2.2c1.

* Thu Nov  1 2001 Sean Reifschneider <jafo-rpms@tummy.com>
[Release 2.2b1-3]
- Changed the way the sed for fixing the #! in pydoc works.

* Wed Oct  24 2001 Sean Reifschneider <jafo-rpms@tummy.com>
[Release 2.2b1-2]
- Fixed missing "email" package, thanks to anonymous report on sourceforge.
- Fixed missing "compiler" package.

* Mon Oct 22 2001 Sean Reifschneider <jafo-rpms@tummy.com>
[Release 2.2b1-1]
- Updated to 2.2b1.

* Mon Oct  9 2001 Sean Reifschneider <jafo-rpms@tummy.com>
[Release 2.2a4-4]
- otto@balinor.mat.unimi.it mentioned that the license file is missing.

* Sun Sep 30 2001 Sean Reifschneider <jafo-rpms@tummy.com>
[Release 2.2a4-3]
- Ignacio Vazquez-Abrams pointed out that I had a spruious double-quote in
  the spec files.  Thanks.

* Wed Jul 25 2001 Sean Reifschneider <jafo-rpms@tummy.com>
[Release 2.2a1-1]
- Updated to 2.2a1 release.
- Changed idle and pydoc to use binsuffix macro

#######
#  PREP
#######
%prep
%setup -n Python-%{version}
%patch0 -p1
%patch1

########
#  BUILD
########
%build
./configure %{ipv6} %{pymalloc} --prefix=%{__prefix}
make

##########
#  INSTALL
##########
%install
#  set the install path
echo '[install_scripts]' >setup.cfg
echo 'install_dir='"${RPM_BUILD_ROOT}/usr/bin" >>setup.cfg

[ -d "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{__prefix}/lib/python%{libvers}/lib-dynload
make prefix=$RPM_BUILD_ROOT%{__prefix} install

#  REPLACE PATH IN PYDOC
if [ ! -z "%{binsuffix}" ]
then
   (
      cd $RPM_BUILD_ROOT%{__prefix}/bin
      mv pydoc pydoc.old
      sed 's|#!.*|#!/usr/bin/env python'%{binsuffix}'|' \
            pydoc.old >pydoc
      chmod 755 pydoc
      rm -f pydoc.old
   )
fi

#  add the binsuffix
if [ ! -z "%{binsuffix}" ]
then
   ( cd $RPM_BUILD_ROOT%{__prefix}/bin; rm -f python[0-9a-zA-Z]*;
         mv -f python python"%{binsuffix}" )
   ( cd $RPM_BUILD_ROOT%{__prefix}/man/man1; mv python.1 python%{binsuffix}.1 )
   ( cd $RPM_BUILD_ROOT%{__prefix}/bin; mv -f pydoc pydoc"%{binsuffix}" )
fi

########
#  Tools
echo '#!/bin/bash' >${RPM_BUILD_ROOT}%{_bindir}/idle%{binsuffix}
echo 'exec %{_prefix}/bin/python%{binsuffix} /usr/lib/python%{libvers}/Tools/idle/idle.py' >>$RPM_BUILD_ROOT%{_bindir}/idle%{binsuffix}
chmod 755 $RPM_BUILD_ROOT%{_bindir}/idle%{binsuffix}
cp -a Tools $RPM_BUILD_ROOT%{_prefix}/lib/python%{libvers}

#  MAKE FILE LISTS
rm -f mainpkg.files
find "$RPM_BUILD_ROOT""%{__prefix}"/lib/python%{libvers}/lib-dynload -type f |
	sed "s|^${RPM_BUILD_ROOT}|/|" |
	grep -v -e '_tkinter.so$' >mainpkg.files
find "$RPM_BUILD_ROOT""%{__prefix}"/bin -type f |
	sed "s|^${RPM_BUILD_ROOT}|/|" |
	grep -v -e '/bin/idle%{binsuffix}$' >>mainpkg.files

rm -f tools.files
find "$RPM_BUILD_ROOT""%{__prefix}"/lib/python%{libvers}/Tools -type f |
	sed "s|^${RPM_BUILD_ROOT}|/|" >tools.files
echo "%{__prefix}"/bin/idle%{binsuffix} >>tools.files

######
# Docs
mkdir -p "$RPM_BUILD_ROOT"/var/www/html/python
(
   cd "$RPM_BUILD_ROOT"/var/www/html/python
   bunzip2 < %{SOURCE1} | tar x
)
mkdir -p "$RPM_BUILD_ROOT"/usr/share/info
(
   cd "$RPM_BUILD_ROOT"/usr/share/info
   bunzip2 < %{SOURCE2} | tar x
)

########
#  CLEAN
########
%clean
rm -fr $RPM_BUILD_ROOT
rm -f mainpkg.files tools.files

########
#  FILES
########
%files -f mainpkg.files
%defattr(-,root,root)
%doc Misc/README Misc/HYPE Misc/cheatsheet Misc/unicode.txt Misc/Porting
%doc LICENSE Misc/ACKS Misc/BLURB.* Misc/HISTORY Misc/NEWS
%{__prefix}/man/man1/python%{binsuffix}.1.gz

%dir %{__prefix}/include/python%{libvers}
%dir %{__prefix}/lib/python%{libvers}/
%{__prefix}/lib/python%{libvers}/*.txt
%{__prefix}/lib/python%{libvers}/*.py*
%{__prefix}/lib/python%{libvers}/pdb.doc
%{__prefix}/lib/python%{libvers}/profile.doc
%{__prefix}/lib/python%{libvers}/curses
%{__prefix}/lib/python%{libvers}/distutils
%{__prefix}/lib/python%{libvers}/encodings
%dir %{__prefix}/lib/python%{libvers}/lib-old
%{__prefix}/lib/python%{libvers}/plat-linux2
%{__prefix}/lib/python%{libvers}/site-packages
%{__prefix}/lib/python%{libvers}/test
%{__prefix}/lib/python%{libvers}/xml
%{__prefix}/lib/python%{libvers}/email
%{__prefix}/lib/python%{libvers}/compiler

%files devel
%defattr(-,root,root)
%{__prefix}/include/python%{libvers}/*.h
%{__prefix}/lib/python%{libvers}/config

%files -f tools.files tools
%defattr(-,root,root)

%if %{include_tkinter}
%files tkinter
%defattr(-,root,root)
%{__prefix}/lib/python%{libvers}/lib-tk
%{__prefix}/lib/python%{libvers}/lib-dynload/_tkinter.so*
%endif

%files docs
%defattr(-,root,root)
/var/www/html/python
/usr/share/info
