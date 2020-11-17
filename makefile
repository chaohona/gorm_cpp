PWD=$(shell pwd)

CXX=g++
DEBUG=-g -ggdb -rdynamic
INC=-I$(PWD)/include -I./thirdpart/yaml/ -I./thirdpart/protobuf/
CXXFLAGS=-std=c++11 -lstdc++ $(DEBUG) $(INC) -fPIC -lpthread  -lm -lrt -lssl -lcrypto -ldl 

GORM_LIB_PATH=./lib/linux/gorm/

GORM_SHARE_CLIENT=$(GORM_LIB_PATH)/libgorm-client.so
GORM_STATIC_CLIENT=$(GORM_LIB_PATH)/libgorm-client.a
GORM_SHARE_TABLES=$(GORM_LIB_PATH)/libgorm-tables.so
GORM_STATIC_TABLES=$(GORM_LIB_PATH)/libgorm-tables.a
GORM_SHARE_LIB=$(GORM_LIB_PATH)/libgorm.so

GORM_CLIENT_H=$(wildcard $(PWD)/include/*.h)
GORM_CLIENT_SRC=$(wildcard $(PWD)/src/*.cc)
GORM_CLIENT_OBJ=$(GORM_CLIENT_SRC:%.cc=%.o)
GORM_TABLE_H=$(wildcard $(PWD)/tables/*.h)
GORM_TABLE_SRC=$(wildcard $(PWD)/tables/*.cc)
GORM_TABLE_OBJ=$(GORM_TABLE_SRC:%.cc=%.o)


$(GORM_SHARE_CLIENT): $(GORM_CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^
	
#$(GORM_STATIC_CLIENT) : $(GORM_CLIENT_H) $(GORM_CLIENT_OBJ)
#	ar rcs $(GORM_STATIC_CLIENT) $(GORM_CLIENT_OBJ)

tables: $(GORM_SHARE_TABLES) $(GORM_STATIC_TABLES)
	mkdir -p $(GORM_LIB_PATH)

all_lib: $(GORM_SHARE_CLIENT) $(GORM_STATIC_CLIENT)
	mkdir -p $(GORM_LIB_PATH)

all: $(GORM_SHARE_CLIENT)
	mkdir -p $(GORM_LIB_PATH)

client: $(GORM_SHARE_CLIENT)
	mkdir -p $(GORM_LIB_PATH)

$(GORM_SHARE_LIB): $(GORM_TABLE_OBJ)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^

gorm: $(GORM_SHARE_LIB)
	mkdir -p $(GORM_LIB_PATH)

clean:
	rm -rf $(PWD)/src/*o
	rm -rf $(PWD)/tables/*o
	rm -rf $(GORM_LIB_PATH)/libgorm-client.so
	rm -rf $(GORM_LIB_PATH)/libgorm.so
