#!/bin/bash/
sudo apt-get update
cd
wget https://github.com/libvmi/libvmi/archive/v0.10.1.tar.gz
tar xvzf v0.10.1.tar.gz
cd libvmi-0.10.1
./autogen.sh
./configure --disable-xen
sudo make install

