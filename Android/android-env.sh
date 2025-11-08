# This script must be sourced with the following variables already set:
: "${ANDROID_HOME:?}"  # Path to Android SDK
: "${HOST:?}"  # GNU target triplet

# You may also override the following:
: "${ANDROID_API_LEVEL:=24}"  # Minimum Android API level the build will run on
: "${PREFIX:-}"  # Path in which to find required libraries


# Print all messages on stderr so they're visible when running within build-wheel.
log() {
    echo "$1" >&2
}

fail() {
    log "$1"
    exit 1
}

# When moving to a new version of the NDK, carefully review the following:
#
# * https://developer.android.com/ndk/downloads/revision_history
#
# * https://android.googlesource.com/platform/ndk/+/ndk-rXX-release/docs/BuildSystemMaintainers.md
#   where XX is the NDK version. Do a diff against the version you're upgrading from, e.g.:
#   https://android.googlesource.com/platform/ndk/+/ndk-r25-release..ndk-r26-release/docs/BuildSystemMaintainers.md
ndk_version=27.3.13750724

ndk=$ANDROID_HOME/ndk/$ndk_version
if ! [ -e "$ndk" ]; then
    log "Installing NDK - this may take several minutes"
    yes | "$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager" "ndk;$ndk_version"
fi

if [ "$HOST" = "arm-linux-androideabi" ]; then
    clang_triplet=armv7a-linux-androideabi
else
    clang_triplet="$HOST"
fi

# These variables are based on BuildSystemMaintainers.md above, and
# $ndk/build/cmake/android.toolchain.cmake.
toolchain=$(echo "$ndk"/toolchains/llvm/prebuilt/*)
export AR="$toolchain/bin/llvm-ar"
export AS="$toolchain/bin/llvm-as"
export CC="$toolchain/bin/${clang_triplet}${ANDROID_API_LEVEL}-clang"
export CXX="${CC}++"
export LD="$toolchain/bin/ld"
export NM="$toolchain/bin/llvm-nm"
export RANLIB="$toolchain/bin/llvm-ranlib"
export READELF="$toolchain/bin/llvm-readelf"
export STRIP="$toolchain/bin/llvm-strip"

# The quotes make sure the wildcard in the `toolchain` assignment has been expanded.
for path in "$AR" "$AS" "$CC" "$CXX" "$LD" "$NM" "$RANLIB" "$READELF" "$STRIP"; do
    if ! [ -e "$path" ]; then
        fail "$path does not exist"
    fi
done

export CFLAGS="-D__BIONIC_NO_PAGE_SIZE_MACRO"
export LDFLAGS="-Wl,--build-id=sha1 -Wl,--no-rosegment -Wl,-z,max-page-size=16384"

# Unlike Linux, Android does not implicitly use a dlopened library to resolve
# relocations in subsequently-loaded libraries, even if RTLD_GLOBAL is used
# (https://github.com/android/ndk/issues/1244). So any library that fails to
# build with this flag, would also fail to load at runtime.
LDFLAGS="$LDFLAGS -Wl,--no-undefined"

# Many packages get away with omitting -lm on Linux, but Android is stricter.
LDFLAGS="$LDFLAGS -lm"

# -mstackrealign is included where necessary in the clang launcher scripts which are
# pointed to by $CC, so we don't need to include it here.
if [ "$HOST" = "arm-linux-androideabi" ]; then
    CFLAGS="$CFLAGS -march=armv7-a -mthumb"
fi

if [ -n "${PREFIX:-}" ]; then
    abs_prefix="$(realpath "$PREFIX")"
    CFLAGS="$CFLAGS -I$abs_prefix/include"
    LDFLAGS="$LDFLAGS -L$abs_prefix/lib"

    export PKG_CONFIG="pkg-config --define-prefix"
    export PKG_CONFIG_LIBDIR="$abs_prefix/lib/pkgconfig"
fi

# When compiling C++, some build systems will combine CFLAGS and CXXFLAGS, and some will
# use CXXFLAGS alone.
export CXXFLAGS="$CFLAGS"

# Use the same variable name as conda-build
if [ "$(uname)" = "Darwin" ]; then
    CPU_COUNT="$(sysctl -n hw.ncpu)"
    export CPU_COUNT
else
    CPU_COUNT="$(nproc)"
    export CPU_COUNT
fi
