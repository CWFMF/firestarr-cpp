#!/bin/bash
./fix_ws.sh
if [ "$?" -eq 0 ]; then
    grep -r "<<< HEAD" src/cpp
    if [ "$?" -ne 0 ]; then
        rm -f src/cpp/version.cpp
        scripts/mk_clean.sh \
        && (GIT_PAGER=cat git diff -P src/cpp; git add -u) \
        && ( \
            GIT_EDITOR=true git rebase --continue \
            || ( \
                git status \
                && [ -f .git/rebase-merge/git-rebase-todo ] \
                && cat .git/rebase-merge/git-rebase-todo | wc -l \
            ) \
        )
        if [ "$?" -eq 0 ]; then
            git status | grep "deleted by"
            if [ "$?" -eq 0 ]; then
                echo "Deleted files"
                git status \
                    | grep "deleted by" \
                    | sed "s/.* //g" \
                    | xargs -I {} git rm {}
            fi
            if [ "$?" -eq 0 ]; then
                if [ -f .git/rebase-merge/git-rebase-todo ]; then
                    $0
                fi
            fi
        fi
    else
        echo "Invalid merge history"
    fi
fi
