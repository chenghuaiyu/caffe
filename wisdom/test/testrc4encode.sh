g++ testrc4encode.cpp -MMD -MP -pthread -fPIC -DCAFFE_VERSION=1.0.0-rc3 -DNDEBUG -O2 -std=c++11 -DUSE_CUDNN -DUSE_OPENCV -DUSE_LEVELDB -DUSE_LMDB -I/usr/include/python2.7 -I/usr/lib/python2.7/dist-packages/numpy/core/include -I/usr/local/include -I/usr/include/hdf5/serial \
-D_LINUX_ -I../ \
-I../../build/src \
-I../../src \
-I../../include \
-I/usr/local/cuda/include -Wall -Wno-sign-compare -c -o testrc4encode.o

g++ testrc4encode.o -o testrc4encode.bin ../libdetect.a -pthread -fPIC -DCAFFE_VERSION=1.0.0-rc3 -DNDEBUG -O2 -std=c++11 -DUSE_CUDNN -DUSE_OPENCV -DUSE_LEVELDB -DUSE_LMDB -I/usr/include/python2.7 -I/usr/lib/python2.7/dist-packages/numpy/core/include -I/usr/local/include -I/usr/include/hdf5/serial \
-I../../build/src \
-I../../src \
-I../../include \
-I/usr/local/cuda/include -Wall -Wno-sign-compare -lcaffe -L/usr/lib -L/usr/local/lib -L/usr/lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu/hdf5/serial -L/usr/local/cuda/lib64 -L/usr/local/cuda/lib \
-L../../build/lib  \
-lcudart -lcublas -lcurand -lglog -lgflags -lprotobuf -lboost_system -lboost_filesystem -lm -lhdf5_hl -lhdf5 -lleveldb -lsnappy -llmdb -lopencv_core -lopencv_highgui -lopencv_imgproc -lboost_thread -lstdc++ -lcudnn -lcblas -latlas \
        -Wl,-rpath,\$ORIGIN/../lib

rm *.o *.d
./testrc4encode.bin 0 deploy.prototxt SSD_300x300/VGG_VOC0712_SSD_300x300_iter_60000.caffemodel rc4.caffemodel 12345678901234567890
./testrc4encode.bin 1 rc4.caffemodel tmp.prototxt tmp.caffemodel 12345678901234567890
