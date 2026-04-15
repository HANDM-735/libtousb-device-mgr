#!/bin/bash

if [ -d "build" ]
then
    rm -rf build
fi

TXT=$(cat config/version.txt)
VER_NUM=${TXT:4}
VER_STR=V${VER_NUM}
#echo ${VER_STR}

DEV=$(cat config/device.txt)
#echo ${DEV}

mkdir build
cd build
cmake -DSOFTVERSION="${VER_STR}" -DDEVICETYPE="${DEV}" -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=DEBUG ..
make -j4 VERBOSE=1
cmake --install . --prefix=.

tar -zcvf Test_LibToUSB-Device-Mgr-${VER_STR}.tar.gz ./bin ./lib ./include ./script

if [ -d "../release" ]
then
    rm -rf ../release
fi

mkdir ../release
cp -r ./include    ../release
cp -r ./lib         ../release
cp -r ./bin         ../release
cp -r ./script      ../release
cp -r ./bin/module_install.sh    ../release
