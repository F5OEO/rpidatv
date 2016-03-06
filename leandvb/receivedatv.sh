sudo killall leandvb
sudo killall omxplayer
mkfifo fifots
sudo rtl_sdr -g 20 -f 437500000 -s 1024000 - | ./leandvb --sr 249980 --anf 1 -f 1024000 |buffer > fifots &
omxplayer fifots &
