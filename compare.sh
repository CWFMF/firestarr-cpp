cmake --build cmake-build-debug -j 48
cmake --build cmake-build-release -j 48
cmake-build-debug/FireSTARR ./Data/output.debug 2017-08-27 52.01 -89.024 12:15 -i -v -v --wx test/wx.csv --ffmc 90 --dmc 40 --dc 300 --apcp_0800 0
cmake-build-release/FireSTARR ./Data/output.release 2017-08-27 52.01 -89.024 12:15 -i -v -v --wx test/wx.csv --ffmc 90 --dmc 40 --dc 300 --apcp_0800 0
