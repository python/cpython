<div align=center>

The JIT Compiler
================

</div>

### Installing LLVM

While the JIT compiler does not require end users to install any third-party dependencies, part of it must be *built* using LLVM. It is *not* required for you to build the rest of CPython using LLVM, or the even the same version of LLVM (in fact, this is uncommon).

LLVM versions 14, 15, and 16 all work. Both `clang` and `llvm-readobj` need to be installed and discoverable (version suffixes, like `clang-16`, are okay). It's highly recommended that you also have `llvm-objdump` available, since this allows the build script to dump human-readable assembly for the generated code.

It's easy to install all of the required tools:

#### Linux

Install LLVM 16 on Ubuntu/Debian:

```
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 16
```

#### macOS

Install LLVM 16 with Homebrew:

```sh
$ brew install llvm@16
```

Homebrew won't add any of the tools to your `$PATH`. That's okay; the build script knows how to find them.

#### Windows

LLVM 16 can be installed on Windows by using the installers published on [LLVM's GitHub releases page](https://github.com/llvm/llvm-project/releases/tag/llvmorg-16.0.6).

[Here's a recent one.](https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.4/LLVM-16.0.4-win64.exe)

### Building

Once LLVM is installed, just configure and build as you normally would (either with `PCbuild/build.bat` or `./configure` and `make`). Cross-compiling "just works", too, since the JIT is built for the host platform.
