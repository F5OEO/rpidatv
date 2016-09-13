#! /bin/bash
PATHBIN="/home/pi/rpidatv/"
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
FEC=$(get_config_var fec $CONFIGFILE)
let FECNUM=FEC
let FECDEN=FEC+1

FreqHz=$(echo "($FREQ_OUTPUT*1000000)/1" | bc )
let SYMBOLRATE=SYMBOLRATEK*1000
#let FreqHz=FREQ_OUTPUT*1000000
echo Freq = $FreqHz

if [ "$SYMBOLRATEK" -lt 250 ]; then
SR_RTLSDR=250000
else
SR_RTLSDR=1024000
fi

sudo killall leandvb
mkfifo fifo.264

sudo rtl_sdr -p 82 -g 30 -f $FreqHz -s $SR_RTLSDR - 2>/dev/null | $PATHBIN"leandvb"  --cr $FECNUM"/"$FECDEN --sr $SYMBOLRATE -f $SR_RTLSDR 2>/dev/null |buffer| $PATHBIN"ts2es" -video -stdin fifo.264 &
$PATHBIN"hello_video.bin" fifo.264 &

