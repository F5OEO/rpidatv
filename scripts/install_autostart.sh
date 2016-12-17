#FOR Jessie
# Whiptail and /etc/rc.local is not running properly !!!!
# Use systemd method
#sudo mkdir -pv /etc/systemd/system/getty@tty1.service.d/
#sudo cp ./autologin.conf /etc/systemd/system/getty@tty1.service.d/
#sudo systemctl enable getty@tty1.service

ln -fs /etc/systemd/system/autologin@.service \
 /etc/systemd/system/getty.target.wants/getty@tty1.service

cp install_bashrc /home/pi/.bashrc
