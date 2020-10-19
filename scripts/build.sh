#!/bin/bash
# didn't figure out how to do this with cmake yet but this works for now
DIR_ROOT="$(realpath $(dirname $(realpath "$0"))/..)"
DIR_BUILD="${DIR_ROOT}/build"
VAR_RELEASE="Release"
VAR_DEBUG="Debug"
VAR_TEST="Test"
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
    || [ "${VAR_TEST,,}" == "${VARIANT,,}" ] ); then
  ASAN_ARGS=-DUSE_ASAN:BOOL=1
fi
if [ "${VAR_TEST,,}" == "${VARIANT,,}" ]; then
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
VARIANT=${VARIANT^^}
echo Set VARIANT=${VARIANT}
echo Set ASAN_ARGS=${ASAN_ARGS}
echo "${@}"
/usr/bin/cmake --no-warn-unused-cli ${ASAN_ARGS} ${@} -DCMAKE_BUILD_TYPE:STRING=${VARIANT} -S${DIR_ROOT} -B${DIR_BUILD} -G "Unix Makefiles" \
  && /usr/bin/cmake --build ${DIR_BUILD} --config ${VARIANT} --target all -j 50 ${CLEAN_FIRST} --
