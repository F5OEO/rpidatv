#!/bin/sh

echo "Installing transmitter..."

sudo apt-get update
sudo apt-get install rpi-update
sudo rpi-update

cd src
make -j 4
cd ../scripts
./process-templates
rm *.template
chmod +x *.sh
cd ..

echo "Done."
