# Android/external-libraries/Makefile

NCURSES := ncurses-6.0
READLINE := readline-6.3
SQLITE := sqlite3
SQLITE_FNAME := sqlite-autoconf-3080803
LIBFFI := libffi-3.2.1
BUILD_DIR := $(BUILD_DIR)/external-libraries
py_install_dir := $(PY_EXTDIR)/$(SYS_EXEC_PREFIX)

libffi: libffi_header $(py_install_dir)/lib/libffi.a
libffi_header:
	@echo "---> Build the $(LIBFFI) external library."
ncurses: ncurses_header $(py_install_dir)/lib/libncursesw.a
ncurses_header:
	@echo "---> Build the $(NCURSES) external library."
readline: readline_header $(py_install_dir)/lib/libreadline.a
readline_header:
	@echo "---> Build the $(READLINE) external library."
sqlite: sqlite_header $(py_install_dir)/lib/libsqlite3.a
sqlite_header:
	@echo "---> Build the sqlite external library (from $(SQLITE_FNAME).tar.gz)."

$(BUILD_DIR)/$(NCURSES).tar.gz:
	cd $(BUILD_DIR); \
	    $(DOWNLOAD) http://ftp.gnu.org/pub/gnu/ncurses/$(NCURSES).tar.gz
$(BUILD_DIR)/$(READLINE).tar.gz:
	cd $(BUILD_DIR); \
	    $(DOWNLOAD) ftp://ftp.gnu.org/gnu/readline/$(READLINE).tar.gz
$(BUILD_DIR)/$(SQLITE_FNAME).tar.gz:
	cd $(BUILD_DIR); \
	    $(DOWNLOAD) https://sqlite.org/2015/$(SQLITE_FNAME).tar.gz
$(BUILD_DIR)/$(LIBFFI).tar.gz:
	cd $(BUILD_DIR); \
	    $(DOWNLOAD) ftp://sourceware.org/pub/libffi/$(LIBFFI).tar.gz

$(py_install_dir)/lib/libncursesw.a: $(BUILD_DIR)/$(NCURSES).tar.gz
	rm -rf $(BUILD_DIR)/$(NCURSES)-$(BUILD_TYPE)
	tar xzf $< -C $(BUILD_DIR)
	cd $(BUILD_DIR)/$(NCURSES); patch -p0 < $(CURDIR)/patches/ncurses.patch
	mv $(BUILD_DIR)/$(NCURSES) $(BUILD_DIR)/$(NCURSES)-$(BUILD_TYPE)
	cd $(BUILD_DIR)/$(NCURSES)-$(BUILD_TYPE); \
	    ./configure --host=$(ANDROID_HOST) --build=$(ANDROID_BUILD) \
	    --prefix=$(py_install_dir) \
	    --without-ada --without-cxx --without-manpages \
	    --without-progs --without-tests \
	    --without-termlib --enable-widec \
	    --with-shared --without-shlib-version --without-gpm
	$(MAKE) -C $(BUILD_DIR)/$(NCURSES)-$(BUILD_TYPE) all install
	@echo Fix symlinks for Python _curses and _curses_panel extensions.
	cd $(py_install_dir)/include; cp -lf ncursesw/*.h .

$(py_install_dir)/lib/libreadline.a: $(BUILD_DIR)/$(READLINE).tar.gz
	rm -rf $(BUILD_DIR)/$(READLINE)-$(BUILD_TYPE)
	tar xzf $< -C $(BUILD_DIR)
	mv $(BUILD_DIR)/$(READLINE) $(BUILD_DIR)/$(READLINE)-$(BUILD_TYPE)
	cd $(BUILD_DIR)/$(READLINE)-$(BUILD_TYPE); \
	    autoreconf -i; \
	    bash_cv_wcwidth_broken=no \
	    CFLAGS="$(CFLAGS) -DANDROID" \
	    ./configure --host=$(ANDROID_HOST) --build=$(ANDROID_BUILD) \
	    --prefix=$(py_install_dir) \
	    --disable-shared --with-curses=$(BUILD_DIR)/$(NCURSES)-$(BUILD_TYPE)
	$(MAKE) -C $(BUILD_DIR)/$(READLINE)-$(BUILD_TYPE) all install

$(py_install_dir)/lib/libsqlite3.a: $(BUILD_DIR)/$(SQLITE_FNAME).tar.gz
	rm -rf $(BUILD_DIR)/$(SQLITE)-$(BUILD_TYPE)
	tar xzf $< -C $(BUILD_DIR)
	mv $(BUILD_DIR)/$(SQLITE_FNAME) $(BUILD_DIR)/$(SQLITE)-$(BUILD_TYPE)
	cd $(BUILD_DIR)/$(SQLITE)-$(BUILD_TYPE); \
	    ./configure --host=$(ANDROID_HOST) --build=$(ANDROID_BUILD) \
	        --prefix=$(py_install_dir) \
	    --disable-shared
	$(MAKE) -C $(BUILD_DIR)/$(SQLITE)-$(BUILD_TYPE) all install


$(py_install_dir)/lib/libffi.a: $(BUILD_DIR)/$(LIBFFI).tar.gz
	rm -rf $(BUILD_DIR)/$(LIBFFI)-$(BUILD_TYPE)
	tar xzf $< -C $(BUILD_DIR)
	cd $(BUILD_DIR)/$(LIBFFI); patch -p1 < $(CURDIR)/patches/libffi-mmap-exec.patch
ifeq ($(ANDROID_ARCH), armv7)
	cd $(BUILD_DIR)/$(LIBFFI); patch -p1 < $(CURDIR)/patches/libffi-stmeqia.patch
endif
	mv $(BUILD_DIR)/$(LIBFFI) $(BUILD_DIR)/$(LIBFFI)-$(BUILD_TYPE)
	cd $(BUILD_DIR)/$(LIBFFI)-$(BUILD_TYPE); \
	    ./configure --host=$(ANDROID_HOST) --build=$(ANDROID_BUILD) \
	    --prefix=$(py_install_dir) \
	    --disable-shared
	-$(MAKE) -C $(BUILD_DIR)/$(LIBFFI)-$(BUILD_TYPE) all install install-pkgconfigDATA

.PHONY: all libffi_header ncurses_header sqlite_header readline_header
