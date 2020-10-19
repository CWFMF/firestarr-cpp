#!/bin/bash
scripts/build.sh $* \
    && git rebase --continue \
    && $0 $*
