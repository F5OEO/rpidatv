#!/bin/bash

# Updated by davecrump 20161220

# Modified to overwrite ~/rpidatv/scripts and
# ~/rpidatv/src, then compile
# rpidatv, rpidatv gui avc2ts and adf4351

printf "\nCommencing update.\n"

# Note previous version number
cp -f -r /home/pi/rpidatv/scripts/installed_version.txt /home/pi/prev_installed_version.txt

# Make a safe copy of rpidatvconfig.txt
cp -f -r /home/pi/rpidatv/scripts/rpidatvconfig.txt /home/pi/rpidatvconfig.txt

set -e  # Don't report errors

# Check if fbi (frame buffer imager) needs to be installed

if [ ! -f "/usr/bin/fbi" ]; then
  sudo apt-get -y install fbi
fi

# ---------- Update rpidatv -----------

cd /home/pi
wget -q https://github.com/davecrump/rpidatv/archive/master.zip -O master.zip
unzip -o master.zip 
# cp -f -r rpidatv-master/bin rpidatv
# cp -f -r rpidatv-master/doc rpidatv
cp -f -r rpidatv-master/scripts rpidatv
cp -f -r rpidatv-master/src rpidatv
cp -f -r rpidatv-master/video rpidatv
rm master.zip
rm -rf rpidatv-master

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
cd /home/pi

## Get tstools
# cd /home/pi/rpidatv/src
# wget https://github.com/F5OEO/tstools/archive/master.zip
# unzip master.zip
# rm -rf tstools
# mv tstools-master tstools
# rm master.zip

## Compile tstools
#cd tstools
#make
#cp bin/ts2es ../../bin/

## install H264 Decoder : hello_video
## compile ilcomponet first
#cd /opt/vc/src/hello_pi/
#sudo ./rebuild.sh

# cd /home/pi/rpidatv/src/hello_video
# make
#cp hello_video.bin ../../bin/

## TouchScreen GUI
## FBCP : Duplicate Framebuffer 0 -> 1
#cd /home/pi/
#wget https://github.com/tasanakorn/rpi-fbcp/archive/master.zip
#unzip master.zip
#rm -rf rpi-fbcp
#mv rpi-fbcp-master rpi-fbcp
#rm master.zip

## Compile fbcp
#cd rpi-fbcp/
#rm -rf build
#mkdir build
#cd build/
#cmake ..
#make
#sudo install fbcp /usr/local/bin/fbcp
#cd ../../

# Restore rpidatvconfig.txt
# Comment out if upgrade changes format of file
cp -f -r /home/pi/rpidatvconfig.txt /home/pi/rpidatv/scripts/rpidatvconfig.txt
rm -rf /home/pi/rpidatvconfig.txt

# Update the version number
rm -rf /home/pi/rpidatv/scripts/installed_version.txt
cp /home/pi/rpidatv/scripts/latest_version.txt /home/pi/rpidatv/scripts/installed_version.txt
cp -f -r /home/pi/prev_installed_version.txt /home/pi/rpidatv/scripts/prev_installed_version.txt
rm -rf /home/pi/prev_installed_version.txt

# Offer reboot
printf "A reboot will be required before using the update.\n"
printf "Do you want to reboot now? (y/n)\n"
read -n 1
printf "\n"
if [[ "$REPLY" = "y" || "$REPLY" = "Y" ]]; then
  printf "\nRebooting\n"
  sudo reboot now
fi
exit
