#!/bin/bash


make clean
make client
make gorm
cp -rf ./lib/linux/gorm/* /usr/local/lib/
