#!/bin/bash

reset

printf "Micro-SD Card Information\n"
printf "=========================\n\n"

printf "CID\n\n"

cat /sys/block/mmcblk0/device/cid

printf "\nManufacturer ID\n\n"

cat /sys/block/mmcblk0/device/manfid

printf "\nOEM ID\n\n"

cat /sys/block/mmcblk0/device/oemid

printf "\nDate of Manufacture\n\n"

cat /sys/block/mmcblk0/device/date

printf "\nProduct Name\n\n"

cat /sys/block/mmcblk0/device/name

printf "\nProduct Serial Number\n\n"

cat /sys/block/mmcblk0/device/serial


printf "\n\nPress any key to return to the main menu\n"
read -n 1
exit
