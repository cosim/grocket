#!/bin/bash

cd ../third
rm -fr gperftools-2.1
tar -zxvf gperftools-2.1.tar.gz
cd gperftools-2.1
sh ./configure
cd ..
cd ..

cd server
make -f Makefile.mac clean
make -f Makefile.mac
