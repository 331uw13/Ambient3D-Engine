#!/bin/bash

if [[ -z $1 ]]; then
    echo "Add commit message."
    exit
fi

make clean
(cd server/ && make clean)
(cd testing/ && make clean)

git add .
git commit -m "$1"
git push -u origin main

