# JIT Build Infrastructure

This document includes details about the intricacies of the JIT build infrastructure.

## Updating LLVM

When we update LLVM, we need to also update the LLVM release artifact for Windows builds. This is because Windows builds automatically pull prebuilt LLVM binaries in our pipelines (e.g. notice that `.github/workflows/jit.yml` does not explicitly download LLVM or build it from source).

To update the LLVM release artifact for Windows builds, follow these steps:
1. Go to the [LLVM releases page](https://github.com/llvm/llvm-project/releases).
1. Download x86_64 Windows artifact for the desired LLVM version (e.g. `clang+llvm-21.1.4-x86_64-pc-windows-msvc.tar.xz`).
1. Extract and repackage the tarball with the correct directory structure. For example:
   ```bash
   tar -xf clang+llvm-21.1.4-x86_64-pc-windows-msvc.tar.xz
   mv clang+llvm-21.1.4-x86_64-pc-windows-msvc llvm-21.1.4.0
   tar -cf - llvm-21.1.4.0 | pv | xz > llvm-21.1.4.0.tar.xz  
   ```
    The tarball must contain a top-level directory named `llvm-{version}.0/`.
1. Go to [cpython-bin-deps](https://github.com/python/cpython-bin-deps).
1. Create a new release with the updated LLVM artifact.
    - Create a new tag to match the LLVM version (e.g. `llvm-21.1.4.0`).
    - Specify the release title (e.g. `LLVM 21.1.4 for x86_64 Windows`).
    - Upload the asset (you can leave all other fields the same).

### Other notes
- You must make sure that the name of the artifact matches exactly what is expected in `Tools/jit/_llvm.py` and `PCbuild/get_externals.py`.
- We don't need multiple release artifacts for each architecture because LLVM can cross-compile for different architectures on Windows; x86_64 is sufficient.
- You must have permissions to create releases in the `cpython-bin-deps` repository. If you don't have permissions, you should contact one of the organization admins.