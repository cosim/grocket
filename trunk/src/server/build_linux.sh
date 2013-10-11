#!/bin/bash

#如果发现gperftools-2.1链接不过的原因是pthread_atfork找不到实现，
#那是你的pthread库版本太低了，打开../third/gperftools-2.1/src/config.h 文件
#把#define HAVE_FORK 1那行注释掉，再重新执行
#make -f Makefile.linux clean
#make -f Makefile.linux
#就行了

cd ../third
rm -fr gperftools-2.1
tar -zxvf gperftools-2.1.tar.gz
cd gperftools-2.1
./configure
cd ..
cd ..

cd server
make -f Makefile.linux clean
make -f Makefile.linux
