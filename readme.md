GORM客户端CPP版本<br/>
使用方式:<br/>
1、生成表对应的struct<br/>
./conv.sh --gorm=true --conf_in_file=./conf/gorm-db.xml --conf_out_path=./conf/ --cpp_out_path=./tables/ --lib_out_path=/usr/local/lib --yaml_inc_path=./thirdpart/yaml/ --proto_inc_path=./thirdpart/protobuf/ --lib_out_path=./lib/linux/gorm/ --protoc_path./bin/protoc <br/>
参数说明：<br/>
conf_in_file：xml配置文件所在路径<br/>
conf_out_path：输出配置文件所在路径<br/>
cpp_out_path：输出的中间代码所在路径<br/>
lib_out_path：libgorm-client库使用路径，需要将此库拷贝到响应路径<br/>
yaml_inc_path: yaml头文件地址
proto_inc_path: proto头文件地址
lib_out_path: libgorm-client.so gorm基础库输出地址
protoc_path: protoc 文件所在目录
2、使用表结构<br/>
代码例子见example/main.cc
