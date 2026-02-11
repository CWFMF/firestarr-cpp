#!/bin/bash
set -e
export VCPKG_ROOT=$(realpath .)/vcpkg
export PATH=$VCPKG_ROOT:$PATH
export VCPKG_DISABLE_METRICS=1
export VCPKG_INSTALLED_DIR=$VCPKG_ROOT/installed
if [ ! -d "${VCPKG_ROOT}" ]; then
  git clone https://github.com/microsoft/vcpkg.git
  cd vcpkg && ./bootstrap-vcpkg.sh -disableMetrics && cd ..
fi
# sudo apt-get install pkg-config
cmake --preset=default
cmake --build build
