##
## Author: Lasse Collin
##
## This file has been put into the public domain.
## You can do whatever you want with this file.
##

nodist_bin_SCRIPTS = xzdiff xzgrep xzmore xzless
dist_man_MANS = xzdiff.1 xzgrep.1 xzmore.1 xzless.1

links = \
	xzdiff-xzcmp \
	xzgrep-xzegrep \
	xzgrep-xzfgrep

if COND_LZMALINKS
links += \
	xzdiff-lzdiff \
	xzdiff-lzcmp \
	xzgrep-lzgrep \
	xzgrep-lzegrep \
	xzgrep-lzfgrep \
	xzmore-lzmore \
	xzless-lzless
endif

install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
	for pair in $(links); do \
		target=`echo $$pair | sed 's/-.*$$//' | sed '$(transform)'` && \
		link=`echo $$pair | sed 's/^.*-//' | sed '$(transform)'` && \
		rm -f $$link && \
		$(LN_S) $$target $$link; \
	done

install-data-hook:
	cd $(DESTDIR)$(mandir)/man1 && \
	for pair in $(links); do \
		target=`echo $$pair | sed 's/-.*$$//' | sed '$(transform)'` && \
		link=`echo $$pair | sed 's/^.*-//' | sed '$(transform)'` && \
		rm -f $$link.1 && \
		$(LN_S) $$target.1 $$link.1; \
	done

uninstall-hook:
	cd $(DESTDIR)$(bindir) && \
	for pair in $(links); do \
		link=`echo $$pair | sed 's/^.*-//' | sed '$(transform)'` && \
		rm -f $$link; \
	done
	cd $(DESTDIR)$(mandir)/man1 && \
	for pair in $(links); do \
		link=`echo $$pair | sed 's/^.*-//' | sed '$(transform)'` && \
		rm -f $$link.1; \
	done
