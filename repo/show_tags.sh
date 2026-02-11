#!/bin/bash
curl -s https://github.com/CWFMF/firestarr-cpp/pkgs/container/firestarr-cpp%2Fcache/versions | grep 'tag=' | sed "s/.*tag=\([^\"]*\)\".*/cache:\1/g"
curl -s https://github.com/CWFMF/firestarr-cpp/pkgs/container/firestarr-cpp%2Fvcpkg-installed/versions | grep 'tag=' | sed "s/.*tag=\([^\"]*\)\".*/vcpkg-installed:\1/g"
curl -s https://github.com/CWFMF/firestarr-cpp/pkgs/container/firestarr-cpp%2Ffirestarr/versions | grep 'tag=' | sed "s/.*tag=\([^\"]*\)\".*/firestarr:\1/g"
curl -s https://github.com/CWFMF/firestarr-cpp/pkgs/container/firestarr-cpp%2Ffirestarr-minimal/versions | grep 'tag=' | sed "s/.*tag=\([^\"]*\)\".*/firestarr-minimal:\1/g"
