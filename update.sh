#!/bin/bash

# Updated by davecrump 201702021

# Modified to overwrite ~/rpidatv/scripts and
# ~/rpidatv/src, then compile
# rpidatv, rpidatv gui avc2ts and adf4351

reset

printf "\nCommencing update.\n"

# Note previous version number
cp -f -r /home/pi/rpidatv/scripts/installed_version.txt /home/pi/prev_installed_version.txt

# Make a safe copy of rpidatvconfig.txt
cp -f -r /home/pi/rpidatv/scripts/rpidatvconfig.txt /home/pi/rpidatvconfig.txt

# Check if fbi (frame buffer imager) needs to be installed

if [ ! -f "/usr/bin/fbi" ]; then
  sudo apt-get -y install fbi
fi

# ---------- Update rpidatv -----------

cd /home/pi

# Check which source to download.  Default is production
# option -d is development from davecrump
# option -s is staging from batc/staging
if [ "$1" == "-d" ]; then
  echo "Installing development load"
  wget https://github.com/davecrump/rpidatv/archive/master.zip -O master.zip
elif [ "$1" == "-s" ]; then
  echo "Installing BATC Staging load"
  wget https://github.com/BritishAmateurTelevisionClub/rpidatv/archive/batc_staging.zip -O master.zip
else
  echo "Installing BATC Production load"
  wget https://github.com/BritishAmateurTelevisionClub/rpidatv/archive/master.zip -O master.zip
fi

# Unzip and overwrite where we need to
unzip -o master.zip

if [ "$1" == "-s" ]; then
  # cp -f -r rpidatv-batc_staging/bin rpidatv
  # cp -f -r rpidatv-batc_staging/doc rpidatv
  cp -f -r rpidatv-batc_staging/scripts rpidatv
  cp -f -r rpidatv-batc_staging/src rpidatv
  rm -f rpidatv/video/*.jpg
  cp -f -r rpidatv-batc_staging/video rpidatv
  rm master.zip
  rm -rf rpidatv-batc_staging
else
  # cp -f -r rpidatv-master/bin rpidatv
  # cp -f -r rpidatv-master/doc rpidatv
  cp -f -r rpidatv-master/scripts rpidatv
  cp -f -r rpidatv-master/src rpidatv
  rm -f rpidatv/video/*.jpg
  cp -f -r rpidatv-master/video rpidatv
  rm master.zip
  rm -rf rpidatv-master
fi

# Compile rpidatv core
sudo killall -9 rpidatv
cd rpidatv/src
make clean
make
sudo make install

# Compile rpidatv gui
sudo killall -9 rpidatvgui
cd gui
make clean
make
sudo make install
cd ../

# Compile avc2ts
sudo killall -9 avc2ts
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

# Disable fallback IP (201701230)

cd /etc
sudo sed -i '/profile static_eth0/d' dhcpcd.conf
sudo sed -i '/static ip_address=192.168.1.60/d' dhcpcd.conf
sudo sed -i '/static routers=192.168.1.1/d' dhcpcd.conf
sudo sed -i '/static domain_name_servers=192.168.1.1/d' dhcpcd.conf
sudo sed -i '/interface eth0/d' dhcpcd.conf
sudo sed -i '/fallback static_eth0/d' dhcpcd.conf

# Disable the Touchscreen Screensaver (201701070)
cd /boot
if ! grep -q consoleblank cmdline.txt; then
  sudo sed -i -e 's/rootwait/rootwait consoleblank=0/' cmdline.txt
fi
cd /etc/kbd
sudo sed -i 's/^BLANK_TIME.*/BLANK_TIME=0/' config
sudo sed -i 's/^POWERDOWN_TIME.*/POWERDOWN_TIME=0/' config
cd /home/pi

# Delete, download, amend, compile and install DATV Express-server (201702021)

if [ ! -f "/bin/netcat" ]; then
  sudo apt-get -y install netcat
fi

sudo rm -f -r /lib/firmware/datvexpress
sudo rm -f /usr/bin/express_server
sudo rm -f /etc/udev/rules.d/10-datvexpress.rules
cd /home/pi
wget https://github.com/G4GUO/express_server/archive/master.zip -O master.zip
unzip master.zip
mv express_server-master express_server
rm master.zip
cd /home/pi/express_server
sed -i 's/^     express_handle_events( 32 ).*/     express_handle_events( 1 );/' express.cpp
make
sudo make install

# Update pi-sdn (201702020)
rm -f /home/pi/pi-sdn
wget 'https://github.com/philcrump/pi-sdn/releases/download/v1.1/pi-sdn' -O /home/pi/pi-sdn
chmod +x /home/pi/pi-sdn
# Update the call to pi-sdn if it is enabled (201702020)
if [ -f /home/pi/.pi-sdn ]; then
  rm -f /home/pi/.pi-sdn
  cp /home/pi/rpidatv/scripts/configs/text.pi-sdn /home/pi/.pi-sdn
fi

# Restore or update rpidatvconfig.txt for 201701020 and 201701270
if ! grep -q adfref /home/pi/rpidatvconfig.txt; then
  # File needs updating
  source /home/pi/rpidatv/scripts/copy_config.sh
else
  # File is correct format
  cp -f -r /home/pi/rpidatvconfig.txt /home/pi/rpidatv/scripts/rpidatvconfig.txt
fi
rm -f /home/pi/rpidatvconfig.txt
rm -f /home/pi/rpidatv/scripts/copy_config.sh

# Update the version number
rm -rf /home/pi/rpidatv/scripts/installed_version.txt
cp /home/pi/rpidatv/scripts/latest_version.txt /home/pi/rpidatv/scripts/installed_version.txt
cp -f -r /home/pi/prev_installed_version.txt /home/pi/rpidatv/scripts/prev_installed_version.txt
rm -rf /home/pi/prev_installed_version.txt

# Offer reboot
printf "A reboot may be required before using the update.\n"
printf "Do you want to reboot now? (y/n)\n"
read -n 1
printf "\n"
if [[ "$REPLY" = "y" || "$REPLY" = "Y" ]]; then
  printf "\nRebooting\n"
  sudo reboot now
fi
exit
