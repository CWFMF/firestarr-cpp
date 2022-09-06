#!/bin/bash
cd /appl/tbd
cmake --configure .
cmake --build .
python get_fgmj.py
