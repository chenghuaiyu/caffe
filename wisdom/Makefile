CC := gcc
AR := ar
CFLAGS = -I./ -I./output -I./decryption -I./postprocess -I./runcaffe -I./util -I../include/ -I/usr/local/cuda-8.0/include/ 
CFLAGS += -D_LINUX_ -fPIC -std=gnu++11 -O3

SRC  := output/cjson.cpp output/outputcjson.cpp output/outputxml.cpp output/output.cpp output/tinyxml2.cpp\
	decryption/rc4.cpp decryption/decryption.cpp preprocess.cpp \
	postprocess/postprocess_normal.cpp postprocess/postprocess_case1.cpp \
	postprocess/postprocess_classify.cpp postprocess/postprocess_classify_uniform.cpp \
	postprocess/postprocess.cpp \
	runcaffe/run_caffe_normal.cpp runcaffe/run_caffe_case1.cpp runcaffe/run_caffe_case2.cpp runcaffe/run_caffe.cpp \
	config/get_config_normal.cpp config/get_config_xml.cpp config/get_config.cpp \
	util/util.cpp detection.cpp tools/encrypt.cpp

OBJ  := $(SRC:%.cpp=%.o)

TARGET := libdetect.a
all: $(TARGET)

$(OBJ): %.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@ 

$(TARGET): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^ 

clean:
	rm *.o *.a -rf
	find . -name "*.o"|xargs rm -rf

