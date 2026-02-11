#!/bin/bash
# set -e
DIR_LOG="build/docker"
# # don't have base images for all the platforms so just use a list
# PLATFORMS=$(docker buildx inspect --bootstrap | grep Platforms: | sed "s/Platforms: *//;s/ //g;s/,/\n/g;s/linux\///g" | sort)
# # exclude some things
# # PLATFORMS=$(echo "$PLATFORMS" | sed "/^386$/d;/^loong64$/d;/^mips64$/d;/^mips64le$/d;/^ppc64$/d")
# PLATFORMS=$(echo "$PLATFORMS" | sed "/^386$/d" | sed)
# # unsure about why arm/v8 isn't in list but works
# # PLATFORMS="amd64 arm/v8 arm/v7 arm64/v8 ppc64le s390x arm/v5 riscv64"
# no point in more than base image supports
# ignore 32-bit - 386, arm/v5
PLATFORMS="amd64 arm/v7 arm64/v8 ppc64le riscv64 s390x"
# echo PLATFORMS = ${PLATFORMS}
mkdir -p ${DIR_LOG}
for p in ${PLATFORMS}
do
  platform="linux/${p}"
  LOG_BASE="${DIR_LOG}/${p//\//}"
  # HACK: don't run if running
  printf "%15s" "${platform}"
  ps aux | grep -v grep | grep "platform=${platform}" > /dev/null \
    && printf "%30s\n" "already building" \
    && continue
  for flag in arch cpu tune
  do
    (docker run --platform=${platform} -it --rm gcc:latest g++ -m${flag}=native -Q --help=target 2>&1 || echo "${flag} failed") > ${LOG_BASE}_m${flag}.txt
    # printf "%8s=%d" "${flag}" "$?"
  done
  # (docker run --platform=${platform} -it --rm debian:trixie-slim /bin/bash -c 'apt update && apt install -y g++ && g++ -mtune=native -Q --help=target 2>&1'  || echo "tune failed") > ${LOG_BASE}_mtune.txt
  # (docker run --platform=${platform} -it --rm debian:trixie-slim /bin/bash -c 'apt update && apt install -y g++ && g++ -mcpu=native -Q --help=target 2>&1'  || echo "cpu failed") > ${LOG_BASE}_mcpu.txt
  # (docker run --platform=${platform} -it --rm debian:trixie-slim /bin/bash -c 'apt update && apt install -y g++ && g++ -march=native -Q --help=target 2>&1'  || echo "arch failed") > ${LOG_BASE}_march.txt
  target=firestarr
  log=${LOG_BASE}_${target}.log
  docker build --progress=plain --platform=${platform} --target ${target} -t ${target} -f .docker/Dockerfile . &> ${log}
  RET=$?
  # echo "RET=$RET"
  if [ ${RET} -ne 0 ]; then
    RESULT="failed"
    (grep "no match for platform in manifest" "${log}" > /dev/null) && RESULT="no base image"
    printf "%30s\n" "${RESULT}"
    continue
  fi
  # target=firestarr-minimal
  # log=${LOG_BASE}_${target}.log
  # docker build --progress=plain --platform=${platform} --target ${target} -t ${target} -f .docker/Dockerfile . 2&>1 ${log}
  # RET=$?
  # if [ ${RET} -ne 0 ]; then
  #   RESULT="failed"
  #   (grep "no match for platform in manifest" "${log}" > /dev/null) && RESULT="no base image"
  #   printf "%30s\n" "${RESULT}"
  #   continue
  # fi
  printf "%30s\n" "succeeded"
done
