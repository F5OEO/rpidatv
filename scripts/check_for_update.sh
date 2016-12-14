#!/bin/bash

## Download the latest_version file and convert it to a variable
cd /home/pi/rpidatv
rm /home/pi/rpidatv/latest_version.txt  >/dev/null 2>/dev/null
wget -q https://raw.githubusercontent.com/davecrump/rpidatv/master/scripts/latest_version.txt
LATESTVERSION=$(head -c 9 latest_version.txt)

## Check installed version
INSTALLEDVERSION=$(head -c 9 scripts/installed_version.txt)

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
        printf "\nUgrading now...\n"
        cd /home/pi
        rm update.sh >/dev/null 2>/dev/null
        wget -q https://raw.githubusercontent.com/davecrump/rpidatv/master/update.sh
        chmod +x update.sh
        /home/pi/update.sh &
        printf "Starting Update....\n"
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

