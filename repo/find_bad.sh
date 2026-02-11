#!/bin/bash
TIMEOUT="36000s"
FILE_TODO=".git/rebase-merge/git-rebase-todo"
DIR_OUT=test/output
TEST_ONE="test/both_one.sh"
# TEST_ONE="test/both_all.sh"
TEST_ALL="test/both_all.sh"
TEST_MAXIMUM="${TEST_ALL}"
TEST_MINIMUM="${TEST_ALL}"
# TEST_MINIMUM="${TEST_ONE}"
# HACK: only test one for now
# TEST_ALL="${TEST_ONE}"
TEST=${TEST_MINIMUM}
# undefine this to not check first run fully
# IS_FIRST=1
IS_FIRST=
v=0
while [ $v -eq 0 ]
do
    # remove commit hash
    msg="$(git log --oneline -n1 | sed 's/^[^ ]* //g')"
    msg_base=$(echo "${msg}" | sed "s/.*: //g" | sed "s/^ *//g")
    if [[ "${msg}" =~ "OLD_CHANGED:" ]]; then
        msg_base="OLD_CHANGED: ${msg_base}"
    fi
    if [[ "${msg}" =~ "BUGFIX:" ]]; then
        msg_base="BUGFIX: ${msg_base}"
    fi
    if [[ "${msg}" =~ "FEATURE:" ]]; then
        msg_base="FEATURE: ${msg_base}"
    fi
    prefix=""
    [ -f "${FILE_TODO}" ]
    v=$?
    if [ $v -eq 0 ]; then
        COMPARE_TO="HEAD~1"
        if [ "$(git log --oneline | wc -l)" -eq 1 ]; then
            COMPARE_TO="HEAD"
        fi
        # if fuel/wx stuff hasn't changed then there shouldn't be any difference in most fuel sims?
        CHANGED_FILES=$(git log --oneline -n1 --name-status | grep src/cpp)
        # if [ "" == "$(echo ${CHANGED_FILES} | grep -vE '(Scenario|Model)')" ]; then
        # always test all for first thing we do, or else we don't know it's actually good compared to previous
        if [ -z "${IS_FIRST}" ] && [ "${TEST_MINIMUM}" != "${TEST_ALL}" ] && [ "" == "$(echo ${CHANGED_FILES} | grep -E '(Duff|FBP45|Fuel|FWI|Weather)')" ]; then
            # only spread algorithm has changed, so if one fuel is the same they all should be
            TEST=${TEST_ONE}
        else
            TEST=${TEST_ALL}
        fi
        echo -e "$(date '+%Y-%m-%d %H:%M:%S')\tChecking:\t${msg}"
        # HACK: set to be all if something changed, so just loop until empty
        while [ "" != "${TEST}" ]
        do
            OUTPUT_ALL=
            # OUTPUT=$(timeout -k 1s 60s ${TEST} $* 2>&1 | grep -E "Final Size: .*, ROS: ")
            OUTPUT_ALL=$(timeout -k 1s ${TIMEOUT} ${TEST} $* 2>&1)
            r1=$?
            OUTPUT=$(echo "${OUTPUT_ALL}" | grep -E "(Aborted|Error 2|Segmentation|Terminated|Final Size: )")
            echo "${OUTPUT}" | grep "options to try"
            if [ "" == "${OUTPUT}" ]; then
                # no ROS but ran
                prefix="NO_ROS: "
            elif [ "$r1" -eq 137 ] || [[ "${OUTPUT}" =~ "Aborted" ]]; then
                # ran out of memory
                prefix="OOM: "
            elif [[ "${OUTPUT}" =~ "Terminated" ]]; then
                # hung
                prefix="HUNG: "
            elif [[ "${OUTPUT}" =~ "Segmentation" ]]; then
                # segfault
                prefix="SEGFAULT: "
            elif [[ "${OUTPUT}" =~ "Error 2" ]]; then
                # make failed
                prefix="BAD: NO_BUILD: "
                break
            elif [[ "${OUTPUT}" =~ .*ROS:\ (.*) ]]; then
                ROS=${BASH_REMATCH[1]}
                if [ $(echo "${ROS} > 0" | bc -l) -eq 1 ]; then
                    # remove BAD: from commit message since it worked
                    # echo "ROS worked"
                    prefix=""
                    # (echo "${msg}" | grep -v "BAD:") \
                    #     || git commit --amend -m "$(echo ${msg})"
                elif [ $(echo "${ROS} < 0" | bc -l) -eq 1 ]; then
                    prefix="ROS_INVALID: "
                else
                    # ROS must be 0?
                    prefix="ROS_ZERO: "
                fi
            else
                # no output and sleep wasn't killed, so failed but didn't hang
                # add BAD: to commit message
                # (echo "${msg}" | grep "BAD:") \
                #     || git commit --amend -m "$(echo BAD: ${msg})"
                prefix="UNKNOWN: "
            fi
            if [ "${prefix}" != "" ]; then
                prefix="BAD: ${prefix}"
                TEST=""
                # if run was bad then keep previous run's output
                rm -rf "${DIR_OUT}" \
                    && git restore --staged "${DIR_OUT}" \
                    && git restore -s ${COMPARE_TO} "${DIR_OUT}"
            else
                # if it was a BAD commit then future bad commits might be okay now
                if [[ "${msg}" =~ "BAD:" ]]; then
                    # make sure all of the BAD commits are marked as edit, or this doesn't make sense
                    sed -i "/BAD:/{s/^pick/edit/g}" "${FILE_TODO}"
                fi
                # msg_new="${prefix}${msg_base}"
                # echo "[ ${msg_new} ] vs [ ${msg} ]"
                # only need to check for changes if not bad, since revert when it is
                # git add "${DIR_OUT}"
                # # fails diff on initial commit, but that's good
                # HACK: check for deleted files in TEST_ONE folder
                if [ "${TEST}" != "${TEST_ALL}" ]; then
                    TEST_FOLDER=$(ls -d1 "${DIR_OUT}")
                    if [ $(echo "${TEST_FILES}" | wc -l) -ne 1 ]; then
                        echo "ERROR: EXPECTED EXACTLY ONE TEST FOLDER"
                        exit -1
                    fi
                    CHANGED_FILES=$(git diff ${COMPARE_TO} --name-status 2>&1 | grep "${DIR_OUT}/${TEST_FOLDER}")
                    if [ "" == "${CHANGED_FILES}" ]; then
                        # if no changed files in test folder then revert all test files
                        git restore "${DIR_OUT}"
                    fi
                fi
                CHANGED_FILES=$([ $(git log --oneline | wc -l) -gt 1 ] && git diff ${COMPARE_TO} --name-status "${DIR_OUT}" 2>&1)
                #  | grep -vE "^D" > /dev/null
                # v=$?
                # if [ $v -eq 0 ]; then
                if [ "" != "${CHANGED_FILES}" ]; then
                    # test output changed
                    prefix="${prefix}CHANGED: "
                    # would have reverted deleted files above if ran only one
                    if [ "${TEST}" == "${TEST_ALL}" ]; then
                        git add "${DIR_OUT}"
                    fi
                    # if we only tested one, but it changed, then we should test all of them
                    if [ "${TEST}" == "${TEST_MAXIMUM}" ]; then
                        TEST=""
                    else
                        TEST="${TEST_MAXIMUM}"
                    fi
                else
                    # no change, so assume no others would change
                    # clear loop variable
                    TEST=""
                    # # revert any deleted test outputs
                    # # no change, so can just revert everthing
                    # git restore --staged -s ${COMPARE_TO} "${DIR_OUT}"
                    # git restore -s ${COMPARE_TO} "${DIR_OUT}"
                    # # # fails diff on initial commit, but that's good
                    # # CHANGED_OUTPUTS=$(git diff ${COMPARE_TO} --name-status 2>&1 | grep "${DIR_OUT}" | grep -E "^D" | sed "s@.*\(${DIR_OUT}/[^/]*\)/.*@\1@g" | uniq)
                    # # #  | xargs -I {} git restore -s ${COMPARE_TO} {} > /dev/null
                    # git add "${DIR_OUT}"
                fi
            fi
        done
        msg_new="${prefix}${msg_base}"
        # echo "[ ${msg_new} ] vs [ ${msg} ]"
        # break
        if [ "${msg_new}" != "${msg}" ]; then
            echo -e "$(date '+%Y-%m-%d %H:%M:%S')\tChanging:\t${msg_new}"
            # whatever it was doesn't match what it should be
            git commit --amend -m "${msg_new}" > /dev/null
        else
            echo -e "$(date '+%Y-%m-%d %H:%M:%S')\tKeeping:\t${msg_new}"
        fi
        # break
        [ -f "${FILE_TODO}" ] \
            && GIT_EDITOR=true git rebase --continue > /dev/null 2>&1
        v=$?
    fi
    # echo "Done loop"
    # break
    IS_FIRST=
done
n=$(git log --oneline | wc -l)
g=$(git log --oneline | grep -v "BAD:" | wc -l)
pct=$(echo "($g/$n)*100" | bc -l | sed "s/\(^[0-9]*\.[0-9]\).*/\1/g")
echo "Done with $g / $n = $pct% good commits"
