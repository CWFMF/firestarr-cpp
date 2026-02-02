ARG USERNAME=user
ARG USER_ID=1000

FROM debian:trixie-slim AS minimal-with-user
ARG USERNAME
ARG USER_ID
ENV TMPDIR=/tmp
RUN apt-get update --fix-missing \
  && apt-get install -y locales \
  && sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen \
  && locale-gen \
  && apt-get install -y sudo \
  && groupadd --gid ${USER_ID} ${USERNAME} \
  && useradd --uid ${USER_ID} --gid ${USER_ID} -m ${USERNAME} \
  && chsh --shell /bin/bash ${USERNAME} \
  && echo ${USERNAME} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} \
  && chmod 0440 /etc/sudoers.d/${USERNAME} \
  && mkdir -p /appl/firestarr \
  && chown -R ${USERNAME}:${USERNAME} /appl


FROM minimal-with-user AS gcc-cmake-with-libs
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  cmake gcc g++ make ninja-build pkg-config \
  curl zip unzip tar ca-certificates git


FROM gcc-cmake-with-libs AS firestarr-dev
ARG USERNAME
ARG VERSION
ENV VERSION=${VERSION}
RUN apt-get update --fix-missing \
  && apt-get install -y --no-install-recommends \
  git time nano less psmisc man bc tmux \
  google-perftools libgoogle-perftools-dev golang-github-google-pprof-dev \
  ssh \
  cmake linux-libc-dev curl zip unzip tar ninja-build \
  gdb valgrind libdwarf-dev libelf-dev libdw-dev linux-perf \
  cmake gcc g++ make clang-tools clang-tidy clang-format
RUN git config --global core.editor "nano"
RUN git config --global core.autocrlf false
WORKDIR /appl/firestarr
USER ${USERNAME}


FROM gcc-cmake-with-libs AS firestarr-build
ARG USERNAME
ARG VERSION
ENV VERSION=${VERSION}
ENV VCPKG_DISABLE_METRICS=1
ENV VCPKG_ROOT=/appl/firestarr/vcpkg
WORKDIR /appl/firestarr
# need .git for CMakeLists.txt
USER ${USERNAME}
RUN git clone https://github.com/microsoft/vcpkg.git
COPY --chown=${USERNAME}:${USERNAME} ./vcpkg*.json .
RUN cd vcpkg \
  && ./bootstrap-vcpkg.sh -disableMetrics \
  && cd .. \
  && vcpkg/vcpkg install
COPY --chown=${USERNAME}:${USERNAME} ./.git .git/
COPY --chown=${USERNAME}:${USERNAME} ./src/cpp src/cpp/
COPY --chown=${USERNAME}:${USERNAME} ./cmake cmake/
COPY --chown=${USERNAME}:${USERNAME} ./.env ./CMake* ./.clang-* ./fuel.lut ./settings.ini ./vcpkg*.json .
RUN cmake --preset Release
RUN cmake --build --parallel --preset Release
COPY --chown=${USERNAME}:${USERNAME} ./test test/
RUN ctest --preset Release

FROM minimal-with-user AS firestarr-base-env
ARG USERNAME
ARG VERSION
ENV VERSION=${VERSION}
USER root
# get rid of anything that's not required for firestarr - breaks apt in container
RUN apt-get clean \
  && rm -rf \
    /tmp/* \
    /usr/share/doc/* \
    /var/cache/* \
    /var/lib/apt/lists/* \
    /var/lib/dpkg/* \
    /var/tmp/*

# make a layer that's the environment for firestarr with scratch to clear out old layers
FROM scratch AS firestarr-base
COPY --from=firestarr-base-env / /

# derive final image from base so updates to binary are small layers
FROM firestarr-base AS firestarr
ARG USERNAME
USER ${USERNAME}
WORKDIR /appl/firestarr
COPY --chown=${USERNAME}:${USERNAME} ./fuel.lut /appl/firestarr/
COPY --chown=${USERNAME}:${USERNAME} ./settings.ini /appl/firestarr/
COPY --chown=${USERNAME}:${USERNAME} --from=firestarr-build /appl/firestarr/firestarr /appl/firestarr/
COPY --chown=${USERNAME}:${USERNAME} --from=firestarr-build /appl/firestarr/proj.db /appl/firestarr/proj.db
USER ${USERNAME}
RUN echo cd /appl/firestarr >> /home/${USERNAME}/.bashrc
ENTRYPOINT [ "/appl/firestarr/firestarr" ]
