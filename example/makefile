# ================================================================
# Makefile for gorm example
#
# Date:   2020-06-29
# ================================================================

GORM_HOME=$(PWD)/../
GORM_THIRD_PART=$(GORM_HOME)/thirdpart/

GORM_LIB_PATH=$(GORM_HOME)/lib/linux/

GORM_SHARE_LIB=-L$(GORM_LIB_PATH)/gorm -lpthread  -lm -lrt -lssl -lcrypto -ldl -lgorm -lgorm-client -lprotobuf -lyaml-cpp -lgorm

GORM_INC =-I$(GORM_HOME)/include/ -I$(GORM_HOME)/tables/


.PHONY: all clean 

all:
	g++ -std=c++11 -lstdc++ -g -ggdb -rdynamic -o gorm_test main.cc ${GORM_INC} ${GORM_SHARE_LIB}

clean:
	rm -f gorm_test
