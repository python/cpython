#!/bin/bash

if [[ "${PYTHON_VARIANT}" == "free-threading" ]]; then
    echo "BUILD TYPE: FREE-THREADING"
    BUILD_DIR="../build_free_threading"
    CONFIGURE_EXTRA="--disable-gil"
elif [[ "${PYTHON_VARIANT}" == "asan" ]]; then
    echo "BUILD TYPE: ASAN"
    BUILD_DIR="../build_asan"
    CONFIGURE_EXTRA="--with-address-sanitizer"
    export ASAN_OPTIONS="strict_init_order=true"
elif [[ "${PYTHON_VARIANT}" == "tsan" ]]; then
    echo "BUILD TYPE: TSAN"
    BUILD_DIR="../build_tsan"
    CONFIGURE_EXTRA="--disable-gil --with-thread-sanitizer"
    export TSAN_OPTIONS="suppressions=${SRC_DIR}/Tools/tsan/suppressions_free_threading.txt"
else
    echo "BUILD TYPE: DEFAULT"
    BUILD_DIR="../build"
    CONFIGURE_EXTRA=""
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

if [[ -f configure-done ]]; then
    echo "Skipping configure step, already done."
else
    "${SRC_DIR}/configure" \
        --prefix="${PREFIX}" \
        --oldincludedir="${BUILD_PREFIX}/${HOST}/sysroot/usr/include" \
        --enable-shared \
        --srcdir="${SRC_DIR}" \
        ${CONFIGURE_EXTRA}
fi

touch configure-done

make -j"${CPU_COUNT}" install
ln -sf "${PREFIX}/bin/python3" "${PREFIX}/bin/python"

# https://github.com/prefix-dev/rattler-build/issues/2012
if [[ ${OSTYPE} == "darwin"* ]]; then
    cp "${BUILD_PREFIX}/lib/clang/21/lib/darwin/libclang_rt.asan_osx_dynamic.dylib" "${PREFIX}/lib/libclang_rt.asan_osx_dynamic.dylib"
fi
