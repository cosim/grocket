#!/bin/bash

cd ../third
rm -fr gperftools-2.1
tar -zxvf gperftools-2.1.tar.gz
cd gperftools-2.1
sh ./configure
cat src/config.h | grep -v "HAVE_FORK" > src/config.h.daming.mac.new
rm -fr src/config.h
mv src/config.h.daming.mac.new src/config.h
cd ..
cd ..

cd server
make -f Makefile.mac clean
make -f Makefile.mac
