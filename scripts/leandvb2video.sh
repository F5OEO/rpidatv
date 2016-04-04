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


if [ "$SYMBOLRATEK" -lt 250 ]; then
SR_RTLSDR=250000
else
SR_RTLSDR=1024000
fi

sudo killall leandvb
sudo killall mplayer
mkfifo fifots

#sudo rtl_sdr -p 82 -g 30 -f $FreqHz -s $SR_RTLSDR - 2>/dev/null | leandvb_gui --agc --gui --sr $SYMBOLRATE -f $SR_RTLSDR |buffer > fifots &
sudo rtl_sdr -p 82 -g 30 -f $FreqHz -s $SR_RTLSDR - 2>/dev/null | leandvb --agc --sr $SYMBOLRATE -f $SR_RTLSDR 2>/dev/null |buffer > fifots &
sudo SDL_VIDEODRIVER=fbcon SDL_FBDEV=/dev/fb0 mplayer -really-quiet -ao /dev/null -vo sdl fifots   &
