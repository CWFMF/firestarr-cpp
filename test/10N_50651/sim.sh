#!/bin/bash
/appl/tbd/tbd "." 2024-06-03 58.81228184403946 -122.9117103995713 01:00 --no-intensity --ffmc 89.9 --dmc 59.5 --dc 450.9 --apcp_prev 0 -v --output_date_offsets [1,2,3,7,14] --wx firestarr_10N_50651_wx.csv --perim 10N_50651.tif 2>&1 | tee -a from_tee.log $*
