#! /bin/bash
PATHSCRIPT="/home/pi/rpidatv/scripts"
CONFIGFILE=$PATHSCRIPT"/rpidatvconfig.txt"

get_config_var() {
lua - "$1" "$2" <<EOF
local key=assert(arg[1])
local fn=assert(arg[2])
local file=assert(io.open(fn))
for line in file:lines() do
local val = line:match("^#?%s*"..key.."=(.*)$")
if (val ~= nil) then
print(val)
break
end
end
EOF
}

SYMBOLRATEK=$(get_config_var symbolrate $CONFIGFILE)
FREQ_OUTPUT=$(get_config_var freqoutput $CONFIGFILE)

let SYMBOLRATE=SYMBOLRATEK*1000
let FreqHz=FREQ_OUTPUT*1000000

sudo killall leandvb
sudo killall mplayer
mkfifo fifots
sudo rtl_sdr -p 82 -g 30 -f $FreqHz -s 1024000 - | leandvb --agc --sr $SYMBOLRATE --gui --anf 1 -f 1024000 |buffer > fifots  &
#sudo rtl_sdr -p 80 -g 20 -f 436989000 -s 1400000 - | leandvb --agc --sr 1200000 --gui --anf 1 -f 1400000 |buffer > F5AGO.ts  &
sudo SDL_VIDEODRIVER=fbcon SDL_FBDEV=/dev/fb0 mplayer -ao /dev/null -vo sdl  fifots &
