#!/bin/bash

# Script used to set up WiFi from rpidatv menu

reset  # Clear the screen

# Check that wifi has not been disabled

if [ -f ~/.wifi_off ]; then
    printf "WiFi was disabled at start-up.\n"
    printf "You cannot set up the WiFi until it is re-enabled.\n"
    printf "Do you want to re-enable it and reboot imediately?\n"
    read -n 1
    printf "\n"
    if [ "$REPLY" = "Y" ]; then
	echo "rebooting"
        rm ~/.wifi_off
	sudo reboot now
    else
        if [ "$REPLY" = "y" ]; then
            echo "rebooting"
            rm ~/.wifi_off
            sudo reboot now
        else
            exit
        fi
    fi
fi

## Wifi is enabled so
## List the available networks

printf "The following networks are available:\n"
printf "\n"
sudo iwlist wlan0 scan | grep 'ESSID'
printf "\n"
printf "Type the SSID of the network that you want to connect to (without qoutes) and press enter\n"
printf "\n"

read SSID

printf "\n"
printf "Type the network password and press enter\n"
printf "Characters will not be displayed\n"
printf "\n"

stty -echo

read PW

stty echo
printf "\nWorking....\n\n"
stty -echo

PSK_TEXT=$(wpa_passphrase "$SSID" "$PW" | grep 'psk=' | grep -v '#psk')

PATHCONFIGS="/home/pi/rpidatv/scripts/configs"  ## Path to config files

## Build text for supplicant file

rm $PATHCONFIGS"/wpa_text.txt"

echo -e "network={" >> $PATHCONFIGS"/wpa_text.txt"
echo -e "    ssid="\"""$SSID"\"" >> $PATHCONFIGS"/wpa_text.txt"
echo -e "   "$PSK_TEXT >> $PATHCONFIGS"/wpa_text.txt"
echo -e "}" >>  $PATHCONFIGS"/wpa_text.txt"

## Copy the existing wpa_supplicant file to work on

sudo cp /etc/wpa_supplicant/wpa_supplicant.conf $PATHCONFIGS"/wpa_supcopy.txt"
sudo chown pi:pi $PATHCONFIGS"/wpa_supcopy.txt"

## Define the parameters for the replace script

lead='^##STARTNW'                         ## Marker for start of inserted text
tail='^##ENDNW'                           ## Marker for end of inserted text
CHANGEFILE=$PATHCONFIGS"/wpa_supcopy.txt" ## File requiring added text
APPENDFILE=$PATHCONFIGS"/wpa_markers.txt" ## File containing both markers
TRANSFILE=$PATHCONFIGS"/transfer.txt"     ## File used for transfer
INSERTFILE=$PATHCONFIGS"/wpa_text.txt"    ## File to be included

grep -q "$lead" "$CHANGEFILE"             ## Is the first marker already present?
if [ $? -ne 0 ]; then
    sudo bash -c 'cat '$APPENDFILE' >> '$CHANGEFILE' '  ## If not append the markers
fi

## Replace whatever is between the markers with the insert text

sed -e "/$lead/,/$tail/{ /$lead/{p; r $INSERTFILE
        }; /$tail/p; d }" $CHANGEFILE >> $TRANSFILE

sudo cp "$TRANSFILE" "$CHANGEFILE"          ## Copy from the transfer file
rm $TRANSFILE                               ## Delete the transfer file

## Give the file root ownership and copy it back over the original

sudo chown root:root $PATHCONFIGS"/wpa_supcopy.txt"
sudo cp $PATHCONFIGS"/wpa_supcopy.txt" /etc/wpa_supplicant/wpa_supplicant.conf
sudo rm $PATHCONFIGS"/wpa_supcopy.txt"

stty echo

##bring wifi down and up again

sudo ifdown wlan0
sudo ifup wlan0

printf "WiFi Configured\n"
sleep 1

exit
