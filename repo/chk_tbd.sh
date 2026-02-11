#!/bin/bash
scripts/mk_clean.sh \
    && rm -rf firestarr build \
    && git add -u \
    && ./repeat_not.sh grep -ri tbd src/cpp
