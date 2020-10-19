#1/bin/bash
# for f in `ls src/cpp/*`; do
#     printf "%03d $f\n" `git log --oneline cpp23 $f | wc -l`
# done
git log --name-status --oneline cpp23 --diff-filter=M \
    | grep 'src/cpp' \
    | sed 's/^M *//g' \
    | sort \
    | uniq -c \
    | sort -n \
    > changes.cnt
