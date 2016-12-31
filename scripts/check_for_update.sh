#!/bin/bash

## Download the latest_version file and convert it to a variable
cd /home/pi/rpidatv/scripts
rm /home/pi/rpidatv/scripts/latest_version.txt  >/dev/null 2>/dev/null
wget -q https://raw.githubusercontent.com/BritishAmateurTelevisionClub/rpidatv/master/scripts/latest_version.txt
LATESTVERSION=$(head -c 9 latest_version.txt)
rm /home/pi/rpidatv/scripts/latest_version.txt  >/dev/null 2>/dev/null

## Check installed version
INSTALLEDVERSION=$(head -c 9 installed_version.txt)
cd /home/pi

## Compare versions
if [ $LATESTVERSION -eq $INSTALLEDVERSION ];
then
    printf "The installed version "$INSTALLEDVERSION" is the latest available\n"
    sleep 2
    exit
fi
if [ $LATESTVERSION -gt $INSTALLEDVERSION ];
then
    printf "The installed version is "$INSTALLEDVERSION".\n"
    printf "The latest version is    "$LATESTVERSION" do you want to upgrade now? (y/n)\n"
    read -n 1
    printf "\n"
    if [[ "$REPLY" = "y" || "$REPLY" = "Y" ]];
    then
        printf "\nUpgrading now...\n"
        cd /home/pi
        rm update.sh >/dev/null 2>/dev/null
        wget -q https://raw.githubusercontent.com/BritishAmateurTelevisionClub/rpidatv/master/update.sh
        chmod +x update.sh
        source /home/pi/update.sh 
        exit
    else
        printf "Not upgrading\n"
        printf "The installed version is "$INSTALLEDVERSION".\n"
        printf "The latest version is    "$LATESTVERSION".\n"
    fi
else
    printf "There has been an error, or the installed version is newer than the published version\n"
    printf "The installed version is "$INSTALLEDVERSION".\n"
    printf "The latest version is    "$LATESTVERSION".\n"
fi

