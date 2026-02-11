#!/bin/bash
if [ "" != "$1" ]; then
    if [ "" != "$2" ]; then
        ./fix_ws.sh \
            && sed -i "s/double \([A-Za-z_]*${1}[A-Za-z_]*\)/${2} \1/g" src/cpp/* \
            && scripts/build.sh \
            && git add -u \
            && GIT_EDITOR=true git rebase --continue
        # when rebase fails then call again
        if [ -f .git/rebase-merge/git-rebase-todo ]; then
            $0 $*
        fi
    fi
fi
