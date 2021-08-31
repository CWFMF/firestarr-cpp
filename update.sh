#!/bin/bash
cd /FireGUARD/FireSTARR
cmake --configure .
cmake --build .
python get_fgmj.py
