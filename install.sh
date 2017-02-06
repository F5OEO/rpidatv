#!/bin/bash

# Updated by davecrump on 20170202

# Update the package manager, then install the packages we need
sudo dpkg --configure -a
sudo apt-get clean
sudo apt-get update
sudo apt-get -y install apt-transport-https git rpi-update
sudo apt-get -y install cmake libusb-1.0-0-dev g++ libx11-dev buffer libjpeg-dev indent libfreetype6-dev ttf-dejavu-core bc usbmount fftw3-dev wiringpi libvncserver-dev
sudo apt-get -y install fbi netcat

# rpi-update to get latest firmware
sudo rpi-update
cd /home/pi

# Check which source to download.  Default is production
# option d is development from davecrump
# option s is staging from batc/staging
if [ "$1" == "-d" ]; then
  echo "Installing development load"
  wget https://github.com/davecrump/rpidatv/archive/master.zip
elif [ "$1" == "-s" ]; then
  echo "Installing BATC Staging load"
  wget https://github.com/BritishAmateurTelevisionClub/rpidatv/archive/batc_staging.zip -O master.zip
else
  echo "Installing BATC Production load"
  wget https://github.com/BritishAmateurTelevisionClub/rpidatv/archive/master.zip
fi

# Unzip the source software and copy to the Pi
unzip -o master.zip
if [ "$1" == "-s" ]; then
  mv rpidatv-batc_staging rpidatv
else
  mv rpidatv-master rpidatv
fi
rm master.zip

# Compile rpidatv core
cd rpidatv/src
make
sudo make install

# Compile rpidatv gui
cd gui
make
sudo make install
cd ../

# Get libmpegts and compile
cd avc2ts
wget https://github.com/kierank/libmpegts/archive/master.zip
unzip master.zip
mv libmpegts-master libmpegts
rm master.zip
cd libmpegts
./configure
make

# Compile avc2ts
cd ../
make
sudo make install

# Compile adf4351
cd /home/pi/rpidatv/src/adf4351
touch adf4351.c
make
cp adf4351 ../../bin/

# Get rtl_sdr
cd /home/pi
wget https://github.com/keenerd/rtl-sdr/archive/master.zip
unzip master.zip
mv rtl-sdr-master rtl-sdr
rm master.zip

# Compile and install rtl-sdr
cd rtl-sdr/ && mkdir build && cd build
cmake ../ -DINSTALL_UDEV_RULES=ON
make && sudo make install && sudo ldconfig
sudo bash -c 'echo -e "\n# for RTL-SDR:\nblacklist dvb_usb_rtl28xxu\n" >> /etc/modprobe.d/blacklist.conf'
cd ../../

# Get leandvb
cd /home/pi/rpidatv/src
wget https://github.com/pabr/leansdr/archive/master.zip
unzip master.zip
mv leansdr-master leansdr
rm master.zip

# Compile leandvb
cd leansdr/src/apps
make
cp leandvb ../../../../bin/

# Get tstools
cd /home/pi/rpidatv/src
wget https://github.com/F5OEO/tstools/archive/master.zip
unzip master.zip
mv tstools-master tstools
rm master.zip

# Compile tstools
cd tstools
make
cp bin/ts2es ../../bin/

#install H264 Decoder : hello_video
#compile ilcomponet first
cd /opt/vc/src/hello_pi/
sudo ./rebuild.sh

cd /home/pi/rpidatv/src/hello_video
make
cp hello_video.bin ../../bin/

# TouchScreen GUI
# FBCP : Duplicate Framebuffer 0 -> 1
cd /home/pi/
wget https://github.com/tasanakorn/rpi-fbcp/archive/master.zip
unzip master.zip
mv rpi-fbcp-master rpi-fbcp
rm master.zip

# Compile fbcp
cd rpi-fbcp/
mkdir build
cd build/
cmake ..
make
sudo install fbcp /usr/local/bin/fbcp
cd ../../

# Install Waveshare DTOVERLAY
cd /home/pi/rpidatv/scripts/
sudo cp ./waveshare35a.dtbo /boot/overlays/

# Install the Waveshare driver

sudo bash -c 'cat /home/pi/rpidatv/scripts/configs/waveshare_mkr.txt >> /boot/config.txt'

# Disable the Touchscreen Screensaver

cd /boot
sudo sed -i -e 's/rootwait/rootwait consoleblank=0/' cmdline.txt
cd /etc/kbd
sudo sed -i 's/^BLANK_TIME.*/BLANK_TIME=0/' config
sudo sed -i 's/^POWERDOWN_TIME.*/POWERDOWN_TIME=0/' config

# Download, amend, compile and install DATV Express-server

cd /home/pi
wget https://github.com/G4GUO/express_server/archive/master.zip
unzip master.zip
mv express_server-master express_server
rm master.zip
cd /home/pi/express_server
sed -i 's/^     express_handle_events( 32 ).*/     express_handle_events( 1 );/' express.cpp
sed -i 's/^set ptt tx.*/set ptt rx/' datvexpress.txt
make
sudo make install

cd /home/pi/rpidatv/scripts/

# Fallback IP to 192.168.1.60 Disabled 201701230
#sudo bash -c 'echo -e "\nprofile static_eth0\nstatic ip_address=192.168.1.60/24\nstatic routers=192.168.1.1\nstatic domain_name_servers=192.168.1.1\ninterface eth0\nfallback static_eth0" >> /etc/dhcpcd.conf'

# Enable camera
sudo bash -c 'echo -e "\n##Enable Pi Camera" >> /boot/config.txt'
sudo bash -c 'echo -e "\ngpu_mem=128\nstart_x=1\n" >> /boot/config.txt'

# Disable sync option for usbmount
sudo sed -i 's/sync,//g' /etc/usbmount/usbmount.conf

# Install executable for hardware shutdown button
wget 'https://github.com/philcrump/pi-sdn/releases/download/v1.1/pi-sdn' -O /home/pi/pi-sdn
chmod +x /home/pi/pi-sdn

# Create directory for Autologin link
sudo mkdir -p /etc/systemd/system/getty.target.wants

# Show console menu at first user log-in
cp /home/pi/rpidatv/scripts/configs/console.bashrc /home/pi/.bashrc

# Record Version Number
cd /home/pi/rpidatv/scripts/
cp latest_version.txt installed_version.txt
cd /home/pi

# Switch to French if required
if [ "$1" == "fr" ];
then
  echo "Installing French Language and Keyboard"
  cd /home/pi/rpidatv/scripts/
  sudo cp configs/keyfr /etc/default/keyboard
  cp configs/rpidatvconfig.fr rpidatvconfig.txt
  cd /home/pi
  echo "Completed French Install"
else
  echo "Completed English Install"
fi

# Offer reboot
printf "A reboot will be required before using the software."
printf "Do you want to reboot now? (y/n)\n"
read -n 1
printf "\n"
if [[ "$REPLY" = "y" || "$REPLY" = "Y" ]]; then
  echo "rebooting"
  sudo reboot now
fi
exit


