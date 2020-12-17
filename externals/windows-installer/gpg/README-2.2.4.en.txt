                   English README file for Gpg4win
                   ===============================

This is Gpg4win, version 2.2.4 (2015-03-18).

Content:

     1. Important Notes
     2. Changes
     3. Known Bugs (and Workarounds)
     4. Installation
     5. Version History
     6. Version Numbers of Included Software
     7. Legal Notices


1. Important Notes
==================

The Gpg4win Compendium describes the installation and use of Gpg4win.
After installation it is available in the Gpg4win start menu or online:
http://www.gpg4win.org/doc/en/gpg4win-compendium.html

Please read the section "3. Known Bugs (and Workarounds)" of this
README before you start working with Gpg4win.

Gpg4win supports these platforms:

  * Operating System: Windows XP, Vista, 7, 8 (for all: 32/64 bit)

  * MS Outlook: 2003, 2007, 2010, 2013 (for all: only 32bit!)


2. Changes
==========

Included Gpg4win components in Version 2.2.4 are:

    GnuPG:          2.0.27
    Kleopatra:      2.2.0-git945878c
    GPA:            0.9.7
    GpgOL:          1.2.1
    GpgEX:          1.0.1
    Claws-Mail:     3.9.1
    Kompendium DE:  3.0.0
    Kompendium EN:  3.0.0


New in Gpg4win Version 2.2.4 (2015-03-18)
-----------------------------------------

- GnuPG has been updated to version 2.0.27. See release notes on
  www.gnupg.org for details.

- Libgcrypt has been updated to version 1.6.3 (includes fix for
  CVE-2014-3591)

- An issue has been fixed which could cause extracted files from
  TAR Archives to be truncated (issue#1926).


3. Known Bugs (and Workarounds)
===============================

- Using smart cards with Kleopatra:

   * OpenPGP card
     The initial setting of a smart card with Kleopatra is not yet
     possible. Please run the following steps once to use your smart
     card with Kleopatra.
     Use the gpg command line tool to create a new OpenPGP certificate
     on your card (a) or to activate your existing certificate of
     your card (b):

     (a) Create new certificate
       - Insert card.
       - Run "gpg --card-edit".
       - Switch to admin modus by enter: "admin".
       - Enter "generate" to create a new certificate.

     (b) Activate existing certificate of your card
       - Import associated (public) certificate of your card (e.g.
         from certificate server or from a exported certificate file).
       - Insert card.
       - Run "gpg --card-status".

   * X.509 Telesec Netkey 3 card
     Use Kleopatra to initialize your card:
     - Insert card.
     - Click on the flashing Kleopatra system tray icon (or use the 
       context menu "smart card" of the system tray icon and run the 
       "learn card" command directly).

   After finishing these steps your OpenPGP / X.509 certificate from your
   smart card is shown in Kleopatra under the tab "My certificates" 
   (marked with a smart card icon).

- Using the Outlook Plugin "GpgOL":

  * You defintely should create copies of your old encrypted/signed
    emails before installing GpgOL, e.g. in the form of PST files.

  * For Outlook 2003/2007 only:
    Sending signed or encrypted messages via an Exchange based account
    does not yet work.
    [see https://bugs.g10code.com/gnupg/issue1102]
    (Please note, using SMTP with GpgOL and Exchange seems to be work.
    Or use GpgOL with Outlook 2010/2013.)

  * For Outlook 2003/2007 only:
    Encrypted E-Mails occuring un-encrypted on the email server: It
    can happen that parts of encrypted emails are copied to your email
    server (IMAP or MAPI) in un-encrypted/decrypted form when creating
    or viewing them.  Affected is the content of the email view
    window, thus usually the so-called email body.  Attachments are
    not affected.  Switching off the Outlook preview will lower the
    probability of this to happen, but not eliminate the issue.
    Or use GpgOL with Outlook 2010/2013.


4. Installation
===============

For installation instructions please read the new Gpg4win Compendium:
http://www.gpg4win.org/doc/en/gpg4win-compendium_11.html

Hints for automated installations (without user dialogs):
http://www.gpg4win.org/doc/en/gpg4win-compendium_35.html


5. Version History
==================

Listed below are the changes as recorded in the source distribution's
NEWS file. An up-to-date list of changes is also available at:
http://www.gpg4win.org/change-history.html

Noteworthy changes in version 2.2.4 (2015-03-17)
------------------------------------------------
   * GnuPG has been updated to version 2.0.27. See release notes on
     www.gnupg.org for details.



6. Version Numbers of Included Software
=======================================

=========== SHA-1 checksum ============= == package ==

aecd6213118f01aa38f535dc3bafc31b1e7c1c21 adns-1.4-g10-5.tar.bz2

3c31c9d6b19af840e2bd8ccbfef4072a6548dc4e atk-1.32.0.zip

d0b8c53e01e4541d3d3befc82e41fb5b84949030 atk-dev-1.32.0.zip

6e38be3377340a21a1f13ff84b5e6adce97cd1d4 bzip2-1.0.6-g10.tar.gz

d44cd66a9f4d7d29a8f2c28d1c1c5f9b0525ba44 cairo-1.10.2.zip

45cae1fac45a6c6f33498c37c0cdc47722614d92 cairo-dev-1.10.2.zip

ec3a787b34b07983d938f7d353e3cfd85167122b claws-mail-3.9.1.tar.bz2

04b4c2ac296499d378f4a35bbeb1b129165b2e61 crypt-1.1.tar.gz

3bde6fb2e599197e9579c0735ad255c1ddbd914d curl-7.37.0.tar.bz2

59abdc742ce87011dadbc58e96ed870a927d0045 dbus-x86-mingw4-1.4.24-20130417-1-bin.tar.bz2

fc557d7eb161881e1d6c091f215dd8e0615682cb dbus-x86-mingw4-1.4.24-20130417-lib.tar.bz2

e708d4aa5ce852f4de3f4b58f4e4f221f5e5c690 dirmngr-1.1.1.tar.bz2

321f9cf0abfa1937401676ce60976d8779c39536 enchant-1.6.0.tar.gz

f47790b9e324cd8613acc9a17fd56bf2c14745fc expat-2.0.1.zip

2e9189c6c6d1dac847a47c537c7a5e9dffd91992 expat-dev-2.0.1.zip

37a3117ea6cc50c8a88fba9b6018f35a04fa71ce fontconfig-2.8.0.zip

0b772aaeb0a7a0d5de21afd901d6cf00753efa51 fontconfig-dev-2.8.0.zip

c20ab9ff053fe59f73612fd392f6e6dc01af614a freetype-2.4.2.zip

00e877d7ec7c416e2b48a392324a5502019a20bf freetype-dev-2.4.2.zip

6277b4e5b5e334b3669f15ae0376e184be9e8cd8 gdk-pixbuf-2.30.8.tar.xz

e0d425de1bd1a16993b262ff37eaf08abee8f953 gettext-0.19.1.tar.xz

d3b119707786f84496c366f143e6e70e95370d32 glib-2.41.0.tar.xz

d065be185f5bac8ea07b210ab7756e79b83b63d4 gnupg2-2.0.27.tar.bz2

3c0ba2153560abfb08d88dcb016cd6b72e465db5 gnutls-2.12.23.tar.bz2

9eb07bcceeb986c7b6dbce8a18b82a2c344b50ce gpa-0.9.7.tar.bz2

eb54767fd8e3728e8d14c7c158e0841b67c714a6 gpgex-1.0.1.tar.bz2

8dd7711a4de117994fe2d45879ef8a9900d50f6a gpgme-1.5.3.tar.bz2

4cf226bc452ffefbce82abada99d23f72a9f1e0e gpgol-1.2.1.tar.bz2

c43eb248b3d30c6b49937692b4c4bfa10b96201e gtk+-2.24.24.tar.xz

7f3f8a9e3f7bec443a51c890d1bbcd26049dde02 gtkhtml2_viewer-0.34.tar.gz

18c9bfd2ba2ceeb0ce605cb45a2c41a35d5d5eb2 kleopatra-20141125-1-bin.tar.xz

7cf0545955ce414044bb99b871d324753dd7b2e5 libassuan-2.2.0.tar.bz2

d7195498005d340ccd82e183de19163d16e56ec2 libetpan-0.58-g10-1.tar.bz2

f5230890dc0be42fb5c58fbf793da253155de106 libffi-3.0.13.tar.gz

9456e7b64db9df8360a1407a38c8c958da80bbf1 libgcrypt-1.6.3.tar.bz2

50fbff11446a7b0decbf65a6e6b0eda17b5139fb libgpg-error-1.13.tar.bz2

08fd5dfdd3d88154cf06cb0759a732790c47b4f7 libgsasl-1.8.0.tar.gz

be7d67e50d72ff067b2c0291311bc283add36965 libiconv-1.14.tar.gz

37d0893a587354af2b6e49f6ae701ca84f52da67 libksba-1.3.2.tar.bz2

accc4d6aaac1e35d74af84ee7246c0b0eed0bee4 libpng-1.4.15.tar.xz

22f9e0b15f870c8e03ac9cc1ead969d4d84eb931 libtasn1-2.14.tar.gz

eb3e2146c6d68aea5c2a4422ed76fe196f933c21 libxml2-2.9.1.tar.gz

744dbbc7585205948fa62d63c9dbb8c6dd8fc9fb oxygen-icons-20141125-bin.tar.bz2

3959319bd04fbce513458857f334ada279b8cdd4 pango-1.29.4.zip

49ae12458f2e29c27ed9d1390d95db18fd4a49ac pango-dev-1.29.4.zip

16af56d0e7bdf081d60c59ea4d72e7df6d9cec21 paperkey-1.3.tar.gz

f8e5c774c35fbb91d84e82559baf76f6b4513236 pinentry-0.9.0.tar.bz2

d063e705812e1ee7feb8f35d51b3cad04ca13b0d pkgconfig-0.23.zip

da8371cb20e8e238f96a1d0651212f154d84a9ac pthreads-w32-2-8-0-release.tar.gz

657c833fa39a93c6d7bfccc03c9fb88df4746136 qt-x86-mingw4-4.8.6-bin.tar.bz2

bc7fc185fd54259be25833b4c482a5680279cdae qt-x86-mingw4-4.8.6-lib.tar.bz2

56c87366f49916997729fc335747a6a55a2bacc3 regex-20090805.tar.gz

e28141d2b03612c09512651795976c58ed3f8035 scute-1.4.0.tar.bz2

d648b98ce215f81e901f3f982470d37c704433a6 w32pth-2.0.5.tar.bz2

a4d316c404ff54ca545ea71a27af7dbc29817088 zlib-1.2.8.tar.gz



7. Legal notices pertaining to the individual packets
=====================================================

Gpg4win consist of several independent developed packages, available
under different license conditions.  Most of these packages however
are available under the GNU General Public License (GNU GPL).  Common
to all is that they are free to use without restrictions, may be
modified and that modifications may be distributed.  If the source
files (i.e. gpg4win-src-x.y.z.exe) are distributed along with the
binaries and the use of the GNU GPL has been pointed out, distribution
is in in all cases possible.

What follows is a list of copyright statements.

Here is a list with collected copyright notices.  For details see the
description of each individual package.  This is not meant as an
exhaustive list of copyright notices.  Notices from several packages
are even not listed.  The most restrictive requirements are those of
the GNU General Public License version 3 (GPLv3+); thus complying to
those terms and conditions should be sufficient.  If in doubt check
the individual packages.


Gpg4win (the installer) is

  Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2013 g10 Code GmbH

  Gpg4win is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gpg4win is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA


GnuPG is

  Copyright 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005,
            2006, 2007, 2008, 2009, 2010, 2011, 2012,
            2013 Free Software Foundation, Inc.

  GnuPG is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  GnuPG is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  See the files AUTHORS and THANKS for credits, further legal
  information and bug reporting addresses pertaining to GnuPG.


NSIS is

  Copyright 1999-2009 Nullsoft, Jeff Doozan and Contributors
  Copyright 2002-2008 Amir Szekely
  Copyright 2003 Ramon
  Copyright 1995-1998 Jean-loup Gailly and Mark Adler
  Copyright 1999-2006 Igor Pavlov
  Copyright 1996-2000 Julian R Seward

  This license applies to everything in the NSIS package, except where
  otherwise noted.

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any
  damages arising from the use of this software.

  Permission is granted to anyone to use this software for any
  purpose, including commercial applications, and to alter it and
  redistribute it freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must
     not claim that you wrote the original software. If you use this
     software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  3. This notice may not be removed or altered from any source
     distribution.

  The user interface used with the installer is

  Copyright (C) 2002-2005 Joost Verburg

  [It is distributed along with NSIS and the same conditions as stated
   above apply]


GLIB is

  Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
  Copyright (C) 1995-2011 Red Hat, Inc.
  Copyright (C) 2008-2010 Novell, Inc.
  Copyright (C) 2008-2010 Codethink Limited.
  Copyright (C) 2008-2010 Collabora, Ltd.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.

  See the AUTHORS file for a list of people on the GLib Team.  See the
  ChangeLog files for a list of changes.  These files are distributed
  with GLib at ftp://ftp.gtk.org/pub/gtk/.


GPA is

  Copyright (C) 2000-2002 G-N-U GmbH (http://www.g-n-u.de)
  Copyright (C) 2002-2003 Miguel Coca.
  Copyright (C) 2005-2013 g10 Code GmbH

  GPA uses fragments from the following programs and libraries:
  JNLIB, Copyright (C) 1998-2000 Free Software Foundation, Inc.
  GPGME, Copyright (C) 2000-2001 Werner Koch
  WinPT, Copyright (C) 2000-2002 Timo Schulz
  (For details, see the file `AUTHORS'.)

  GPA is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  GPA is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.


GPGEX is

  Copyright (C) 2007 g10 Code GmbH

  GpgEX is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  GpgEX is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.


GPGME is

  Copyright (C) 2000 Werner Koch
  Copyright (C) 2001--2013 g10 Code GmbH

  GPGME is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of
  the License, or (at your option) any later version.

  GPGME is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.

  See the files AUTHORS and THANKS for credits, further legal
  information and bug reporting addresses pertaining to GPGME.


GpgOL is

  Copyright (C) 2004, 2005, 2007, 2008, 2009, 2010,
                2011 g10 Code GmbH

  GpgOL is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  GpgOL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.

  See the files AUTHORS and THANKS for credits, further legal
  information and bug reporting addresses pertaining to GpgOL.


GTK+ is

  Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.

  Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
  file for a list of people on the GTK+ Team.  See the ChangeLog
  files for a list of changes.  These files are distributed with
  GTK+ at ftp://ftp.gtk.org/pub/gtk/.


LIBGCRYPT is

  Copyright 2000, 2002, 2003, 2004, 2007, 2008, 2009,
            2010, 2011, 2012 Free Software Foundation, Inc.
  Copyright 2012, 2013 g10 Code GmbH

  Libgcrypt is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser general Public License as
  published by the Free Software Foundation; either version 2.1 of
  the License, or (at your option) any later version.

  Libgcrypt is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.


LIBGPG-ERROR is

  Copyright 2003, 2004, 2010, 2013 g10 Code GmbH

  libgpg-error is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  as published by the Free Software Foundation; either version 2.1 of
  the License, or (at your option) any later version.

  libgpg-error is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.


Pthreads-win32 is

  Copyright(C) 1998 John E. Bossom
  Copyright(C) 1999,2005 Pthreads-win32 contributors

  Most of this is work available under the GNU Lesser General Public
  License as published by the Free Software Foundation version 2.1 of
  the License.  The detailed terms are given in the file COPYING in
  the source distribution; that very file may not be modified and thus
  it is not possible to include it here.


Claws Mail is

  Copyright(C) 1999-2013 Hiroyuki Yamamoto <hiro-y@kcn.ne.jp> and the
  Claws Mail Team

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

  For more details see the file COPYING.


BZIP2 is

  This program, "bzip2", the associated library "libbzip2", and all
  documentation, are copyright (C) 1996-2006 Julian R Seward.  All
  rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. The origin of this software must not be misrepresented; you must
     not claim that you wrote the original software.  If you use this
     software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.

  3. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  4. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior written
     permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Julian Seward, Cambridge, UK.
  jseward@bzip.org
  bzip2/libbzip2 version 1.0.4 of 20 December 2006


ADNS

  adns is Copyright 2008 g10 Code GmbH, Copyright 1997-2000,2003,2006
  Ian Jackson, Copyright 1999-2000,2003,2006 Tony Finch, and Copyright
  (C) 1991 Massachusetts Institute of Technology.

  adns is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program and documentation is distributed in the hope that it will
  be useful, but without any warranty; without even the implied warranty
  of merchantability or fitness for a particular purpose. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with adns, or one should be available above; if not, write to
  the Free Software Foundation, 59 Temple Place - Suite 330, Boston,
  MA 02111-1307, USA, or email adns-maint@chiark.greenend.org.uk.


Paperkey

  Copyright (C) 2007, 2008, 2009 David Shaw <dshaw@jabberwocky.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.

  The included man page is

  Copyright (C) 2007 Peter Palfrader <peter@palfrader.org>

  Examples have been taken from David Shaw's README. The license is
  the same as for Paperkey.


Scute

  Copyright 2006, 2008 g10 Code GmbH

  Scute is licensed under the GNU General Pubic License, either
  version 2, or (at your option) any later version with this special
  exception:

  In addition, as a special exception, g10 Code GmbH gives permission
  to link this library: with the Mozilla Foundation's code for
  Mozilla (or with modified versions of it that use the same license
  as the "Mozilla" code), and distribute the linked executables.  You
  must obey the GNU General Public License in all respects for all of
  the code used other than "Mozilla".  If you modify the software, you
  may extend this exception to your version of the software, but you
  are not obligated to do so.  If you do not wish to do so, delete this
  exception statement from your version and from all source files.


[Compiled by wk 2006-02-15, last updated 2013-05-13]




Happy GiPiGing,

  Your Gpg4win Team


***end of file ***
