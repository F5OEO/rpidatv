#! /bin/bash
# set -x #Uncomment for testing

# Version 201702190

############# SET GLOBAL VARIABLES ####################

PATHRPI="/home/pi/rpidatv/bin"
PATHSCRIPT="/home/pi/rpidatv/scripts"
CONFIGFILE=$PATHSCRIPT"/rpidatvconfig.txt"

############# MAKE SURE THAT WE KNOW WHERE WE ARE ##################

cd /home/pi

############ FUNCTION TO READ CONFIG FILE #############################

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
sudo killall avc2ts >/dev/null 2>/dev/null
sudo killall rpidatv >/dev/null 2>/dev/null
sudo killall hello_encode.bin >/dev/null 2>/dev/null
sudo killall h264yuv >/dev/null 2>/dev/null

#sudo killall express_server >/dev/null 2>/dev/null
# Leave Express Server running
sudo killall tcanim >/dev/null 2>/dev/null
# Kill netcat that night have been started for Express Srver
sudo killall netcat >/dev/null 2>/dev/null
sudo killall -9 netcat >/dev/null 2>/dev/null

############ FUNCTION TO DETECT USB AUDIO DONGLE #############################

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

############ READ FROM rpidatvconfig.txt and Set PARAMETERS #######################

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

STREAM_URL=$(get_config_var streamurl $CONFIGFILE)
STREAM_KEY=$(get_config_var streamkey $CONFIGFILE)
OUTPUT_STREAM="-f flv "$STREAM_URL"/"$STREAM_KEY

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
ANALOGCAMINPUT=$(get_config_var analogcaminput $CONFIGFILE)
ANALOGCAMSTANDARD=$(get_config_var analogcamstandard $CONFIGFILE)
VNCADDR=$(get_config_var vncaddr $CONFIGFILE)

OUTPUT_IP=""

let SYMBOLRATE=SYMBOLRATEK*1000
FEC=$(get_config_var fec $CONFIGFILE)
let FECNUM=FEC
let FECDEN=FEC+1

#v4l2-ctl --overlay=0

detect_audio

######################### Pre-processing for each Output Mode ###############

case "$MODE_OUTPUT" in

  IQ)
    FREQUENCY_OUT=0
    OUTPUT=videots
    MODE=IQ
    $PATHSCRIPT"/ctlfilter.sh"
    $PATHSCRIPT"/ctlvco.sh"
    #GAIN=0
  ;;

  QPSKRF)
    FREQUENCY_OUT=$FREQ_OUTPUT
    OUTPUT=videots
    MODE=RF
  ;;

  BATC)
    # Set Output string "-f flv rtmp://fms.batc.tv/live/"$BATC_OUTPUT"/"$BATC_OUTPUT 
    OUTPUT=$OUTPUT_BATC
    # If CAMH264 is selected, temporarily select CAMMPEG-2
    if [ "$MODE_INPUT" == "CAMH264" ]; then
      MODE_INPUT="CAMMPEG-2"
    fi
    # Temporarily set optimum symbol rate for BATC Streamer
    SYMBOLRATEK="400"
    let SYMBOLRATE=SYMBOLRATEK*1000
  ;;

  STREAMER)
    # Set Output string "-f flv "$STREAM_URL"/"$STREAM_KEY
    OUTPUT=$OUTPUT_STREAM
    # If CAMH264 is selected, temporarily select CAMMPEG-2
    if [ "$MODE_INPUT" == "CAMH264" ]; then
      MODE_INPUT="CAMMPEG-2"
    # Temporarily set optimum symbol rate for another Streamer
    SYMBOLRATEK="400"
    let SYMBOLRATE=SYMBOLRATEK*1000
    fi
  ;;

  DIGITHIN)
    FREQUENCY_OUT=0
    OUTPUT=videots
    DIGITHIN_MODE=1
    MODE=DIGITHIN
    $PATHSCRIPT"/ctlfilter.sh"
    $PATHSCRIPT"/ctlvco.sh"
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
    if pgrep -x "express_server" > /dev/null
    then
      # Express already running
      echo > null
    else
      # Stopped, so make sure the control file is not locked and start it
      # From its own folder otherwise it doesnt read the config file
      sudo rm /tmp/expctrl >/dev/null 2>/dev/null
      cd /home/pi/express_server
      sudo nice -n -40 /home/pi/express_server/express_server  >/dev/null 2>/dev/null &
      cd /home/pi
      sleep 5
    fi
    # Set output for ffmpeg (avc2ts uses netcat to pipe output from videots)
    OUTPUT="udp://127.0.0.1:1314?pkt_size=1316&buffer_size=1316"
    FREQUENCY_OUT=0  # Not used in this mode?
    # Calculate output freq in Hz using floating point
    FREQ_OUTPUTHZ=`echo - | awk '{print '$FREQ_OUTPUT' * 1000000}'`
    echo "set freq "$FREQ_OUTPUTHZ >> /tmp/expctrl
    echo "set fec "$FECNUM"/"$FECDEN >> /tmp/expctrl
    echo "set srate "$SYMBOLRATE >> /tmp/expctrl
    # Set the ports
    $PATHSCRIPT"/ctlfilter.sh"

    # Set the output level based on the band
    INT_FREQ_OUTPUT=${FREQ_OUTPUT%.*}
    if (( $INT_FREQ_OUTPUT \< 100 )); then
      GAIN=$(get_config_var explevel0 $CONFIGFILE);
    elif (( $INT_FREQ_OUTPUT \< 250 )); then
      GAIN=$(get_config_var explevel1 $CONFIGFILE);
    elif (( $INT_FREQ_OUTPUT \< 950 )); then
      GAIN=$(get_config_var explevel2 $CONFIGFILE);
    elif (( $INT_FREQ_OUTPUT \< 2000 )); then
      GAIN=$(get_config_var explevel3 $CONFIGFILE);
    elif (( $INT_FREQ_OUTPUT \< 4400 )); then
      GAIN=$(get_config_var explevel4 $CONFIGFILE);
    else
      GAIN="30";
    fi

    # Set Gain
    echo "set level "$GAIN >> /tmp/expctrl

    # Make sure that carrier mode is off
    echo "set car off" >> /tmp/expctrl
  ;;

  IP)
    FREQUENCY_OUT=0
    OUTPUT_IP="-n"$UDPOUTADDR":10000"
    #GAIN=0
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
let BITRATE_VIDEO=(BITRATE_TS*75)/100-10000

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
    # Start Pi Camera
    sudo modprobe -r bcm2835_v4l2

    case "$MODE_OUTPUT" in
      "BATC")
        : # Do nothing
        # sudo nice -n -30 $PATHRPI"/ffmpeg" -i videots -y $OUTPUT_BATC &
      ;;
      "STREAMER")
        : # Do nothing
      ;;
      "IP")
        OUTPUT_FILE=""
      ;;
      "DATVEXPRESS")
        echo "set ptt tx" >> /tmp/expctrl
        sudo nice -n -30 netcat -u -4 127.0.0.1 1314 < videots & 
     ;;
      *)
        # For IQ, QPSKRF, DIGITHIN and DTX1
        sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
      ;;
    esac

    if [ "$AUDIO_CARD" == 0 ]; then
      # ******************************* H264 VIDEO, NO AUDIO ************************************
      $PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT -f $VIDEO_FPS -i 100 -d 300 -p $PIDPMT -s $CHANNEL $OUTPUT_FILE $OUTPUT_IP > /dev/null &
    else
      # ******************************* H264 VIDEO WITH AUDIO (TODO) ************************************
 let BITRATE_VIDEO=BITRATE_VIDEO-32000
      $PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT -f $VIDEO_FPS -i 100 -p $PIDPMT -s $CHANNEL $OUTPUT_FILE $OUTPUT_IP  > /dev/null &
rm /home/pi/rpidatv/scripts/output.aac	
mkfifo /home/pi/rpidatv/scripts/output.aac
amixer -c stk1160mixer sset Line unmute cap
sudo killall arecord
#For USB AUDIO
 arecord -f S16_LE -r 48000 -N -M  -c 1 -D hw:1  |  fdkaac --raw --raw-channels 1 --raw-rate 48000 -p5 -b 24000 -f2  - -o $PATHSCRIPT"/output.aac" &	
# arecord -f S16_LE -r 48000  -c 2 -D hw:1  |buffer|  fdkaac --raw --raw-channels 2 --raw-rate 48000 -p5 -b32 -f2  - -o /home/pi/rpidatv/scripts/output.aac &
    fi
  ;;

  #============================================ MPEG-2 INPUT MODE =============================================================
  "CAMMPEG-2")
    # Start the Camera
    #VIDEO_WIDTH=352
    #VIDEO_HEIGHT=288
    #VIDEO_FPS=25
#ffmpeg is usually undeflow, takes margin on TS output
let BITRATE_TS=SYMBOLRATE*2*188*FECNUM/204/FECDEN+10000	
    let OVERLAY_VIDEO_WIDTH=$VIDEO_WIDTH-64
    let OVERLAY_VIDEO_HEIGHT=$VIDEO_HEIGHT-64
    echo "Overlay width is $OVERLAY_VIDEO_WIDTH"
    v4l2-ctl --get-fmt-overlay
    sudo modprobe bcm2835-v4l2
    v4l2-ctl --set-fmt-video=width=$VIDEO_WIDTH,height=$VIDEO_HEIGHT,pixelformat=0
    v4l2-ctl --set-fmt-overlay=left=0,top=0,width=$OVERLAY_VIDEO_WIDTH,height=$OVERLAY_VIDEO_HEIGHT
    v4l2-ctl -p $VIDEO_FPS
    let DELAY=(BITRATE_VIDEO*8)/10
    # If sound arrives first, decrease the numeric number to delay it
    # "-00:00:0.7" works well at SR1000 on IQ mode
    # "-00:00:1.0" works well at SR2000 on IQ mode
    ITS_OFFSET="-00:00:1.0"

    case "$MODE_OUTPUT" in
      "BATC")
        ITS_OFFSET="-00:00:5.0"
        #sudo nice -n -30 $PATHRPI"/ffmpeg" -i videots -y $OUTPUT_STREAM &
        sudo nice -n -30 $PATHRPI"/ffmpeg" -i videots -y  -video_size 640x480\
          -b:v 500k -maxrate 700k -bufsize 2048k $OUTPUT_BATC &
        OUTPUT="videots"
      ;;
      "STREAMER")
        ITS_OFFSET="-00:00:5.0"
        #sudo nice -n -30 $PATHRPI"/ffmpeg" -i videots -y $OUTPUT_STREAM &
        sudo nice -n -30 $PATHRPI"/ffmpeg" -i videots -y  -video_size 640x480\
          -b:v 500k -maxrate 700k -bufsize 2048k $OUTPUT_STREAM &
        OUTPUT="videots"
      ;;
      "IP")
        : # Do nothing
      ;;
      "DATVEXPRESS")
        echo "set ptt tx" >> /tmp/expctrl
        # ffmpeg sends the stream directly to DATVEXPRESS
      ;;
      *)
        # For IQ, QPSKRF, DIGITHIN and DTX1 rpidatv generates the IQ (and RF for QPSKRF)
        sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
      ;;
    esac

#    AUDIO_CARD=0

    if [ "$AUDIO_CARD" == 0 ]; then
      # ******************************* MPEG-2 VIDEO WITH BEEP ************************************
	let BITRATE_VIDEO=BITRATE_VIDEO-72000
	
      sudo $PATHRPI"/ffmpeg"  -loglevel $MODE_DEBUG -itsoffset -00:00:0.2\
        -analyzeduration 0 -probesize 2048  -fpsprobesize 0 -re -ac 1 -f lavfi -thread_queue_size 512\
        -i "sine=frequency=500:beep_factor=4:sample_rate=44100:duration=3600"\
        -f v4l2 -framerate $VIDEO_FPS -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT"\
        -i /dev/video0 -fflags nobuffer -vcodec mpeg2video -s "$VIDEO_WIDTH"x"$VIDEO_HEIGHT"\
        -aspect 4:3 -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO\
        -f mpegts  -blocksize 1880 -strict experimental  -acodec mp2 -ab 64K -ar 44100 -ac 1\
        -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID\
        -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL\
        -metadata service_name=$CHANNEL -muxrate $BITRATE_TS -y $OUTPUT &
    else
      # ******************************* MPEG-2 VIDEO WITH AUDIO ************************************
		let BITRATE_VIDEO=BITRATE_VIDEO-72000
	
      sudo nice -n -30 arecord -f S16_LE -r 44100 -c 1 -M -D hw:1\
        |sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -itsoffset "$ITS_OFFSET"\
        -analyzeduration 0 -probesize 2048  -fpsprobesize 0 -ac 1 -thread_queue_size 512\
        -i -  -f v4l2 -framerate $VIDEO_FPS -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT"\
        -i /dev/video0 -fflags nobuffer -vcodec mpeg2video -s "$VIDEO_WIDTH"x"$VIDEO_HEIGHT"\
        -aspect 4:3 -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO\
        -f mpegts  -blocksize 1880 -strict experimental  -acodec mp2 -ab 64K -ar 44100 -ac 1\
        -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID\
        -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL\
        -metadata service_name=$CHANNEL -muxrate $BITRATE_TS -y $OUTPUT &
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
	"DATVEXPRESS")
                echo "set ptt tx" >> /tmp/expctrl
		sudo nice -n -30 netcat -u -4 127.0.0.1 1314 < videots & ;;
	*)
		sudo  $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &;;
	esac

let BITRATE_VIDEO=BITRATE_VIDEO-32000

$PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT -f $VIDEO_FPS -i 100 $OUTPUT_FILE -t 3 -p $PIDPMT -s $CHANNEL $OUTPUT_IP  &

$PATHRPI"/tcanim" $PATERNFILE"/*10" "48" "72" "CQ" "CQ CQ CQ DE "$CALL" IN $LOCATOR - DATV $SYMBOLRATEK KS FEC "$FECNUM"/"$FECDEN &

#need a way to loop audio file 

# $PATHRPI"/ffmpeg" -re -i $PATHSCRIPT"/sounds/sw.wav" -f u16le -acodec pcm_s16le -ar 48000 -ac 1 - | fdkaac --raw --raw-channels 1 --raw-rate 48000 -p5 -b8 -f2  - -o $PATHSCRIPT"/output.aac" &	

 $PATHRPI"/ffmpeg" -re -f lavfi -i "sine=frequency=500:beep_factor=4:sample_rate=48000:duration=3600" -f u16le -acodec pcm_s16le -ar 48000 -ac 1 - | fdkaac --raw --raw-channels 1 --raw-rate 48000 -p5 -b 24000 -m 0 -f2  - -o $PATHSCRIPT"/output.aac" &	


;;

#============================================ VNC =============================================================


"VNC")
sudo modprobe -r bcm2835_v4l2
case "$MODE_OUTPUT" in
	"BATC")
		sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i videots -y $OUTPUT_BATC & ;;
	"IP")
		OUTPUT_FILE="" ;;
	"DATVEXPRESS")
          echo "set ptt tx" >> /tmp/expctrl
	  sudo nice -n -30 netcat -u -4 127.0.0.1 1314 < videots &
        ;;
	*)
		sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &;;
	esac


$PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT -f $VIDEO_FPS -i 100 $OUTPUT_FILE -t 4 -e $VNCADDR -p $PIDPMT -s $CHANNEL $OUTPUT_IP &

;;

  #============================================ ANALOG =============================================================
  "ANALOGCAM")

    if [ "$ANALOGCAMINPUT" != "-" ]; then
      v4l2-ctl -d $ANALOGCAMNAME "--set-input="$ANALOGCAMINPUT
    fi
    if [ "$ANALOGCAMSTANDARD" != "-" ]; then
      v4l2-ctl -d $ANALOGCAMNAME "--set-standard="$ANALOGCAMSTANDARD
    fi

    sudo modprobe -r bcm2835_v4l2

    case "$MODE_OUTPUT" in
      "BATC")
#    sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i videots -y $OUTPUT_BATC & 
        sudo nice -n 0 $PATHRPI"/ffmpeg" -i videots -y $OUTPUT_BATC &
        OUTPUT_FILE="videots"
      ;;
      "IP")
        OUTPUT_FILE=""
      ;;
      "DATVEXPRESS")
        echo "set ptt tx" >> /tmp/expctrl
        sudo nice -n -30 netcat -u -4 127.0.0.1 1314 < videots &
      ;;
      *)
        sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
      ;;
    esac

    $PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT\
      -f $VIDEO_FPS -i 100 $OUTPUT_FILE -t 2 -e $ANALOGCAMNAME -p $PIDPMT -s $CHANNEL $OUTPUT_IP &
  ;;

#============================================ DESKTOP =============================================================
"DESKTOP")
sudo modprobe -r bcm2835_v4l2
case "$MODE_OUTPUT" in
	"BATC")
		sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i videots -y $OUTPUT_BATC & ;;
	"IP")
		OUTPUT_FILE="" ;;
	"DATVEXPRESS")
          echo "set ptt tx" >> /tmp/expctrl
	  sudo nice -n -30 netcat -u -4 127.0.0.1 1314 < videots &
        ;;
	*)
		sudo nice -n -30 $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &;;
	esac

$PATHRPI"/avc2ts" -b $BITRATE_VIDEO -m $BITRATE_TS -x $VIDEO_WIDTH -y $VIDEO_HEIGHT -f $VIDEO_FPS -i 100 $OUTPUT_FILE -t 3 -p $PIDPMT -s $CHANNEL $OUTPUT_IP &

$PATHRPI"/ffmpeg" -re -f lavfi -i "sine=frequency=500:beep_factor=4:sample_rate=48000:duration=3600" -f u16le -acodec pcm_s16le -ar 48000 -ac 1 - | fdkaac --raw --raw-channels 1 --raw-rate 48000 -p5 -b 24000 -m 0 -f2  - -o $PATHSCRIPT"/output.aac" &	

;;

# *********************************** TRANSPORT STREAM INPUT THROUGH IP ******************************************
"IPTSIN")
case "$MODE_OUTPUT" in
	"BATC")
		sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i videots -y $OUTPUT_BATC & ;;
	"DATVEXPRESS")
		nice -n -30 nc -u -4 127.0.0.1 1314 < videots & ;;
	*)
		sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &;;
	esac

PORT=10000
# $PATHRPI"/mnc" -l -i eth0 -p $PORT $UDPINADDR > videots &
# Unclear why Evariste uses multicast address here - my BT router dislikes routing multicast intensely so
# I have changed it to just listen on the predefined port number for a UDP stream
	netcat -u -4 -l $PORT > videots &
;;

  # *********************************** TRANSPORT STREAM INPUT FILE ******************************************
  "FILETS")
    case "$MODE_OUTPUT" in
      "BATC")
        sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i $TSVIDEOFILE -y $OUTPUT_BATC &
      ;;
      "DATVEXPRESS")
        echo "set ptt tx" >> /tmp/expctrl
        sudo nice -n -30 netcat -u -4 127.0.0.1 1314 < $TSVIDEOFILE &
        #sudo nice -n -30 cat $TSVIDEOFILE | sudo nice -n -30 netcat -u -4 127.0.0.1 1314 & 
      ;;
      *)
        sudo $PATHRPI"/rpidatv" -i $TSVIDEOFILE -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -l -x $PIN_I -y $PIN_Q &;;
    esac
  ;;

  # *********************************** CARRIER  ******************************************
  "CARRIER")
    case "$MODE_OUTPUT" in
      "DATVEXPRESS")
        echo "set car on" >> /tmp/expctrl
        echo "set ptt tx" >> /tmp/expctrl
      ;;
      *)
        # sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c "carrier" -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &

        # Temporary fix for swapped carrier and test modes:
        sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c "tesmode" -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
      ;;
    esac
  ;;

  # *********************************** TESTMODE  ******************************************
  "TESTMODE")
    # sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c "tesmode" -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &

    # Temporary fix for swapped carrier and test modes:
    sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c "carrier" -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
  ;;
esac
