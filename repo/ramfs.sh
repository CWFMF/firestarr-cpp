#!/bin/bash
set -x
for d in build test/output vcpkg
do
  dir="/appl/firestarr/$d"
  sudo umount "$dir"
  rm -rf "$dir"
  mkdir -p "$dir"
  sudo chown -R user:user "$dir"
  sudo mount -o size=8G -t tmpfs none "$dir"
done
export GIT_DISCOVERY_ACROSS_FILESYSTEM=1
vmtouch -t test/input
