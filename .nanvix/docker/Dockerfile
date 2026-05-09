# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

# toolchain-python — Nanvix GCC toolchain + host Python 3 for CPython
# cross-compilation.  Published as ghcr.io/nanvix/toolchain-python.

FROM ghcr.io/nanvix/toolchain-gcc:sha-34a3641

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        python3 \
        python3-dev \
    && rm -rf /var/lib/apt/lists/* \
    && ln -sf /usr/bin/python3 /opt/nanvix/bin/python3
