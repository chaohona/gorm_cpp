#!/bin/bash

git checkout makefile

make clean

git add .
git commit -m"update"
git push
