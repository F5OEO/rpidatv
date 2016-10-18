#!/bin/bash

set -e
sudo dpkg --configure -a
sudo apt-get clean
sudo apt-get update
sudo apt-get -y install apt-transport-https git rpi-update
sudo apt-get -y install cmake libusb-1.0-0-dev g++ libx11-dev buffer libjpeg-dev indent libfreetype6-dev ttf-dejavu-core bc usbmount fftw3-dev wiringpi libvncserver-dev

#rpi-update to get latest firmware
sudo rpi-update

# ---------- install rpidatv -----------

cd /home/pi
#git clone git://github.com/F5OEO/rpidatv -> BUG IN QEMU : Go to download method
wget https://github.com/F5OEO/rpidatv/archive/master.zip
unzip master.zip -o
mv rpidatv-master rpidatv
rm master.zip

#rpidatv core
cd rpidatv/src
make
sudo make install
#rpidatv gui
cd gui
make
sudo make install
cd ../
#avc2ts
cd avc2ts

#git clone git://github.com/kierank/libmpegts
wget https://github.com/kierank/libmpegts/archive/master.zip
unzip master.zip
mv libmpegts-master libmpegts
rm master.zip

cd libmpegts
./configure
make
#make avc2ts
cd ../
make
sudo make install



#install rtl_sdr
cd /home/pi
#git clone https://github.com/keenerd/rtl-sdr
wget https://github.com/keenerd/rtl-sdr/archive/master.zip
unzip master.zip
mv rtl-sdr-master rtl-sdr
rm master.zip

cd rtl-sdr/ && mkdir build && cd build
cmake ../ -DINSTALL_UDEV_RULES=ON
make && sudo make install && sudo ldconfig
sudo bash -c 'echo -e "\n# for RTL-SDR:\nblacklist dvb_usb_rtl28xxu\n" >> /etc/modprobe.d/blacklist.conf'
cd ../../

#install leandvb
cd /home/pi/rpidatv/src
#git clone git://github.com/pabr/leansdr
wget https://github.com/pabr/leansdr/archive/master.zip
unzip master.zip
mv leansdr-master leansdr
rm master.zip

cd leansdr/src/apps
make
cp leandvb ../../../../bin/


#install tstools
cd /home/pi/rpidatv/src
wget https://github.com/F5OEO/tstools/archive/master.zip
unzip master.zip
mv tstools-master tstools
rm master.zip

cd tstools
make
cp bin/ts2es ../../bin/

#install H264 Decoder : hello_video
#compile ilcomponet first
cd /opt/vc/src/hello_pi/
./rebuild.sh

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
#git clone https://github.com/tasanakorn/rpi-fbcp
cd rpi-fbcp/
mkdir build
cd build/
cmake ..
make
sudo install fbcp /usr/local/bin/fbcp
cd ../../

#Install Waveshare DTOVERLAY
cd /home/pi/rpidatv/scripts/
sudo cp ./waveshare35a.dtbo /boot/overlays/


#Fallback IP to 192.168.1.60
sudo bash -c 'echo -e "\nprofile static_eth0\nstatic ip_address=192.168.1.60/24\nstatic routers=192.168.1.1\nstatic domain_name_servers=192.168.1.1\ninterface eth0\nfallback static_eth0" >> /etc/dhcpcd.conf'

#enable camera
sudo bash -c 'echo -e "\ngpu_mem=128\nstart_x=1\n" >> /boot/config.txt'

#disable sync option for usbmount
sudo sed -i 's/sync,//g' /etc/usbmount/usbmount.conf

ForImage='Image'
if [$1='Image'];
then
##Menu autostart
cd /home/pi/rpidatv/scripts/
##make kayboard in french
sudo cp keyfr /etc/default/keyboard
##do Menu as auto install
bash install_autostart.sh

#change hostname
CURRENT_HOSTNAME=`sudo cat /etc/hostname | sudo tr -d " \t\n\r"`
NEW_HOSTNAME="rpidatv"
if [ $? -eq 0 ]; then
  sudo sh -c "echo '$NEW_HOSTNAME' > /etc/hostname"
  sudo sed -i "s/127.0.1.1.*$CURRENT_HOSTNAME/127.0.1.1\t$NEW_HOSTNAME/g" /etc/hosts
fi
#change password to tv
echo "pi:tv" | sudo chpasswd
fi

#always enable HDMI at 720p
#sudo bash -c 'echo -e "\nhdmi_force_hotplug=1\nhdmi_drive=2\nhdmi_group=1\nhdmi_mode=4\n" >> /boot/config.txt'


#remove script that starts raspi config on first boot
#sudo rm -rf /etc/profile.d/raspi-config.sh
