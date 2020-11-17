@echo off

set pwd=%~dp0



.\bin\gorm-conv.exe --pb=true --sql=true --xml=conf\gorm-db.xml -O=.\conf\ --cpppath=.\tables\ --codetype="client" --protoversion="3" --cpp_coroutine="true"


.\bin\protoc.exe --proto_path=.\conf\  --cpp_out=.\tables\ .\conf\gorm-db.proto .\conf\gorm_pb_proto.proto .\conf\gorm_pb_tables_inc.proto --experimental_allow_proto3_optional
