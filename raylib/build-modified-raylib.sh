#!/bin/bash

current_dir=$PWD

rm -r build
mkdir build
echo $current_dir
(cd ./build/; cmake -DCUSTOMIZE_BUILD=ON -DBUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=ON .. && make)



