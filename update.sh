#!/bin/bash
cd /appl/TBD
cmake --configure .
cmake --build .
python get_fgmj.py
