%define name BeOpen-Python
%define version 2.0
%define release 1
%define __prefix /usr/local

Summary: An interpreted, interactive, object-oriented programming language.
Name: %{name}
Version: %{version}
Release: %{release}
Copyright: Modified CNRI Open Source License
Group: Development/Languages
Source: %{name}-%{version}.tar.bz2
Source1: html-%{version}.tar.bz2
Patch0: %{name}-%{version}-Setup.patch
BuildRoot: /var/tmp/%{name}-%{version}-root
Prefix: %{__prefix}
URL: http://www.pythonlabs.com/
Vendor: BeOpen PythonLabs
Packager: Jeremy Hylton <jeremy@beopen.com>

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

%changelog
* Mon Oct  9 2000 Jeremy Hylton <jeremy@beopen.com>
- updated for 2.0c1
- build audioop, imageop, and rgbimg extension modules
- include xml.parsers subpackage
- add test.xml.out to files list

* Thu Oct  5 2000 Jeremy Hylton <jeremy@beopen.com>
- added bin/python2.0 to files list (suggested by Martin v. Löwis)

* Tue Sep 26 2000 Jeremy Hylton <jeremy@beopen.com>
- updated for release 1 of 2.0b2
- use .bz2 version of Python source

* Tue Sep 12 2000 Jeremy Hylton <jeremy@beopen.com>
- Version 2 of 2.0b1
- Make the package relocatable.  Thanks to Suchandra Thapa.
- Exclude Tkinter from main RPM.  If it is in a separate RPM, it is
  easier to track Tk releases.

%prep
%setup -n Python-%{version}
%patch0 
%setup -D -T -a 1 -n Python-%{version}
# This command drops the HTML files in the top-level build directory.
# That's not perfect, but it will do for now.

%build
./configure
make

%install
[ -d $RPM_BUILD_ROOT ] && rm -fr $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{__prefix}
make prefix=$RPM_BUILD_ROOT%{__prefix} install

%clean
rm -fr $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%{__prefix}/bin/python
%{__prefix}/bin/python2.0
%{__prefix}/man/man1/python.1
%doc Misc/README Misc/HYPE Misc/cheatsheet Misc/unicode.txt Misc/Porting
%doc LICENSE Misc/ACKS Misc/BLURB.* Misc/HISTORY Misc/NEWS
%doc index.html modindex.html api dist doc ext inst lib mac ref tut icons

%dir %{__prefix}/include/python2.0
%{__prefix}/include/python2.0/*.h
%dir %{__prefix}/lib/python2.0/
%{__prefix}/lib/python2.0/*.py*
%{__prefix}/lib/python2.0/pdb.doc
%{__prefix}/lib/python2.0/profile.doc
%dir %{__prefix}/lib/python2.0/config
%{__prefix}/lib/python2.0/config/Makefile
%{__prefix}/lib/python2.0/config/Makefile.pre.in
%{__prefix}/lib/python2.0/config/Setup
%{__prefix}/lib/python2.0/config/Setup.config
%{__prefix}/lib/python2.0/config/Setup.local
%{__prefix}/lib/python2.0/config/config.c
%{__prefix}/lib/python2.0/config/config.c.in
%{__prefix}/lib/python2.0/config/install-sh
%{__prefix}/lib/python2.0/config/libpython2.0.a
%{__prefix}/lib/python2.0/config/makesetup
%{__prefix}/lib/python2.0/config/python.o
%dir %{__prefix}/lib/python2.0/curses
%{__prefix}/lib/python2.0/curses/*.py*
%dir %{__prefix}/lib/python2.0/distutils
%{__prefix}/lib/python2.0/distutils/*.py*
%{__prefix}/lib/python2.0/distutils/README
%dir %{__prefix}/lib/python2.0/distutils/command
%{__prefix}/lib/python2.0/distutils/command/*.py*
%{__prefix}/lib/python2.0/distutils/command/command_template
%dir %{__prefix}/lib/python2.0/encodings
%{__prefix}/lib/python2.0/encodings/*.py*
%dir %{__prefix}/lib/python2.0/lib-dynload
%dir %{__prefix}/lib/python2.0/lib-tk
%{__prefix}/lib/python2.0/lib-tk/*.py*
%{__prefix}/lib/python2.0/lib-dynload/_codecsmodule.so
%{__prefix}/lib/python2.0/lib-dynload/_cursesmodule.so
%{__prefix}/lib/python2.0/lib-dynload/_localemodule.so
%{__prefix}/lib/python2.0/lib-dynload/arraymodule.so
%{__prefix}/lib/python2.0/lib-dynload/audioop.so
%{__prefix}/lib/python2.0/lib-dynload/binascii.so
%{__prefix}/lib/python2.0/lib-dynload/cPickle.so
%{__prefix}/lib/python2.0/lib-dynload/cStringIO.so
%{__prefix}/lib/python2.0/lib-dynload/cmathmodule.so
%{__prefix}/lib/python2.0/lib-dynload/errnomodule.so
%{__prefix}/lib/python2.0/lib-dynload/fcntlmodule.so
%{__prefix}/lib/python2.0/lib-dynload/gdbmmodule.so
%{__prefix}/lib/python2.0/lib-dynload/grpmodule.so
%{__prefix}/lib/python2.0/lib-dynload/imageop.so
%{__prefix}/lib/python2.0/lib-dynload/linuxaudiodev.so
%{__prefix}/lib/python2.0/lib-dynload/mathmodule.so
%{__prefix}/lib/python2.0/lib-dynload/md5module.so
%{__prefix}/lib/python2.0/lib-dynload/mmapmodule.so
%{__prefix}/lib/python2.0/lib-dynload/newmodule.so
%{__prefix}/lib/python2.0/lib-dynload/operator.so
%{__prefix}/lib/python2.0/lib-dynload/parsermodule.so
%{__prefix}/lib/python2.0/lib-dynload/pwdmodule.so
%{__prefix}/lib/python2.0/lib-dynload/pyexpat.so
%{__prefix}/lib/python2.0/lib-dynload/readline.so
%{__prefix}/lib/python2.0/lib-dynload/resource.so
%{__prefix}/lib/python2.0/lib-dynload/rgbimgmodule.so
%{__prefix}/lib/python2.0/lib-dynload/rotormodule.so
%{__prefix}/lib/python2.0/lib-dynload/selectmodule.so
%{__prefix}/lib/python2.0/lib-dynload/shamodule.so
%{__prefix}/lib/python2.0/lib-dynload/_socketmodule.so
%{__prefix}/lib/python2.0/lib-dynload/stropmodule.so
%{__prefix}/lib/python2.0/lib-dynload/structmodule.so
%{__prefix}/lib/python2.0/lib-dynload/syslogmodule.so
%{__prefix}/lib/python2.0/lib-dynload/termios.so
%{__prefix}/lib/python2.0/lib-dynload/timemodule.so
%{__prefix}/lib/python2.0/lib-dynload/ucnhash.so
%{__prefix}/lib/python2.0/lib-dynload/unicodedata.so
%{__prefix}/lib/python2.0/lib-dynload/zlibmodule.so
%dir %{__prefix}/lib/python2.0/lib-old
%{__prefix}/lib/python2.0/lib-old/*.py*
%dir %{__prefix}/lib/python2.0/plat-linux2
%{__prefix}/lib/python2.0/plat-linux2/*.py*
%{__prefix}/lib/python2.0/plat-linux2/regen
%dir %{__prefix}/lib/python2.0/site-packages
%{__prefix}/lib/python2.0/site-packages/README
%dir %{__prefix}/lib/python2.0/test
%{__prefix}/lib/python2.0/test/*.py*
%{__prefix}/lib/python2.0/test/README
%{__prefix}/lib/python2.0/test/audiotest.au
%{__prefix}/lib/python2.0/test/greyrgb.uue
%{__prefix}/lib/python2.0/test/test.xml
%{__prefix}/lib/python2.0/test/test.xml.out
%{__prefix}/lib/python2.0/test/testimg.uue
%{__prefix}/lib/python2.0/test/testimgr.uue
%{__prefix}/lib/python2.0/test/testrgb.uue
%dir %{__prefix}/lib/python2.0/test/output
%{__prefix}/lib/python2.0/test/output/test_*
%dir %{__prefix}/lib/python2.0/xml
%{__prefix}/lib/python2.0/xml/*.py*
%dir %{__prefix}/lib/python2.0/xml/dom
%{__prefix}/lib/python2.0/xml/dom/*.py*
%dir %{__prefix}/lib/python2.0/xml/parsers
%{__prefix}/lib/python2.0/xml/parsers/*.py*
%dir %{__prefix}/lib/python2.0/xml/sax
%{__prefix}/lib/python2.0/xml/sax/*.py*
