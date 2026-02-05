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
RUN (export && g++ -march=native -Q --help=target) > /tmp/march.log
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

# get rid of anything that's not required for firestarr - breaks apt in container
FROM ${BASE_SLIM} AS firestarr-base-cleaned
RUN apt-get clean \
  && rm -rf \
    /tmp/* \
    /usr/share/doc/* \
    /var/cache/* \
    /var/lib/apt/lists/* \
    /var/lib/dpkg/* \
    /var/tmp/*


# make a layer with scratch to clear out old layers
FROM scratch AS firestarr-base-slim
COPY --from=firestarr-base-cleaned / /

# derive final image from base so updates to binary are small layers
FROM firestarr-base-slim AS firestarr
WORKDIR /appl/firestarr
COPY ./fuel.lut /appl/firestarr/
COPY ./settings.ini /appl/firestarr/
COPY --from=firestarr-build /appl/firestarr/firestarr /appl/firestarr/
COPY --from=firestarr-build /appl/firestarr/proj.db /appl/firestarr/proj.db
ENTRYPOINT [ "/appl/firestarr/firestarr" ]


# break system functionality to make image smaller
# since binary is statically linked we don't need anything else
FROM firestarr-base-slim AS firestarr-base-distroless
RUN rm -rf $(ls -1 / | grep -vE "appl|dev|etc|proc|sys")


# make a layer with scratch to clear out old layers
FROM scratch AS firestarr-base-minimal
COPY --from=firestarr-base-distroless / /


# amd64, arm/v7, arm64/v8, ppc64le, s390x
FROM --platform=linux/amd64 ${BASE_DISTROLESS} AS firestarr-minimal-amd64
FROM --platform=linux/arm/v7 ${BASE_DISTROLESS} AS firestarr-minimal-armv7
FROM --platform=linux/arm64/v8 ${BASE_DISTROLESS} AS firestarr-minimal-arm64v8
FROM firestarr-minimal-arm64v8 AS firestarr-minimal-arm64
FROM --platform=linux/ppc64le ${BASE_DISTROLESS} AS firestarr-minimal-ppc64le
FROM --platform=linux/s390x ${BASE_DISTROLESS} AS firestarr-minimal-s390x

# no distroless version of these, so use minimal
# armv/v5, 386, riscv64, i686
FROM --platform=linux/arm/v5 firestarr-base-minimal AS firestarr-minimal-armv5
# this seems like a waste of time - not sure even if vcpkg supports
# FROM --platform=linux/386 firestarr-base-minimal AS firestarr-minimal-386
FROM --platform=linux/riscv64 firestarr-base-minimal AS firestarr-minimal-riscv64
# FROM --platform=linux/i686 firestarr-base-minimal AS firestarr-minimal-i686

# minimal is either distroless or slim with apt broken
FROM firestarr-minimal-${TARGETARCH}${TARGETVARIANT} AS firestarr-minimal
# FROM firestarr-base-minimal AS firestarr-minimal
WORKDIR /appl/firestarr
COPY ./fuel.lut /appl/firestarr/
COPY ./settings.ini /appl/firestarr/
COPY --from=firestarr-build /appl/firestarr/firestarr /appl/firestarr/
COPY --from=firestarr-build /appl/firestarr/proj.db /appl/firestarr/proj.db
ENTRYPOINT [ "/appl/firestarr/firestarr" ]
