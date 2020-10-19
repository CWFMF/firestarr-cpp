#!/bin/bash
# HACK: fix if not defined
USER=`whoami`
( \
    rm -f /appl/firestarr/firestarr \
    && rm -rf /appl/firestarr/build \
    && scripts/mk_clean.sh \
    && rm -rf /appl/firestarr/test/output/constant \
    && test/constant_one.sh \
) \
&& ( \
    git rebase --continue \
    && $0 $@ \
)
