#!/bin/bash
BRANCH=wip
LAST=$(git log ${BRANCH} --oneline --grep "revert to c++20" | cut -d' ' -f1)~1
NUM=$(git log ${BRANCH} --oneline ${LAST} | wc -l)
FIXES=$(git log ${BRANCH} --oneline ${LAST} -E --grep "(FEATURE|BUGFIX):" | wc -l)
# ignore commits that are fixes pushed to start of history
COMMITS=$(( ${NUM} - ${FIXES} - 1 ))
git log ${BRANCH} --name-status --oneline ${LAST} -n${COMMITS} \
    | grep -P 'M[ \t]*src/cpp' \
    | sed 's/^M *//g' \
    | sort \
    | uniq -c \
    | sort -n \
    > changes.cnt
