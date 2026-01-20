#!/bin/bash

BUILD_TYPE="Debug"
BUILD_DIR="code/build"

rm -r $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..