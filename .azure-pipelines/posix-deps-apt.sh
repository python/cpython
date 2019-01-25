apt-get update

apt-get -yq install \
    build-essential \
    zlib1g-dev \
    libbz2-dev \
    liblzma-dev \
    libncurses5-dev \
    libreadline6-dev \
    libsqlite3-dev \
    libssl-dev \
    libgdbm-dev \
    tk-dev \
    lzma \
    lzma-dev \
    liblzma-dev \
    libffi-dev \
    uuid-dev \
    xvfb

if [ ! -z "$1" ]
then
  echo ##vso[task.prependpath]$PWD/multissl/openssl/$1
  echo ##vso[task.setvariable variable=OPENSSL_DIR]$PWD/multissl/openssl/$1
  python3 Tools/ssl/multissltests.py --steps=library --base-directory $PWD/multissl --openssl $1 --system Linux
fi
