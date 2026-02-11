#!/bin/bash
REV="$1"
REV_ROOT=$(git log --oneline | tail -n1 | cut -d' ' -f1 -)
# REV=$(git log --oneline -n1 --grep "$*" | cut -d' ' -f1 -)
files=$(git log ${REV} -n1 --name-status -- src/cpp \
    | grep -E "^M *" \
    | sed "s/^M[ \t]*//g")
for f in ${files}
do
    # echo $f
    prev=$(git log -n1 --oneline ${REV}~1 -- $f)
    echo $prev
    rev_prev=$(echo $prev | cut -d' ' -f1 -)
    if [ "${rev_prev}" == "${REV_ROOT}" ]; then
        rev_prev="--root"
        rev_prev_parent="${rev_prev}"
        d=$(GIT_PAGER=cat git show -v "${REV}" -- $f)
        n=1
    else
        d=$(GIT_PAGER=cat git diff "${rev_prev}" "${REV}" -- $f)
        n=`git log "${rev_prev}" --oneline | wc -l`
        rev_prev_parent="${rev_prev}~1"
    fi
    echo -e "(change # ${n}) was last change before ${REV} for $f\n\n"
    echo "./rebase.sh ${rev_prev_parent}"
    echo "git restore --source=${REV} -- $f"
    echo "Or to undo:"
    echo "./rebase.sh ${REV}~1"
    # anything before this change wouldn't have the change
    echo "git restore --source=HEAD~1 -- $f"
    echo -e "\n"
    echo "${d}"
    echo -e "\n"
done
