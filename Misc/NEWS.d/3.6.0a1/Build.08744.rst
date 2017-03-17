Issue #25702: A --with-lto configure option has been added that will
enable link time optimizations at build time during a make profile-opt.
Some compilers and toolchains are known to not produce stable code when
using LTO, be sure to test things thoroughly before relying on it.
It can provide a few % speed up over profile-opt alone.