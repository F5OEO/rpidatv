#! /bin/bash


PATHRPI="/home/pi/rpidatv/bin"
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

# ########################## SURE TO KILL ALL PROCESS ################
sudo killall -9 ffmpeg >/dev/null 2>/dev/null
sudo killall rpidatv >/dev/null 2>/dev/null
sudo killall hello_encode.bin >/dev/null 2>/dev/null
sudo killall h264yuv >/dev/null 2>/dev/null
sudo killall avc2ts >/dev/null 2>/dev/null
sudo killall express_server >/dev/null 2>/dev/null
sudo killall tcanim >/dev/null 2>/dev/null
#---- Launch FBCP ----
#sudo killall fbcp 
#fbcp &
# ---------------
#sudo killall uv4l

detect_audio()
{
devicea="/proc/asound/card1"
if [ -e "$devicea" ]; then
	AUDIO_CARD=1
else	
	AUDIO_CARD=0
fi

if [ "$AUDIO_CARD" == 1 ]; then
	echo Audio Card present
else
	echo Audio Card Absent
fi
}

MODE_INPUT=$(get_config_var modeinput $CONFIGFILE)
TSVIDEOFILE=$(get_config_var tsvideofile $CONFIGFILE)
PATERNFILE=$(get_config_var paternfile $CONFIGFILE)
UDPINADDR=$(get_config_var udpinaddr $CONFIGFILE)
UDPOUTADDR=$(get_config_var udpoutaddr $CONFIGFILE)
CALL=$(get_config_var call $CONFIGFILE)
CHANNEL=$CALL"-rpidatv"
FREQ_OUTPUT=$(get_config_var freqoutput $CONFIGFILE)
BATC_OUTPUT=$(get_config_var batcoutput $CONFIGFILE)
OUTPUT_BATC="-f flv rtmp://fms.batc.tv/live/"$BATC_OUTPUT"/"$BATC_OUTPUT
MODE_OUTPUT=$(get_config_var modeoutput $CONFIGFILE)
SYMBOLRATEK=$(get_config_var symbolrate $CONFIGFILE)
GAIN=$(get_config_var rfpower $CONFIGFILE)
PIDVIDEO=$(get_config_var pidvideo $CONFIGFILE)
PIDPMT=$(get_config_var pidpmt $CONFIGFILE)
SERVICEID=$(get_config_var serviceid $CONFIGFILE)
LOCATOR=$(get_config_var locator $CONFIGFILE)
PIN_I=$(get_config_var gpio_i $CONFIGFILE)
PIN_Q=$(get_config_var gpio_q $CONFIGFILE)

ANALOGCAMNAME=$(get_config_var analogcamname $CONFIGFILE)
VNCADDR=$(get_config_var vncaddr $CONFIGFILE)

#v4l2-ctl --overlay=0



detect_audio

let SYMBOLRATE=SYMBOLRATEK*1000
FEC=$(get_config_var fec $CONFIGFILE)
let FECNUM=FEC
let FECDEN=FEC+1

OUTPUT_IP=""

case "$MODE_OUTPUT" in
	IP) 
	FREQUENCY_OUT=0
	OUTPUT_IP="-n"$UDPOUTADDR":10000"
	#GAIN=0
	;;
	IQ) 
	FREQUENCY_OUT=0
	OUTPUT=videots
	MODE=IQ
	#GAIN=0
	;;
	QPSKRF)
	FREQUENCY_OUT=$FREQ_OUTPUT
	OUTPUT=videots
	MODE=RF
	;;
	BATC)
	#MODE_INPUT=BATC
	OUTPUT=$OUTPUT_BATC
	;;
	DIGITHIN) 
	FREQUENCY_OUT=0
	OUTPUT=videots
	DIGITHIN_MODE=1
	MODE=DIGITHIN
	#GAIN=0
	;;
	DTX1) 
	MODE=PARALLEL
	FREQUENCY_OUT=2
	OUTPUT=videots
	DIGITHIN_MODE=0
	#GAIN=0
	;;
	DATVEXPRESS)
	sudo nice -n -30 ./express_server & 
	FREQUENCY_OUT=$FREQ_OUTPUT
	let FREQ_OUTPUTHZ=FREQ_OUTPUT*1000000
	OUTPUT="udp://127.0.0.1:1314?pkt_size=1316&buffer_size=1316"
	echo "set freq "$FREQ_OUTPUTHZ >> /tmp/expctrl
	echo "set fec "$FECNUM"/"$FECDEN >> /tmp/expctrl
	echo "set srate "$SYMBOLRATE >> /tmp/expctrl
	echo "set level "$GAIN >> /tmp/expctrl
	;;


esac


#CALL="F5OEO"
#CHANNEL="rpidatv"

#SYMBOLRATE=100000
#FECNUM=7
#FECDEN=8
VIDEO_FPS=15


#0=Mode IQ, else QPSK directly modulated
# TODO .......... if(mode
#FREQUENCY_OUT=0

# MODEVIDEO : FILETS,PATERN,PATERNAUDIO,CAMH264,CAMH264AUDIO,CAMMPEG2,IPTS,TESTMODE,CARRIER,BATC



#TSVIDEOFILE=/home/pi/UglyDATVRelease/mire250.TS
#PATERNFILE=/home/pi/mire.jpg

#to debug with IP

#OUTPUT_IP="-n 230.0.0.1:10000"
OUTPUT_QPSK="videots"


# ************************ OUTPUT MODE DEFINITION ******************
#OUTPUT=$OUTPUT_IP
#OUTPUT=$OUTPUT_QPSK
MODE_DEBUG=quiet
#MODE_DEBUG=debug

#BITRATE AVEC 5%
# BITRATE TS THEORIC
let BITRATE_TS=SYMBOLRATE*2*188*FECNUM/204/FECDEN 
#let BITRATE_TS=SYMBOLRATE*2*188*FECNUM/204/FECDEN+1000


#let BITRATE_VIDEO=(BITRATE_TS*7)/10-72000 audio
let BITRATE_VIDEO=(BITRATE_TS*5)/10-10000

let DELAY=(BITRATE_VIDEO*8)/10
let SYMBOLRATE_K=SYMBOLRATE/1000


if [ "$BITRATE_VIDEO" -lt 150000 ]; then
VIDEO_WIDTH=160
VIDEO_HEIGHT=140
else
	if [ "$BITRATE_VIDEO" -lt 300000 ]; then
	VIDEO_WIDTH=352
	VIDEO_HEIGHT=288
	else
		VIDEO_WIDTH=720
		VIDEO_HEIGHT=576
	fi	
fi

if [ "$BITRATE_VIDEO" -lt 300000 ]; then
VIDEO_FPS=15
else
VIDEO_FPS=25
fi


sudo rm videoes
sudo rm videots
sudo rm netfifo
mkfifo videoes
mkfifo videots
mkfifo netfifo 



echo "************************************"
echo Bitrate TS $BITRATE_TS
echo Bitrate Video $BITRATE_VIDEO
echo Size $VIDEO_WIDTH x $VIDEO_HEIGHT at $VIDEO_FPS fps
echo "************************************"
echo "ModeINPUT="$MODE_INPUT


OUTPUT_FILE="-o videots"

case "$MODE_INPUT" in
#============================================ H264 INPUT MODE =========================================================
"CAMH264")
	sudo modprobe -r bcm2835_v4l2
 	#$PATHRPI"/mnc" -l -i loopback -p 10000 230.0.0.1 > videots &
	case "$MODE_OUTPUT" in
	"BATC")
		sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i videots -y $OUTPUT_BATC & ;;
	"IP")
		OUTPUT_FILE="" ;;
	*)
		sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q & ;;
	esac

	if [ "$AUDIO_CARD" == 0 ]; then
	# ******************************* H264 VIDEO ONLY ************************************
	$PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT -f $VIDEO_FPS -i 100 -p $PIDPMT -s $CHANNEL $OUTPUT_FILE $OUTPUT_IP &
	
	else
	# ******************************* H264 VIDEO WITH AUDIO (TODO) ************************************
	$PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT -f $VIDEO_FPS -i 100 -p $PIDPMT -s $CHANNEL $OUTPUT_FILE $OUTPUT_IP &
	
	fi
	;;


#============================================ MPEG-2 INPUT MODE =============================================================
"CAMMPEG-2")
VIDEO_WIDTH=352
VIDEO_HEIGHT=288
VIDEO_FPS=25
sudo modprobe bcm2835-v4l2
v4l2-ctl --set-fmt-video=width=$VIDEO_WIDTH,height=$VIDEO_HEIGHT,pixelformat=0
v4l2-ctl -p $VIDEO_FPS
let DELAY=(BITRATE_VIDEO*8)/10

sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &

if [ "$AUDIO_CARD" == 0 ]; then
# ******************************* MPEG-2 VIDEO WITH BEEP ************************************
sudo $PATHRPI"/ffmpeg"  -loglevel $MODE_DEBUG -itsoffset -00:00:0.2 -analyzeduration 0 -probesize 2048  -fpsprobesize 0 -re -ac 1 -f lavfi -thread_queue_size 512 -i "sine=frequency=500:beep_factor=4:sample_rate=48000:duration=3600" -f v4l2 -framerate $VIDEO_FPS -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -i /dev/video0 -fflags nobuffer -vcodec mpeg2video -s "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -f mpegts  -blocksize 1880 -strict experimental  -acodec mp2 -ab 64K -ar 48k -ac 1  -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -muxrate $BITRATE_TS -y $OUTPUT &
else
# ******************************* MPEG-2 VIDEO WITH AUDIO ************************************
sudo nice -n -30 arecord -f S16_LE -r 48000 -c 1 -M -D hw:1 |sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -itsoffset -00:00:0.8 -analyzeduration 0 -probesize 2048  -fpsprobesize 0 -ac 1 -thread_queue_size 512 -i -  -f v4l2 -framerate $VIDEO_FPS -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -i /dev/video0 -fflags nobuffer -vcodec mpeg2video -s "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -f mpegts  -blocksize 1880 -strict experimental  -acodec mp2 -ab 64K -ar 48k -ac 1 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -muxrate $BITRATE_TS -y $OUTPUT &
fi
;;


#============================================ H264 PATERN =============================================================


"PATERNAUDIO")
sudo modprobe -r bcm2835_v4l2

case "$MODE_OUTPUT" in
	"BATC")
		sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i videots -y $OUTPUT_BATC & ;;
	"IP")
		OUTPUT_FILE="" ;;
	*)
		sudo  nice -n -30 $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &;;
	esac


$PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT -f $VIDEO_FPS -d 400 -i 100 $OUTPUT_FILE -t 3 -p $PIDPMT -s $CHANNEL $OUTPUT_IP  &

$PATHRPI"/tcanim" $PATERNFILE"/*10" "48" "72" "CQ" "CQ CQ CQ DE "$CALL" IN $LOCATOR - DATV $SYMBOLRATEK KS FEC "$FECNUM"/"$FECDEN &

;;

#============================================ VNC =============================================================


"VNC")
sudo modprobe -r bcm2835_v4l2
case "$MODE_OUTPUT" in
	"BATC")
		sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i videots -y $OUTPUT_BATC & ;;
	"IP")
		OUTPUT_FILE="" ;;
	*)
		sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &;;
	esac


$PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT -f $VIDEO_FPS -i 100 $OUTPUT_FILE -t 4 -e $VNCADDR -p $PIDPMT -s $CHANNEL $OUTPUT_IP &

;;

#============================================ ANALOG =============================================================


"ANALOGCAM")
sudo modprobe -r bcm2835_v4l2
case "$MODE_OUTPUT" in
	"BATC")
		sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i videots -y $OUTPUT_BATC & ;;
	"IP")
		OUTPUT_FILE="" ;;
	*)
		sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &;;
	esac

$PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT -f $VIDEO_FPS -i 100 $OUTPUT_FILE -t 2 -e $ANALOGCAMNAME -p $PIDPMT -s $CHANNEL $OUTPUT_IP  &

;;

"DESKTOP")
sudo modprobe -r bcm2835_v4l2
case "$MODE_OUTPUT" in
	"BATC")
		sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i videots -y $OUTPUT_BATC & ;;
	"IP")
		OUTPUT_FILE="" ;;
	*)
		sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &;;
	esac

$PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT -f $VIDEO_FPS -i 100 $OUTPUT_FILE -t 3 -p $PIDPMT -s $CHANNEL $OUTPUT_IP &

;;

# *********************************** TRANSPORT STREAM INPUT THROUGH IP ******************************************
"IPTSIN")
case "$MODE_OUTPUT" in
	"BATC")
		sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i videots -y $OUTPUT_BATC & ;;
	*)
		sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &;;
	esac

PORT=10000
$PATHRPI"/mnc" -l -i eth0 -p $PORT $UDPINADDR > videots &
;;

# *********************************** TRANSPORT STREAM INPUT FILE ******************************************
"FILETS")
	
case "$MODE_OUTPUT" in
	"BATC")
		sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i $TSVIDEOFILE -y $OUTPUT_BATC & ;;
	
	*)
		sudo $PATHRPI"/rpidatv" -i $TSVIDEOFILE -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &;;
	esac

;;

# *********************************** CARRIER  ******************************************
"CARRIER")
echo ====================== CARRIER ==========================

sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c "carrier" -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
;;

# *********************************** TESTMODE  ******************************************
"TESTMODE")

sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c "tesmode" -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
;;




esac



