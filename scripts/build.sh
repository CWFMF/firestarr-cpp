#!/bin/bash
# didn't figure out how to do this with cmake yet but this works for now
DIR_ROOT="$(realpath $(dirname $(realpath "$0"))/..)"
DIR_BUILD="${DIR_ROOT}/build"
CLEAN_FIRST=
if [ "--clean-first" == "$1" ]; then
  CLEAN_FIRST="--clean-first"
  # HACK: if used different args for config then just --clean-first doesn't change that
  rm -rf "${DIR_BUILD}"
  shift;
fi
VARIANT="$1"
ASAN_ARGS=
if ( [ "Debug" == "${VARIANT}" ] || [ "Test" == "${VARIANT}" ] ); then
  ASAN_ARGS=-DUSE_ASAN:BOOL=1
fi
if [ "Test" == "${VARIANT}" ]; then
  VARIANT=Release
fi
if ( [ -z "${VARIANT}" ] || ( [ "Release" != "${VARIANT}" ] && [ "Debug" != ${VARIANT} ] ) ); then
  # assume that argument is an arg to pass to cmake
  VARIANT=Release
  # don't shift because $1 wasn't the variant
else
  shift;
fi
echo Set VARIANT=${VARIANT}
echo Set ASAN_ARGS=${ASAN_ARGS}
echo "${@}"
/usr/bin/cmake --no-warn-unused-cli ${ASAN_ARGS} ${@} -DCMAKE_BUILD_TYPE:STRING=${VARIANT} -S${DIR_ROOT} -B${DIR_BUILD} -G "Unix Makefiles" \
  && /usr/bin/cmake --build ${DIR_BUILD} --config ${VARIANT} --target all -j 50 ${CLEAN_FIRST} --
