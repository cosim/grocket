#!/bin/bash

#�������gperftools-2.1���Ӳ�����ԭ����pthread_atfork�Ҳ���ʵ�֣�
#�������pthread��汾̫���ˣ���../third/gperftools-2.1/src/config.h �ļ�
#��#define HAVE_FORK 1����ע�͵���������ִ��
#make -f Makefile.linux clean
#make -f Makefile.linux
#������

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
