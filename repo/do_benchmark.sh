#!/bin/bash
# HACK: fix if not defined
USER=`whoami`
rm -f /appl/firestarr/firestarr
rm -f /appl/firestarr/tbd
rm -rf /appl/firestarr/build
# move expected files so we can't continue rebase if they change
rm -rf /appl/firestarr/test/10N_50651.orig
scripts/mk_clean.sh > /dev/null 2>&1 && (test/benchmark.sh $@ 2>&1 | tail -n1)
( \
    (git status 2>&1 | grep "deleted: " | sed "s/.*deleted: *//g" | xargs -tI {} git restore {} 2>&1) \
    && git rebase --continue \
) > /dev/null 2>&1 \
&& ./do_benchmark.sh $@
