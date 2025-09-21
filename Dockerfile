# Use AlmaLinux 8, a 1:1 RHEL 8 compatible OS with standard repositories
FROM almalinux:8

# Set an environment variable to run dnf without prompts
ENV DNF_ASSUMEYES=1

# Install build dependencies for CPython
RUN dnf update -y && \
    # --- FIX STARTS HERE ---
    # Install the DNF plugins package which provides the 'config-manager' command
    dnf install -y dnf-plugins-core && \
    # --- FIX ENDS HERE ---
    # Enable the 'powertools' repository, which is the AlmaLinux equivalent of CRB
    dnf config-manager --set-enabled powertools && \
    # Now, install all the packages
    dnf install -y \
    gcc \
    make \
    git \
    findutils \
    zlib-devel \
    bzip2-devel \
    libffi-devel \
    openssl-devel \
    readline-devel \
    sqlite-devel \
    xz-devel \
    tk-devel && \
    # Clean up dnf cache to reduce image size
    dnf clean all

# Create a non-root user to run the build
RUN useradd --create-home buildbot
USER buildbot
WORKDIR /home/buildbot

# Clone the CPython repository from GitHub
# We use the 'main' branch, which is what the '3.x' buildbots track.
# --depth 1 creates a shallow clone to save time and space.
RUN git clone --branch main --depth 1 https://github.com/python/cpython.git

# Set the working directory for the build
WORKDIR /home/buildbot/cpython

# Configure and build in a single layer
# - Uses the exact configure flags from the buildbot: --with-pydebug and --with-lto
# - make -j$(nproc) builds in parallel using all available CPU cores
RUN ./configure --with-pydebug --with-lto && \
    make -j$(nproc)

# Set the environment variable to unlock the failure mode.
ENV GITHUB_ACTION=fake

CMD ["make", "test"]
