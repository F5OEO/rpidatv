#!/bin/bash

# Updated by davecrump 20161214

set -e  # Don't report errors

# ---------- Update rpidatv -----------

cd /home/pi
wget https://github.com/F5OEO/rpidatv/archive/master.zip -O master.zip
unzip -o master.zip 
cp -f -r rpidatv-master rpidatv
rm master.zip

# Compile rpidatv core
cd rpidatv/src
make clean
make
sudo make install

# Compile rpidatv gui
cd gui
make clean
make
sudo make install
cd ../

# Compile avc2ts
cd avc2ts
make clean
make
sudo make install

#install adf4351
cd /home/pi/rpidatv/src/adf4351
make
cp adf4351 ../../bin/
cd /home/pi/rpidatv

# Offer reboot
printf "A reboot will be required before using the update."
printf "Do you want to reboot now? (y/n)\n"
read -n 1
printf "\n"
if [[ "$REPLY" = "y" || "$REPLY" = "Y" ]]; then
  echo "rebooting"
  sudo reboot now
fi
exit


