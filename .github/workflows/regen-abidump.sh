set -ex

export DEBIAN_FRONTEND=noninteractive
./.github/workflows/posix-deps-apt.sh
apt-get install -yq abigail-tools python3
export CFLAGS="-g3 -O0"
./configure --enable-shared && make
make regen-abidump
