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
	cd "$(DESTDIR)$(bindir)" && \
	for pair in $(links); do \
		target=`echo $$pair | sed 's/-.*$$//' | sed '$(transform)'` && \
		link=`echo $$pair | sed 's/^.*-//' | sed '$(transform)'` && \
		rm -f "$$link" && \
		$(LN_S) "$$target" "$$link"; \
	done

# The installation of translated man pages abuses Automake internals
# by calling "install-man" with redefined dist_man_MANS and man_MANS.
# If this breaks some day, don't blame Automake developers.
install-data-hook:
	languages= ; \
	if test "$(USE_NLS)" = yes && test -d "$(top_srcdir)/po4a/man"; then \
		languages=`ls "$(top_srcdir)/po4a/man"`; \
	fi; \
	for lang in $$languages; do \
		mans= ; \
		for man in $(dist_man_MANS); do \
			man="$(top_srcdir)/po4a/man/$$lang/$$man" ; \
			if test -f "$$man"; then \
				mans="$$mans $$man"; \
			fi; \
		done; \
		$(MAKE) dist_man_MANS="$$mans" man_MANS= \
				mandir="$(mandir)/$$lang" install-man; \
	done; \
	for lang in . $$languages; do \
		for pair in $(links); do \
			target=`echo $$pair | sed 's/-.*$$//' \
					| sed '$(transform)'` && \
			link=`echo $$pair | sed 's/^.*-//' \
					| sed '$(transform)'` && \
			man1dir="$(DESTDIR)$(mandir)/$$lang/man1" && \
			if test -f "$$man1dir/$$target.1"; then ( \
				cd "$$man1dir" && \
				rm -f "$$link.1" && \
				$(LN_S) "$$target.1" "$$link.1" \
			); fi; \
		done; \
	done

uninstall-hook:
	cd "$(DESTDIR)$(bindir)" && \
	for pair in $(links); do \
		link=`echo $$pair | sed 's/^.*-//' | sed '$(transform)'` && \
		rm -f "$$link"; \
	done
	languages= ; \
	if test "$(USE_NLS)" = yes && test -d "$(top_srcdir)/po4a/man"; then \
		languages=`ls "$(top_srcdir)/po4a/man"`; \
	fi; \
	for lang in . $$languages; do \
		for pair in $(links); do \
			target=`echo $$pair | sed 's/-.*$$//' \
					| sed '$(transform)'` && \
			link=`echo $$pair | sed 's/^.*-//' \
					| sed '$(transform)'` && \
			rm -f "$(DESTDIR)$(mandir)/$$lang/man1/$$target.1" \
				"$(DESTDIR)$(mandir)/$$lang/man1/$$link.1"; \
		done; \
	done
