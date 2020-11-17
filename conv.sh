#!/bin/bash

exit_script() {
	TOP_PID=$$
	kill -s TERM $TOP_PID
}

if [ $# -lt 3 ]; then
    echo "传入参数不对"
    echo "用法: conv.sh --conf_in_file=xmlfile.xml --conf_out_path=./conf/ --cpp_out_path=./src/"
    exit_script
fi


now_path=`pwd`


conf_in_file=${now_path}/conf/gorm-db.xml
conf_in_path=${now_path}/conf/
cpp_out_path=${now_path}/cpp_out/
conf_out_path=${now_path}/conf_out/
yaml_inc_path=${now_path}/thirdpart/yaml/
proto_inc_path=${now_path}/thirdpart/protobuf/
lib_out_path=${now_path}/lib/linux/gorm/
protoc_path=${now_path}/bin/

# 解析参数
TEMP=`getopt -a --longoptions protoc_path:,proto_inc_path:,yaml_inc_path:,lib_out_path:,conf_in_file:,conf_in_path:,conf_out_path:,cpp_out_path:,test:: -n "$0" -- "$@"`
eval set -- "${TEMP}"

while true
do
    case "$1" in
        --test)
            shift 2;
            ;;
        --protoc_path)
		    case "$2" in
                "")
                    echo "Option protoc_path, no argument";
                    shift  
                    ;;
                *)
                    protoc_path=$2
                    shift 2;
                    ;;
            esac
            ;;
        --lib_out_path)
		    case "$2" in
                "")
                    echo "Option lib_out_path, no argument";
                    shift  
                    ;;
                *)
                    lib_out_path=$2
                    shift 2;
                    ;;
            esac
            ;;
        --yaml_inc_path)
            case "$2" in
                "")
                    echo "Option yaml_inc_path, no argument";
                    shift  
                    ;;
                *)
                    yaml_inc_path=$2
                    shift 2;
                    ;;
            esac
            ;;
        --proto_inc_path)
            case "$2" in
                "")
                    echo "Option proto_inc_path, no argument";
                    shift  
                    ;;
                *)
                    proto_inc_path=$2
                    shift 2;
                    ;;
            esac
            ;;
        --conf_in_file)
            case "$2" in
                "")
                    echo "Option conf_in_file, no argument";
                    shift  
                    ;;
                *)
                    conf_in_file=$2
                    shift 2;
                    ;;
            esac
            ;;
        --conf_in_path)
            case "$2" in
                "")
                    echo "Option conf_in_path, no argument";
                    shift  
                    ;;
                *)
                    conf_in_path=$2
                    shift 2;
                    ;;
            esac
            ;;
        --conf_out_path)
            case "$2" in
                "")
                    echo "Option conf_out_path, no argument";
                    shift  
                    ;;
                *)
                    conf_out_path=$2
                    shift 2;
                    ;;
            esac
            ;;
        --cpp_out_path)
            case "$2" in
                "")
                    echo "Option cpp_out_path, no argument";
                    shift  
                    ;;
                *)
                    cpp_out_path=$2
                    shift 2;
                    ;;
            esac
            ;;
        --)
            echo "got --"
            shift
            break
            ;;
        *)
            echo $1
            echo "Internal error!"
            shift
            ;;
    esac
done
# 解析参数结束

chmod +x ${now_path}/bin/gorm-conv
chmod +x ${now_path}/bin/protoc

mkdir -p ${conf_out_path}
mkdir -p ${cpp_out_path}

# 生成中间配置与代码
${now_path}/bin/gorm-conv --pb=true --sql=true --xml=${conf_in_file} -O=${conf_out_path} --cpppath=${cpp_out_path} --codetype="client" --protoversion="3"

${protoc_path}/protoc --proto_path=${conf_out_path}/  --cpp_out=${cpp_out_path}/ ${conf_out_path}/gorm-db.proto ${conf_out_path}/gorm_pb_proto.proto ${conf_out_path}/gorm_pb_tables_inc.proto --experimental_allow_proto3_optional

rm -rf makefile
cp -rf makefile.bak makefile

sed -i s#YAML_INC_PATH#${yaml_inc_path}#g makefile
sed -i s#PROTO_INC_PATH#${proto_inc_path}#g makefile
sed -i s#OUT_LIB_PATH#${lib_out_path}#g makefile
