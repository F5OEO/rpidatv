#!/bin/bash

########## ctlfilter.sh ############

# Called by a.sh in IQ and DATVEXPRESS modes to switch in correct
# Nyquist Filter and band switching
# Written by Dave G8GKQ 20170209

# SR Outputs:

# <130   000
# <260   001
# <360   010
# <550   011
# <1100  100
# <2200  101
# >=2200 110

# Band Outputs:

# <100   00  (71 MHz)
# <250   01  (146.5 MHz)
# <950   10  (437 MHz)
# <4400  11  (1255 MHz)

# DATV Express Switching

# <100   0  (71 MHz)
# <250   1  (146.5 MHz)
# <950   2  (437 MHz)
# <20000 3  (1255 MHz)
# <4400  4  (2400 MHz


# Non integer frequencies are rounded down

############ Set Environment Variables ###############

PATHSCRIPT=/home/pi/rpidatv/scripts
CONFIGFILE=$PATHSCRIPT"/rpidatvconfig.txt"

############### PIN DEFINITIONS ###########

#filter_bit0 LSB of filter control word = BCM 16 / Header 36  
filter_bit0=16

#filter_bit1 Mid Bit of filter control word = BCM 26 / Header 37
filter_bit1=26

#filter_bit0 MSB of filter control word = BCM 20 / Header 38
filter_bit2=20

#band_bit_0 LSB of band switching word = BCM 1 / Header 28
band_bit0=1

#band_bit_1 MSB of band switching word = BCM 19 / Header 35
band_bit1=19

# Set all as outputs
gpio -g mode $filter_bit0 out
gpio -g mode $filter_bit1 out
gpio -g mode $filter_bit2 out
gpio -g mode $band_bit0 out
gpio -g mode $band_bit1 out

############### Function to read Config File ###############

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

############### Read Symbol Rate #########################

SYMBOLRATEK=$(get_config_var symbolrate $CONFIGFILE)

############### Switch GPIOs based on Symbol Rate ########

if (( $SYMBOLRATEK \< 130 )); then
                gpio -g write $filter_bit0 0;
                gpio -g write $filter_bit1 0;
                gpio -g write $filter_bit2 0;
elif (( $SYMBOLRATEK \< 260 )); then
                gpio -g write $filter_bit0 1;
                gpio -g write $filter_bit1 0;
                gpio -g write $filter_bit2 0;
elif (( $SYMBOLRATEK \< 360 )); then
                gpio -g write $filter_bit0 0;
                gpio -g write $filter_bit1 1;
                gpio -g write $filter_bit2 0;
elif (( $SYMBOLRATEK \< 550 )); then
                gpio -g write $filter_bit0 1;
                gpio -g write $filter_bit1 1;
                gpio -g write $filter_bit2 0;
elif (( $SYMBOLRATEK \< 1100 )); then
                gpio -g write $filter_bit0 0;
                gpio -g write $filter_bit1 0;
                gpio -g write $filter_bit2 1;
elif (( $SYMBOLRATEK \< 2200 )); then
                gpio -g write $filter_bit0 1;
                gpio -g write $filter_bit1 0;
                gpio -g write $filter_bit2 1;
else
                gpio -g write $filter_bit0 0;
                gpio -g write $filter_bit1 1;
                gpio -g write $filter_bit2 1;
fi

############### Read Frequency #########################

FREQ_OUTPUT=$(get_config_var freqoutput $CONFIGFILE)

INT_FREQ_OUTPUT=${FREQ_OUTPUT%.*}

############### Switch GPIOs based on Frequency ########

if (( $INT_FREQ_OUTPUT \< 100 )); then
                gpio -g write $band_bit0 0;
                gpio -g write $band_bit1 0;
elif (( $INT_FREQ_OUTPUT \< 250 )); then
                gpio -g write $band_bit0 1;
                gpio -g write $band_bit1 0;
elif (( $INT_FREQ_OUTPUT \< 950 )); then
                gpio -g write $band_bit0 0;
                gpio -g write $band_bit1 1;
elif (( $INT_FREQ_OUTPUT \< 4400 )); then
                gpio -g write $band_bit0 1;
                gpio -g write $band_bit1 1;
else
                gpio -g write $band_bit0 0;
                gpio -g write $band_bit1 0;
fi

################ If DATVEXPRESS in use, Set Ports ########

MODE_OUTPUT=$(get_config_var modeoutput $CONFIGFILE)
if [ $MODE_OUTPUT = "DATVEXPRESS" ]; then
  if (( $INT_FREQ_OUTPUT \< 100 )); then
    EXPPORTS0=$(get_config_var expports0 $CONFIGFILE)
    echo "set port "$EXPPORTS0 >> /tmp/expctrl
  elif (( $INT_FREQ_OUTPUT \< 250 )); then
    EXPPORTS1=$(get_config_var expports1 $CONFIGFILE)
    echo "set port "$EXPPORTS1 >> /tmp/expctrl
  elif (( $INT_FREQ_OUTPUT \< 950 )); then
    EXPPORTS2=$(get_config_var expports2 $CONFIGFILE)
    echo "set port "$EXPPORTS2 >> /tmp/expctrl
  elif (( $INT_FREQ_OUTPUT \< 2000 )); then
    EXPPORTS3=$(get_config_var expports3 $CONFIGFILE)
    echo "set port "$EXPPORTS3 >> /tmp/expctrl
  elif (( $INT_FREQ_OUTPUT \< 4400 )); then
    EXPPORTS4=$(get_config_var expports4 $CONFIGFILE)
    echo "set port "$EXPPORTS4 >> /tmp/expctrl
  else
    echo "set port 0" >> /tmp/expctrl
  fi
fi

### End ###

# Revert to menu.sh or a.sh #

