
XZ Utils on DOS
===============

DOS-specific filename handling

    xz detects at runtime if long filename (LFN) support is
    available and will use it by default. It can be disabled by
    setting an environment variable:

        set lfn=n

    When xz is in LFN mode, it behaves pretty much the same as it
    does on other operating systems. Examples:

        xz foo.tar          -> foo.tar.xz
        xz -d foo.tar.xz    -> foo.tar

        xz -F lzma foo.tar  -> foo.tar.lzma
        xz -d foo.tar.lzma  -> foo.tar

    When LFN support isn't available or it is disabled with LFN=n
    environment setting, xz works in short filename (SFN) mode. This
    affects filename suffix handling when compressing.

    When compressing to the .xz format in SFN mode:

      - Files without an extension get .xz just like on LFN systems.

      - *.tar files become *.txz (shorthand for *.tar.xz). *.txz
        is recognized by xz on all supported operating systems.
        (Try to avoid confusing this with gzipped .txt files.)

      - Files with 1-3 character extension have their extension modified
        so that the last character is a dash ("-"). If the extension
        is already three characters, the last character is lost. The
        resulting *.?- or *.??- filename is recognized in LFN mode, but
        it isn't recognized by xz on other operating systems.

    Examples:

        xz foo              -> foo.xz     |   xz -d foo.xz    -> foo
        xz foo.tar          -> foo.txz    |   xz -d foo.txz   -> foo.tar
        xz foo.c            -> foo.c-     |   xz -d foo.c-    -> foo.c
        xz read.me          -> read.me-   |   xz -d read.me-  -> read.me
        xz foo.txt          -> foo.tx-    |   xz -d foo.tx-   -> foo.tx   !

    Note that in the last example above, the third character of the
    filename extension is lost.

    When compressing to the legacy .lzma format in SFN mode:

      - *.tar files become *.tlz (shorthand for *.tar.lzma). *.tlz
        is recognized by xz on all supported operating systems.

      - Other files become *.lzm. The original filename extension
        is lost. *.lzm is recognized also in LFN mode, but it is not
        recognized by xz on other operating systems.

    Examples:

        xz -F lzma foo      -> foo.lzm    |   xz -d foo.lzm   -> foo
        xz -F lzma foo.tar  -> foo.tlz    |   xz -d foo.tlz   -> foo.tar
        xz -F lzma foo.c    -> foo.lzm    |   xz -d foo.lzm   -> foo      !
        xz -F lzma read.me  -> read.lzm   |   xz -d read.lzm  -> read     !
        xz -F lzma foo.txt  -> foo.lzm    |   xz -d foo.lzm   -> foo      !

    When compressing with a custom suffix (-S .SUF, --suffix=.SUF) to
    any file format:

      - If the suffix begins with a dot, the filename extension is
        replaced with the new suffix. The original extension is lost.

      - If the suffix doesn't begin with a dot and the filename has no
        extension and the filename given on the command line doesn't
        have a dot at the end, the custom suffix is appended just like
        on LFN systems.

      - If the suffix doesn't begin with a dot and the filename has
        an extension (or an extension-less filename is given with a dot
        at the end), the last 1-3 characters of the filename extension
        may get overwritten to fit the given custom suffix.

    Examples:

        xz -S x foo        -> foox      |  xz -dS x foox      -> foo
        xz -S x foo.       -> foo.x     |  xz -dS x foo.x     -> foo
        xz -S .x foo       -> foo.x     |  xz -dS .x foo.x    -> foo
        xz -S .x foo.      -> foo.x     |  xz -dS .x foo.x    -> foo
        xz -S x.y foo      -> foox.y    |  xz -dS x.y foox.y  -> foo
        xz -S .a foo.c     -> foo.a     |  xz -dS .a foo.a    -> foo      !
        xz -S a  foo.c     -> foo.ca    |  xz -dS a foo.ca    -> foo.c
        xz -S ab foo.c     -> foo.cab   |  xz -dS ab foo.cab  -> foo.c
        xz -S ab read.me   -> read.mab  |  xz -dS ab read.mab -> read.m   !
        xz -S ab foo.txt   -> foo.tab   |  xz -dS ab foo.tab  -> foo.t    !
        xz -S abc foo.txt  -> foo.abc   |  xz -dS abc foo.abc -> foo      !

    When decompressing, the suffix handling in SFN mode is the same as
    in LFN mode. The DOS-specific filenames *.lzm, *.?-, and *.??- are
    recognized also in LFN mode.

    xz handles certain uncommon situations safely:

      - If the generated output filename refers to the same file as
        the input file, xz detects this and refuses to compress or
        decompress the input file even if --force is used. This can
        happen when giving an overlong filename in SFN mode. E.g.
        "xz -S x foo.texinfo" would try to write to foo.tex which on
        SFN system is the same file as foo.texinfo.

      - If the generated output filename is a special file like "con"
        or "prn", xz detects this and refuses to compress or decompress
        the input file even if --force is used.


Bugs

    xz doesn't necessarily work in Dosbox. It should work in DOSEMU.

    Pressing Ctrl-c or Ctrl-Break won't remove the incomplete target file
    when running under Windows XP Command Prompt (something goes wrong
    with SIGINT handling). It works correctly under Windows 95/98/98SE/ME.

