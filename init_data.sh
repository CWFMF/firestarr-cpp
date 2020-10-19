#!/bin/bash

test/both_all.sh \
    && diff -rq -x firestarr.log ../data/initial/20251022 test/output/constant/ \
    && git add test/output \
    && GIT_EDITOR=true git commit --amend \
    && GIT_EDITOR=true git rebase --continue \
    && ./find_bad.sh
