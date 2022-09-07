export ARGS="2017-08-27 52.01 -89.024 12:15 $* --wx test/wx_hourly_in.csv"
cmake -S . -B cmake-build-release -D CMAKE_BUILD_TYPE=Release && cmake --build cmake-build-release -j 48 && time cmake-build-release/tbd ./data/output.hourly ${ARGS}
