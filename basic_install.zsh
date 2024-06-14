#!/bin/zsh

./configure --enable-optimizations
make CFLAGS="-O3 -march=native" CXXFLAGS="-O3 -march=native" -j$(sysctl -n hw.ncpu)
sudo make install

PYTHON_VERSION=$(python3 --version | awk '{print $2}')
echo "alias python=python${PYTHON_VERSION}" >> ~/.zshrc
echo "alias python3=python${PYTHON_VERSION}" >> ~/.zshrc

source ~/.zshrc

echo "Python ${PYTHON_VERSION} installed and set as the global version."