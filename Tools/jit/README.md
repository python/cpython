The JIT Compiler
================

This version of CPython can be built with an experimental just-in-time compiler[^pep-744]. While most everything you already know about building and using CPython is unchanged, you will probably need to install a compatible version of LLVM first.

Python 3.11 or newer is required to build the JIT.

## Installing LLVM

The JIT compiler does not require end users to install any third-party dependencies, but part of it must be *built* using LLVM[^why-llvm]. You are *not* required to build the rest of CPython using LLVM, or even the same version of LLVM (in fact, this is uncommon).

LLVM version 19 is required. Both `clang` and `llvm-readobj` need to be installed and discoverable (version suffixes, like `clang-19`, are okay). It's highly recommended that you also have `llvm-objdump` available, since this allows the build script to dump human-readable assembly for the generated code.

It's easy to install all of the required tools:

### Linux

Install LLVM 19 on Ubuntu/Debian:

```sh
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 19
```

Install LLVM 19 on Fedora Linux 40 or newer:

```sh
sudo dnf install 'clang(major) = 19' 'llvm(major) = 19'
```

### macOS

Install LLVM 19 with [Homebrew](https://brew.sh):

```sh
brew install llvm@19
```

Homebrew won't add any of the tools to your `$PATH`. That's okay; the build script knows how to find them.

### Windows

LLVM is downloaded automatically (along with other external binary dependencies) by `PCbuild\build.bat`.

Otherwise, you can install LLVM 19 [by searching for it on LLVM's GitHub releases page](https://github.com/llvm/llvm-project/releases?q=19), clicking on "Assets", downloading the appropriate Windows installer for your platform (likely the file ending with `-win64.exe`), and running it. **When installing, be sure to select the option labeled "Add LLVM to the system PATH".**

Alternatively, you can use [chocolatey](https://chocolatey.org):

```sh
choco install llvm --version=19.1.0
```


## Building

For `PCbuild`-based builds, pass the `--experimental-jit` option to `build.bat`.

For all other builds, pass the `--enable-experimental-jit` option to `configure`.

Otherwise, just configure and build as you normally would. Cross-compiling "just works", since the JIT is built for the host platform.

The JIT can also be enabled or disabled using the `PYTHON_JIT` environment variable, even on builds where it is enabled or disabled by default. More details about configuring CPython with the JIT and optional values for `--enable-experimental-jit` can be found [here](https://docs.python.org/dev/using/configure.html#cmdoption-enable-experimental-jit).

[^pep-744]: [PEP 744](https://peps.python.org/pep-0744/)

[^why-llvm]: Clang is specifically needed because it's the only C compiler with support for guaranteed tail calls (`musttail`), which are required by CPython's continuation-passing-style approach to JIT compilation. Since LLVM also includes other functionalities we need (namely, object file parsing and disassembly), it's convenient to only support one toolchain at this time.
