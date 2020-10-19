#!/bin/bash
if [ -f .git/rebase-merge/git-rebase-todo ]; then
    git status
    echo "Already rebasing"
else
    D=$(date +"%Y%m%d_%H%M")
    USE_ROOT=
    if [ "" == "$1" ]; then
        USE_ROOT="--root"
    fi
    git branch cpp23_${D}
    git rebase -i --committer-date-is-author-date --empty=drop ${USE_ROOT} $*
fi
