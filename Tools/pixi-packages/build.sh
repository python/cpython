#!/bin/bash

if [[ "${PYTHON_ASAN}" == 1 ]]; then
    echo "BUILD TYPE: ASAN"
    BUILD_DIR="../build_asan"
    CONFIGURE_EXTRA="--with-address-sanitizer"
    export ASAN_OPTIONS="detect_leaks=0:symbolize=1:strict_init_order=true:allocator_may_return_null=1:use_sigaltstack=0"
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
ln -sf "${PREFIX}/bin/python${MINOR_VERSION}" "${PREFIX}/bin/python"

# https://github.com/prefix-dev/rattler-build/issues/2012
if [[ ${OSTYPE} == "darwin"* ]]; then
    cp "${BUILD_PREFIX}/lib/clang/21/lib/darwin/libclang_rt.asan_osx_dynamic.dylib" "${PREFIX}/lib/libclang_rt.asan_osx_dynamic.dylib"
fi
