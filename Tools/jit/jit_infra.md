# JIT Build Infrastructure

This document includes details about the intricacies of the JIT build infrastructure.

## Updating LLVM

When we update LLVM, we need to also update the LLVM release artifact for Windows builds. This is because Windows builds automatically pull prebuilt LLVM binaries in our pipelines (e.g. notice that `.github/workflows/jit.yml` does not explicitly download LLVM or build it from source).

To update the LLVM release artifact for Windows builds, follow these steps:
1. Go to the [LLVM releases page](https://github.com/llvm/llvm-project/releases).
1. Download Windows artifacts for the desired LLVM version (e.g. `clang+llvm-21.1.4-x86_64-pc-windows-msvc.tar.xz` and `clang+llvm-21.1.4-aarch64-pc-windows-msvc.tar.xz`).
1. Extract and repackage each tarball with the correct directory structure. For example:
   ```bash
   # For x86_64 (AMD64)
   tar -xf clang+llvm-21.1.4-x86_64-pc-windows-msvc.tar.xz
   mv clang+llvm-21.1.4-x86_64-pc-windows-msvc llvm-21.1.4.0
   tar -cf - llvm-21.1.4.0 | pv | xz > llvm-21.1.4.0-x64.tar.xz
   rm -rf llvm-21.1.4.0

   # For ARM64
   tar -xf clang+llvm-21.1.4-aarch64-pc-windows-msvc.tar.xz
   mv clang+llvm-21.1.4-aarch64-pc-windows-msvc llvm-21.1.4.0
   tar -cf - llvm-21.1.4.0 | pv | xz > llvm-21.1.4.0-ARM64.tar.xz
   ```
   Each tarball must contain a top-level directory named `llvm-{version}.0/`.
1. Go to [cpython-bin-deps](https://github.com/python/cpython-bin-deps).
1. Create a new release with the LLVM artifacts.
    - Create a new tag to match the LLVM version (e.g. `llvm-21.1.4.0`).
    - Specify the release title (e.g. `LLVM 21.1.4`).
    - Upload both platform-specific assets to the same release.

### Other notes
- You must make sure that the name of the artifact matches exactly what is expected in `Tools/jit/_llvm.py` and `PCbuild/get_externals.py`.
- The artifact filename must include the architecture suffix (e.g. `llvm-21.1.4.0-x64.tar.xz`, `llvm-21.1.4.0-ARM64.tar.xz`).
- You must have permissions to create releases in the `cpython-bin-deps` repository. If you don't have permissions, you should contact one of the organization admins.