#!/bin/bash

DIR_INITIAL=$(find ../data/initial/ -maxdepth 1 -mindepth 1 -type d | sort | grep -v firestarr | tail -n1)
test/both_all.sh \
    && diff -rq -x firestarr.log ${DIR_INITIAL} test/output \
    && git add test/output \
    && GIT_EDITOR=true git commit --amend \
    && GIT_EDITOR=true git rebase --continue \
    && ./find_bad.sh
