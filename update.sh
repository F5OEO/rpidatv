#!/bin/bash

set -e

# ---------- Update rpidatv -----------

cd /home/pi
#git clone git://github.com/F5OEO/rpidatv -> BUG IN QEMU : Go to download method
wget https://github.com/F5OEO/rpidatv/archive/master.zip -O master.zip
unzip -o master.zip 
cp -f -r rpidatv-master rpidatv
rm master.zip

#rpidatv core
cd rpidatv/src
make clean
make
sudo make install
#rpidatv gui
cd gui
make clean
make
sudo make install
cd ../
#avc2ts
cd avc2ts
make clean
make
sudo make install



