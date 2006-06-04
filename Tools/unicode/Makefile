#
# Recreate the Python charmap codecs from the Unicode mapping
# files available at ftp://ftp.unicode.org/
#
#(c) Copyright Marc-Andre Lemburg, 2005.
#    Licensed to PSF under a Contributor Agreement.

# Python binary to use
PYTHON = python

# Remove tool to use
RM = /bin/rm

### Generic targets

all:	distclean mappings codecs

codecs:	misc windows iso apple ebcdic custom-mappings cjk

### Mappings

mappings:
	ncftpget -R ftp.unicode.org . Public/MAPPINGS

### Codecs

build/:
	mkdir build

misc:	build/
	$(PYTHON) gencodec.py MAPPINGS/VENDORS/MISC/ build/
	$(RM) build/atarist.*
	$(RM) build/us_ascii_quotes.*
	$(RM) build/ibmgraph.*
	$(RM) build/sgml.*
	$(RM) -f build/readme.*

custom-mappings:	build/
	$(PYTHON) gencodec.py python-mappings/ build/

windows:	build/
	$(PYTHON) gencodec.py MAPPINGS/VENDORS/MICSFT/WINDOWS/ build/
	$(RM) build/cp9*
	$(RM) -f build/readme.*

iso:	build/
	$(PYTHON) gencodec.py MAPPINGS/ISO8859/ build/ iso
	$(RM) -f build/isoreadme.*

apple:	build/
	$(PYTHON) gencodec.py MAPPINGS/VENDORS/APPLE/ build/ mac_
	$(RM) build/mac_dingbats.*
	$(RM) build/mac_japanese.*
	$(RM) build/mac_chin*
	$(RM) build/mac_korean.*
	$(RM) build/mac_symbol.*
	$(RM) build/mac_corpchar.*
	$(RM) build/mac_devanaga.*
	$(RM) build/mac_gaelic.*
	$(RM) build/mac_gurmukhi.*
	$(RM) build/mac_hebrew.*
	$(RM) build/mac_inuit.*
	$(RM) build/mac_thai.*
	$(RM) build/mac_ukraine.*
	$(RM) build/mac_arabic.py
	$(RM) build/mac_celtic.*
	$(RM) build/mac_gujarati.*
	$(RM) build/mac_keyboard.*
	$(RM) -f build/mac_readme.*

ebcdic:	build/
	$(PYTHON) gencodec.py MAPPINGS/VENDORS/MICSFT/EBCDIC/ build/
	$(RM) -f build/readme.*

cjk:	build/
	$(PYTHON) gencjkcodecs.py build/

### Cleanup

clean:
	$(RM) -f build/*

distclean:	clean
	$(RM) -rf MAPPINGS/
