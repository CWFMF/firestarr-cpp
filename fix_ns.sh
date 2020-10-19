#!/bin/bash
n=$(git log --oneline wip | wc -l)
while [ -f .git/rebase-merge/git-rebase-todo ]
do
    n_old=$n
    FILES=$(git status | grep "both modified:" | sed "s/.*src/src/g")
    ( \
        git checkout --theirs ${FILES};
        ls -1 src/cpp/ \
            | xargs -I {} sed -i "s/namespace fs::sim/namespace fs/g;s/sim:://g;s/using [A-Za-z]*;$//g;" src/cpp/{};
        ls -1 src/cpp/ | grep -v "Main.cpp" \
            | xargs -I {} sed -i "s/using fs::[A-Z].*;$//g" src/cpp/{};
    ) \
    && scripts/mk_clean.sh $* \
    && git add src/cpp \
    && GIT_EDITOR=true git rebase --continue
    n=$(cat .git/rebase-merge/git-rebase-todo | wc -l)
    if [[ "$n" -eq "$n_old" ]]; then
        break
    fi
done
