#!/bin/bash

## Download the latest_version file
cd /home/pi/rpidatv/scripts
rm /home/pi/rpidatv/scripts/latest_version.txt  >/dev/null 2>/dev/null
wget --timeout=2 https://raw.githubusercontent.com/BritishAmateurTelevisionClub/rpidatv/master/scripts/latest_version.txt

## Create the file if it doesn't exist
if  [ ! -f "latest_version.txt" ]; then
    echo '000000000' > latest_version.txt
fi

## Check the file has a valid format (first 2 characters are 20)
CHECK=$(head -c 2 latest_version.txt)
if [ $CHECK -ne "20" ];  ## If not a valid version number
then                     ## then check the internet connection
  ping 8.8.8.8 -c1 >/dev/null 2>/dev/null
  if [ $? -eq "0" ];     ## If ping to Google successful
  then
    printf "Unable to connect to GitHub to check the latest version.\n\n"
    printf "There is a working internet connection,\n"
    printf "but GitHub is not responding or being blocked.\n\n"
    printf "Try connecting to: \n\nhttps://raw.githubusercontent.com/BritishAmateurTelevisionClub/rpidatv/master/scripts/latest_version.txt\n\n"
    printf "in a web browser on another computer on the same network to diagnose the fault.\n"
  else                   ## If ping to Google unsuccesful
    printf "Unable to connect to the internet\n"
    printf "Please check your internet connection and then try again\n"
  fi
  printf "\nPress any key to return to the main menu\n"
  read -n 1
  exit
fi

## Format OK, so check against installed version
LATESTVERSION=$(head -c 9 latest_version.txt)
rm /home/pi/rpidatv/scripts/latest_version.txt  >/dev/null 2>/dev/null

## Check installed version
INSTALLEDVERSION=$(head -c 9 installed_version.txt)
cd /home/pi

## Compare versions
if [ $LATESTVERSION -eq $INSTALLEDVERSION ];    ## No need for upgrade
then
    printf "The installed version "$INSTALLEDVERSION" is the latest available\n\n"
    printf "Do you want to force an upgrade now? (y/n)\n"
    read -n 1
    printf "\n"
    if [[ "$REPLY" = "y" || "$REPLY" = "Y" ]];  ## Force upgrade requested
    then
        printf "\nUpgrading now...\n"
        cd /home/pi
        rm update.sh >/dev/null 2>/dev/null
        wget https://raw.githubusercontent.com/BritishAmateurTelevisionClub/rpidatv/master/update.sh
        chmod +x update.sh
        source /home/pi/update.sh
        exit
    else                                        ## Force upgrade not required
        printf "Not upgrading\n"
        sleep 2
    fi
    exit
fi

if [ $LATESTVERSION -gt $INSTALLEDVERSION ];    ## Upgrade available
then
    printf "The installed version is "$INSTALLEDVERSION".\n"
    printf "The latest version is    "$LATESTVERSION" do you want to upgrade now? (y/n)\n"
    read -n 1
    printf "\n"
    if [[ "$REPLY" = "y" || "$REPLY" = "Y" ]];  ## Upgrade requested
    then
        printf "\nUpgrading now...\n"
        cd /home/pi
        rm update.sh >/dev/null 2>/dev/null
        wget https://raw.githubusercontent.com/BritishAmateurTelevisionClub/rpidatv/master/update.sh
        chmod +x update.sh
        source /home/pi/update.sh 
        exit
    else                                        ##  Upgrade available, but rejected
        printf "Not upgrading\n"
        printf "The installed version is "$INSTALLEDVERSION".\n"
        printf "The latest version is    "$LATESTVERSION".\n"
    fi
else                                            ## Version Error 
    printf "There has been an error, or the installed version is newer than the published version\n"
    printf "The installed version is "$INSTALLEDVERSION".\n"
    printf "The latest version is    "$LATESTVERSION".\n"
fi

