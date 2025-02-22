#!/bin/sh
#
# Instruction for build MUD in Linux
#
# Enter to MUD root dir (where CMakeLists.txt)
# mkdir build
# cp cmake1.sh build
# cd build
# ./cmake1.sh
# make
# cp circle ../bin
# cd ..
# bin/circle
#
# PROFIT!
#
# Prool, proolix@gmail.com, www.prool.kharkov.org
# 1 Sep 2021, Kharkiv, Ukraine
# Knowledge is Power (F.Bacon)
#
cmake -DSCRIPTING=NO -DBUILD_TESTS=NO -DCMAKE_BUILD_TYPE=Release ..
