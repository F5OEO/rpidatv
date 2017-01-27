#!/bin/bash

########## ctlfilter.sh ############

# Called by a.sh in IQ mode to switch in correct
# Nyquist Filter and band switching
# Written by Dave G8GKQ 26 Nov 16 and 4 Dec 16

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

### End ###

# Revert to menu.sh or a.sh #

