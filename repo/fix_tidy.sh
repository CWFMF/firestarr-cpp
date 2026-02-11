#!/bin/bash
# ./fix_ws.sh \
#     && scripts/build.sh \
#     && git add -u \
#     && ( \
#         GIT_EDITOR=true ./chk_build.sh \
#             || $0 \
#     )
scripts/build.sh \
    && git diff >> changelog.diff \
    && git add -u src/cpp \
    && ( \
        GIT_EDITOR=true git rebase --continue \
            && $0 \
    )
