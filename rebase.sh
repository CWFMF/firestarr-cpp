#!/bin/bash
# HACK: don't go right back to root since want to start from all fuels
ROOT_MSG="added mixedwood 0% PDF and PC fuels, and seasonal grass"
if [ -f .git/rebase-merge/git-rebase-todo ]; then
    git status
    echo "Already rebasing"
else
    D=$(date +"%Y%m%d_%H%M")
    USE_ROOT=
    if [ "" == "$1" ]; then
        # USE_ROOT="--root"
        USE_ROOT=$(git log --oneline -n1 --grep "${ROOT_MSG}" | cut -d' ' -f1)~1
    fi
    git branch wip_${D}
    git rebase -i --committer-date-is-author-date --empty=drop ${USE_ROOT} $*
fi
