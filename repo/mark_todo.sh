#!/bin/bash
git log wip --oneline "$@" \
    | sed "s/ .*//g" \
    | xargs -tI {} sed -i "s/pick {}/edit {}/" .git/rebase-merge/git-rebase-todo
