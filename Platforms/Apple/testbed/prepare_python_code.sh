#!/bin/sh
set -e
source "$PROJECT_DIR/Python{{abiflags}}.xcframework/build/utils.sh"
install_python "Python{{abiflags}}.xcframework" app
install_site_packages "Python{{abiflags}}.xcframework" app_packages
