# amd64, arm/v5, arm/v7, arm64/v8, ppc64le, s390x
ARG BASE_DISTROLESS=gcr.io/distroless/static-debian13
# armv/v5, 386, riscv64, i686
ARG BASE_SLIM=debian:trixie-slim

# basic image to start from
FROM debian:trixie-slim AS gcc-cmake
ARG TARGETARCH
ARG TARGETVARIANT
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  cmake gcc g++ make ninja-build pkg-config \
  curl zip unzip tar ca-certificates git
WORKDIR /appl/firestarr
# HACK: show what gcc wants to use for debugging purposes
RUN (export \
  && ( \
      g++ -march=native -Q --help=target \
      || g++ -mcpu=native -Q --help=target \
      || echo "Can't use arch or cpu" \
    ) \
  ) > /tmp/march.log
RUN (export && g++ -mtune=native -Q --help=target) > /tmp/mtune.log

# clone/bootstrap for vcpkg doesn't depend on what we're going to install
FROM gcc-cmake AS vcpkg-bootstrap
# vcpkg says: Environment variable VCPKG_FORCE_SYSTEM_BINARIES must be set on arm, s390x, ppc64le and riscv platforms.
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_ROOT=/appl/firestarr/vcpkg
# need .git for CMakeLists.txt
WORKDIR /appl/firestarr
RUN git clone -v https://github.com/microsoft/vcpkg.git \
  && vcpkg/bootstrap-vcpkg.sh -disableMetrics

# install packages so they stay if nothing changes with vcpkg*.json
FROM vcpkg-bootstrap AS vcpkg-installed
# need .git for CMakeLists.txt
WORKDIR /appl/firestarr
COPY ./vcpkg*.json .
RUN vcpkg/vcpkg install

# build and test
FROM vcpkg-installed AS firestarr-build
WORKDIR /appl/firestarr
# need .git for CMakeLists.txt
COPY ./.git .git/
COPY ./src/cpp src/cpp/
COPY ./cmake cmake/
COPY ./.env ./CMake* ./.clang-* ./fuel.lut ./settings.ini ./vcpkg*.json .
RUN cmake --preset Release
RUN cmake --build --parallel --preset Release
COPY ./test test/
RUN ctest --preset Release

# make a layer with scratch to clear out old layers
FROM ${BASE_SLIM} AS firestarr-base-cleaned
RUN apt-get clean \
  && rm -rf \
    /tmp/* \
    /usr/share/doc/* \
    /var/cache/* \
    /var/lib/apt/lists/* \
    /var/lib/dpkg/* \
    /var/tmp/*
COPY ./fuel.lut /appl/firestarr/
COPY ./settings.ini /appl/firestarr/
COPY --from=firestarr-build /appl/firestarr/firestarr /appl/firestarr/
COPY --from=firestarr-build /appl/firestarr/proj.db /appl/firestarr/proj.db

# derive final image from base so updates to binary are small layers
FROM scratch AS firestarr
COPY --from=firestarr-base-cleaned / /
WORKDIR /appl/firestarr
ENTRYPOINT [ "/appl/firestarr/firestarr" ]

FROM --platform=linux/amd64 ${BASE_DISTROLESS} AS distroless-amd64

# seems like we can just delete everything and then copy distroless into any platform?
FROM ${BASE_SLIM} AS firestarr-base-minimal
WORKDIR /
RUN rm -rf $(ls -1 / | grep -vE "appl|dev|etc|proc|sys")
COPY --from=distroless-amd64 / /

# make a layer with scratch to clear out old layers
FROM scratch AS firestarr-minimal
COPY --from=firestarr-base-minimal / /
WORKDIR /appl/firestarr
ENTRYPOINT [ "/appl/firestarr/firestarr" ]
