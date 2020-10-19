#!/bin/bash
grep -r "<<< HEAD" src/cpp/ \
|| ( \
    git rebase --continue \
    && scripts/mk_clean.sh \
    && rm -rf firestarr build \
    && $0 \
)
