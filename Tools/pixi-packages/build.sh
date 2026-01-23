#!/bin/bash

echo "PYTHON_VARIANT: ${PYTHON_VARIANT}"

if [[ "${PYTHON_VARIANT}" == "freethreading" ]]; then
    CONFIGURE_EXTRA="--disable-gil"
elif [[ "${PYTHON_VARIANT}" == "asan" ]]; then
    CONFIGURE_EXTRA="--with-address-sanitizer"
    export ASAN_OPTIONS="strict_init_order=true"
elif [[ "${PYTHON_VARIANT}" == "tsan-freethreading" ]]; then
    CONFIGURE_EXTRA="--disable-gil --with-thread-sanitizer"
    export TSAN_OPTIONS="suppressions=${SRC_DIR}/Tools/tsan/suppressions_free_threading.txt"
elif [[ "${PYTHON_VARIANT}" == "default" ]]; then
    CONFIGURE_EXTRA=""
else
    echo "Unknown PYTHON_VARIANT: ${PYTHON_VARIANT}"
    exit 1
fi

# rattler-build by default set a target of 10.9
# override it to at least 10.12
case ${MACOSX_DEPLOYMENT_TARGET:-10.12} in
    10.12|10.13|10.14|10.15|10.16)
        ;;
    10.*)
        export CPPFLAGS=${CPPFLAGS/-mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}/-mmacosx-version-min=10.12}
        export MACOSX_DEPLOYMENT_TARGET=10.12
        ;;
    *)
        ;;
esac

BUILD_DIR="../build_${PYTHON_VARIANT}"
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
        --with-system-expat \
        ${CONFIGURE_EXTRA}
fi

touch configure-done

make -j"${CPU_COUNT}" install
ln -sf "${PREFIX}/bin/python3" "${PREFIX}/bin/python"
