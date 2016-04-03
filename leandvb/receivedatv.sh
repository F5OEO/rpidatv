#!/bin/sh

# Basic settings
FREQ=437.5M
SYMBOLRATE=500000
SAMPLERATE=1024000
GAIN=20
LEANDVB_ARGS=""

sudo killall leandvb

if hash vlc 2>/dev/null; then
	sudo killall vlc
	PLAYER="vlc"
elif hash mplayer 2>/dev/null; then
	sudo killall mplayer
	PLAYER="mplayer"
elif hash omxplayer 2>/dev/null; then
	PLAYER="omxplayer"
	sudo killall omxplayer
fi

echo Using $PLAYER to play the stream
echo The frequency is $FREQ and the symbolrate is $SYMBOLRATE
echo The sampling rate is $SAMPLERATE and the RX gain is $GAIN
echo Starting leandvb with $LEANDVB_ARGS
echo If that's wrong, please feel free to edit the script.

mkfifo fifots
rtl_sdr -g $GAIN -f $FREQ -s $SAMPLERATE - | ./leandvb $LEANDVB_ARGS --sr $SYMBOLRATE --anf 1 -f $SAMPLERATE |buffer > fifots &
$PLAYER fifots &
