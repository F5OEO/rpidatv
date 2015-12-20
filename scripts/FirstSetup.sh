


disable_InitSetup() {
if [ -e /etc/profile.d/FirstSetup.sh ]; then
sudo rm -f /etc/profile.d/FirstSetup.sh
fi
}

do_english()
{
	sudo cp /home/pi/RpiDATV/keygb /etc/default/keyboard
	sudo raspi-config --expand-rootfs
	disable_InitSetup
	sudo cp /home/pi/RpiDATV/install_inittab /etc/inittab
	cp /home/pi/RpiDATV/install_bashrc.gb /home/pi/.bashrc
	mv /home/pi/RpiDATV/rpibutton.sh /home/pi/RpiDATV/rpidisablebutton.sh
	sync
	sudo reboot
}

do_french()
{
	sudo cp /home/pi/RpiDATV/keyfr /etc/default/keyboard
	sudo raspi-config --expand-rootfs
	disable_InitSetup
	sudo cp /home/pi/RpiDATV/install_inittab /etc/inittab
	cp /home/pi/RpiDATV/install_bashrc.fr /home/pi/.bashrc
	sync
	sudo reboot
}

status="0"

 while [ "$status" -eq 0 ] 
    do

	menuchoice=$(whiptail --title "Setup / Installation" --menu "Welcome on RpiDATV / Bienvenue sur RpiDATV" 16 82 5 \
        "1 English" "English setup" \
	"2 Francais" "Installation en francais" \
	3>&2 2>&1 1>&3)
      
        case "$menuchoice" in
            1\ *) do_english   ;;
	    2\ *) do_french ;;
            *)		
	    status=1	
	;;	 
        esac
   	status=1
    done






#sudo nano /etc/default/keyboard : FR ou GB

