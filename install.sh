#fbcp install for frame buffer copy on tft screen
git clone https://github.com/tasanakorn/rpi-fbcp
cd rpi-fbcp/
mkdir build
cd build/
cmake ..
make
sudo install fbcp /usr/local/bin/fbcp
cd ../../

# Rtlsdr install
sudo apt-get install cmake libusb-1.0-0-dev 
git clone https://github.com/keenerd/rtl-sdr
cd rtl-sdr/ && mkdir build && cd build
cmake ../ -DINSTALL_UDEV_RULES=ON
make && sudo make install && sudo ldconfig
sudo bash -c 'echo -e "\n# for RTL-SDR:\nblacklist dvb_usb_rtl28xxu\n" >> /etc/modprobe.d/blacklist.conf'
cd ../../

# LeanDVB install

sudo apt-get install g++ libx11-dev buffer
cd leandvb/
wget http://www.pabr.org/radio/leandvb/leandvb.cc
g++ -O3 -DGUI leandvb.cc -lX11 -o leandvb
cd ../


