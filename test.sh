#!/bin/bash


./conv.sh --gorm=true --conf_in_file=./conf/gorm-db.xml --conf_out_path=./conf/ --cpp_out_path=./tables/ --lib_out_path=/usr/local/lib --yaml_inc_path=./thirdpart/yaml/ --proto_inc_path=./thirdpart/protobuf/ --lib_out_path=./lib/linux/gorm/ --protoc_path=./bin
