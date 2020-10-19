#!/bin/bash
# HACK: fix if not defined
USER=`whoami`
( \
    rm -f /appl/firestarr/firestarr \
    && rm -rf /appl/firestarr/build \
    && scripts/mk_clean.sh \
    && rm -rf /appl/firestarr/test/data/constant \
    && test/constant.sh \
) \
&& ( \
    git rebase --continue \
    && $0 $@ \
)
