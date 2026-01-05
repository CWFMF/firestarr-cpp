#!/bin/bash
# didn't figure out how to do this with cmake yet but this works for now
DIR_ROOT="$(realpath $(dirname $(realpath "$0"))/..)"
DIR_BUILD="${DIR_ROOT}/build"
VAR_RELEASE="Release"
VAR_DEBUG="Debug"
VAR_ASAN="ASAN"
VAR_RELWITHDEBINFO="RelWithDebInfo"
CLEAN_FIRST=
if [ "--clean-first" == "$1" ]; then
  CLEAN_FIRST="--clean-first"
  # HACK: if used different args for config then just --clean-first doesn't change that
  rm -rf "${DIR_BUILD}"
  shift;
fi
VARIANT="$1"
ASAN_ARGS=
if ( [ "${VAR_DEBUG,,}" == "${VARIANT,,}" ] \
    || [ "${VAR_ASAN,,}" == "${VARIANT,,}" ] ); then
  ASAN_ARGS=-DUSE_ASAN:BOOL=1
fi
if [ "${VAR_ASAN,,}" == "${VARIANT,,}" ]; then
  VARIANT=${VAR_RELWITHDEBINFO}
fi
if ( [ -z "${VARIANT}" ] || \
    ( [ "${VAR_RELEASE,,}" != "${VARIANT,,}" ] \
      && [ "${VAR_DEBUG,,}" != "${VARIANT,,}" ] \
      && [ "${VAR_RELWITHDEBINFO,,}" != "${VARIANT,,}" ] \
    ) ); then
  # assume that argument is an arg to pass to cmake
  VARIANT=${VAR_RELEASE}
  # don't shift because $1 wasn't the variant
else
  shift;
fi
TARGET="$1"
if ( [ -z "${TARGET}" ] || [[ "${TARGET}" =~ -D* ]] ); then
  TARGET="firestarr"
else
  shift;
fi
echo Set VARIANT=${VARIANT}
echo Set ASAN_ARGS=${ASAN_ARGS}
echo Set TARGET=${TARGET}
echo "${@}"

# setup vcpkg
set -e
pushd "${DIR_ROOT}"
export VCPKG_ROOT=$(realpath .)/vcpkg
export PATH=$VCPKG_ROOT:$PATH
export VCPKG_DISABLE_METRICS=1
export VCPKG_INSTALLED_DIR=$VCPKG_ROOT/installed
if [ ! -d "${VCPKG_ROOT}" ]; then
  git clone https://github.com/microsoft/vcpkg.git
  pushd vcpkg && ./bootstrap-vcpkg.sh -disableMetrics && popd
fi
# sudo apt-get install pkg-config
set -x
cmake --preset ${VARIANT} --no-warn-unused-cli ${ASAN_ARGS} ${@} \
  && cmake --build --parallel --preset ${VARIANT} ${CLEAN_FIRST} --target ${TARGET} --
popd
