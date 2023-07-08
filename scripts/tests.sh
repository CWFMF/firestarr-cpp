pushd /appl/tbd
make && ./tbd "/appl/data/sims/202306061927/TIM_FIRE_007/firestarr" 2023-06-06 48.00648101397005 -81.94583730095735 12:00 -v --output_date_offsets "{1}" --wx "/appl/data/sims/202306061927/TIM_FIRE_007/firestarr/wx.csv" --perim "/appl/data/sims/202306061927/TIM_FIRE_007/firestarr/TIM_FIRE_007.tif" -v -v


make && ./tbd "/appl/data/sims/202306061927/TIM_FIRE_007/firestarr" 2023-06-06 48.00648101397005 -81.94583730095735 12:00 -v --output_date_offsets "{1}" --wx "/appl/data/sims/202306061927/TIM_FIRE_007/firestarr/wx.csv" --size 50


rm -rf /appl/data/sims/202306061927/TIM_FIRE_007/firestarr && python tbd.py /appl/data/sims/202306061927/TIM_FIRE_007

cmake -DCMAKE_BUILD_TYPE=Release --build .

make && gdb --ex run --args ./tbd "/appl/data/sims/202306071136/173/firestarr" 2023-06-07 48.16189756450941 -70.87153184826877 06:00 -v --output_date_offsets "{1}" --wx "/appl/data/sims/202306071136/173/firestarr/wx.csv" --perim "/appl/data/sims/202306071136/173/firestarr/173.tif" -s -v -v

make && ./tbd "/appl/data/sims/202306071136/173/firestarr" 2023-06-07 48.16189756450941 -70.87153184826877 06:00 -v --output_date_offsets "{1}" --wx "/appl/data/sims/202306071136/173/firestarr/wx.csv" --perim "/appl/data/sims/202306071136/173/firestarr/173.tif" -s -v -v

./tbd "/appl/data/sims/202306071610/m3_polygons_current.fid--5df00578_188966d004d_-144c/firestarr" 2023-06-07 52.01094601942008 -114.46131704214307 06:00 -v --output_date_offsets "{1}" --wx "/appl/data/sims/202306071610/m3_polygons_current.fid--5df00578_188966d004d_-144c/firestarr/wx.csv" --perim "/appl/data/sims/202306071610/m3_polygons_current.fid--5df00578_188966d004d_-144c/firestarr/m3_polygons_current.fid--5df00578_188966d004d_-144c.tif"

./tbd "/appl/data/sims/202306072005/m3_polygons_current.fid--5df00578_1889711c91e_1b8e/firestarr" 2023-06-07 50.207399113204794 -74.53369925959724 12:00 -v --output_date_offsets "{1, 2, 3, 7, 14}" --wx "/appl/data/sims/202306072005/m3_polygons_current.fid--5df00578_1889711c91e_1b8e/firestarr/wx.csv" --perim "/appl/data/sims/202306072005/m3_polygons_current.fid--5df00578_1889711c91e_1b8e/firestarr/m3_polygons_current.fid--5df00578_1889711c91e_1b8e.tif"


python tbd.py /appl/data/sims/202306072134/TIM_FIRE_007

make && time ./tbd "/appl/data/sims/202306072134/TIM_FIRE_007/firestarr" 2023-06-07 48.00268306042794 -81.94836625102865 12:00 -v --output_date_offsets "{1, 2, 3, 7}" --wx "/appl/data/sims/202306072134/TIM_FIRE_007/firestarr/wx.csv" --perim "/appl/data/sims/202306072134/TIM_FIRE_007/firestarr/TIM_FIRE_007.tif" -v
popd
