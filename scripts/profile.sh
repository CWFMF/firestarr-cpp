sudo sysctl -w kernel.perf_event_paranoid=1
IS_PASTED=
if [[ "$0" =~ "/bash" ]]; then
  DIR_TEST=`realpath test`
  IS_PASTED=1
else
  set -e
  DIR_TEST="$(dirname $(realpath "$0"))"
fi
USER=$(whoami)
DIR_ROOT=$(dirname "${DIR_TEST}")
DIR_BUILD="${DIR_ROOT}/build"
DIR_PERF=${DIR_ROOT}
FILE_PERF=${DIR_PERF}/perf.data
FREQ=100
DIR_FG=/appl/FlameGraph
[ ! -d "${DIR_PERF}" ] && mkdir -p "${DIR_PERF}"
# don't make this an actual submodule because it's just this script that needs it
[ ! -d "${DIR_FG}" ] && sudo apt install git && sudo git clone https://github.com/brendangregg/FlameGraph.git ${DIR_FG} && sudo chown -R ${USER}:${USER} ${DIR_FG}
rm -rf ${DIR_BUILD}
cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo -S${DIR_ROOT} -B${DIR_BUILD} -G "Unix Makefiles" > /dev/null 2>&1
cmake --build ${DIR_BUILD} --config RelWithDebInfo --target all -j $(nproc) -- > /dev/null 2>&1
cp ${DIR_BUILD}/firestarr ${DIR_ROOT}/
time sudo perf record -o ${FILE_PERF} -F ${FREQ} -g --call-graph=dwarf -- $* > ${DIR_PERF}/run.log
sudo chown -R ${USER}:${USER} ${DIR_PERF}
perf script -i ${FILE_PERF} | ${DIR_FG}/stackcollapse-perf.pl | ${DIR_FG}/flamegraph.pl > flame.html
