# amd64, arm/v5, arm/v7, arm64/v8, ppc64le, s390x
ARG BASE_DISTROLESS=gcr.io/distroless/static-debian13
# armv/v5, 386, riscv64, i686
ARG BASE_SLIM=debian:trixie-slim


FROM debian:trixie-slim AS gcc-cmake
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  cmake gcc g++ make ninja-build pkg-config \
  curl zip unzip tar ca-certificates git


# run bootstrap so clone is separate
FROM gcc-cmake AS vcpkg-bootstrap
ENV VCPKG_DISABLE_METRICS=1
# vcpkg says: Environment variable VCPKG_FORCE_SYSTEM_BINARIES must be set on arm, s390x, ppc64le and riscv platforms.
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_ROOT=/appl/firestarr/vcpkg
# need .git for CMakeLists.txt
WORKDIR /appl/firestarr
# compile/setup for vcpkg doesn't depend on what we're going to install
RUN git clone -v https://github.com/microsoft/vcpkg.git
RUN cd vcpkg && ./bootstrap-vcpkg.sh -disableMetrics

# run bootstrap so clone is separate
FROM vcpkg-bootstrap AS vcpkg-installed
ENV VCPKG_DISABLE_METRICS=1
# vcpkg says: Environment variable VCPKG_FORCE_SYSTEM_BINARIES must be set on arm, s390x, ppc64le and riscv platforms.
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_ROOT=/appl/firestarr/vcpkg
# need .git for CMakeLists.txt
WORKDIR /appl/firestarr
# install packages so they stay if nothing changes with vcpkg*.json
COPY ./vcpkg*.json .
RUN vcpkg/vcpkg install --only-downloads
RUN vcpkg/vcpkg install

FROM vcpkg-installed AS firestarr-build
ENV VCPKG_DISABLE_METRICS=1
# vcpkg says: Environment variable VCPKG_FORCE_SYSTEM_BINARIES must be set on arm, s390x, ppc64le and riscv platforms.
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_ROOT=/appl/firestarr/vcpkg
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

# amd64, arm/v7, arm64/v8, ppc64le, s390x
FROM --platform=linux/amd64 ${BASE_DISTROLESS} AS firestarr-base-amd64
FROM --platform=linux/arm/v7 ${BASE_DISTROLESS} AS firestarr-base-armv7
FROM --platform=linux/arm64/v8 ${BASE_DISTROLESS} AS firestarr-base-arm64v8
FROM firestarr-base-arm64v8 AS firestarr-base-arm64
FROM --platform=linux/ppc64le ${BASE_DISTROLESS} AS firestarr-base-ppc64le
FROM --platform=linux/s390x ${BASE_DISTROLESS} AS firestarr-base-s390x

# no distroless version of these, so use slim
# armv/v5, 386, riscv64, i686
FROM --platform=linux/arm/v5 firestarr-base-slim AS firestarr-base-armv5
# this seems like a waste of time - not sure even if vcpkg supports
# FROM --platform=linux/386 firestarr-base-slim AS firestarr-base-386
FROM --platform=linux/riscv64 firestarr-base-slim AS firestarr-base-riscv64
# no trixie-slim
# FROM --platform=linux/i686 firestarr-base-slim AS firestarr-base-i686

# derive final image from base so updates to binary are small layers
FROM firestarr-base-${TARGETARCH}${TARGETVARIANT} AS firestarr
WORKDIR /appl/firestarr
COPY ./fuel.lut /appl/firestarr/
COPY ./settings.ini /appl/firestarr/
COPY --from=firestarr-build /appl/firestarr/firestarr /appl/firestarr/
COPY --from=firestarr-build /appl/firestarr/proj.db /appl/firestarr/proj.db
ENTRYPOINT [ "/appl/firestarr/firestarr" ]
