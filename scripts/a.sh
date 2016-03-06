
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
sudo killall ffmpeg >/dev/null 2>/dev/null
sudo killall h264 >/dev/null 2>/dev/null
sudo killall rpidatv >/dev/null 2>/dev/null
sudo killall cat >/dev/null 2>/dev/null
sudo killall hello_encode.bin >/dev/null 2>/dev/null
sudo killall h264yuv >/dev/null 2>/dev/null
sudo killall raspivid >/dev/null 2>/dev/null
sudo killall express_server >/dev/null 2>/dev/null
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
#v4l2-ctl --overlay=0

PIN_I=12
PIN_Q=13

detect_audio

let SYMBOLRATE=SYMBOLRATEK*1000
FEC=$(get_config_var fec $CONFIGFILE)
let FECNUM=FEC
let FECDEN=FEC+1

case "$MODE_OUTPUT" in
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


OUTPUT_IP="udp://230.0.0.1:10000?pkt_size=1316&buffer_size=1316"
OUTPUT_QPSK="videots"


# ************************ OUTPUT MODE DEFINITION ******************
#OUTPUT=$OUTPUT_IP
#OUTPUT=$OUTPUT_QPSK
MODE_DEBUG=debug

#BITRATE AVEC 5%
# BITRATE TS THEORIC
let BITRATE_TS=SYMBOLRATE*2*188*FECNUM/204/FECDEN 
#let BITRATE_TS=SYMBOLRATE*2*188*FECNUM/204/FECDEN+1000


let BITRATE_VIDEO=(BITRATE_TS*7)/10-72000
#let DELAY=BITRATE_VIDEO*10
#let DELAY=(BITRATE_VIDEO*8)/10
let DELAY=(BITRATE_VIDEO*8)/10
let SYMBOLRATE_K=SYMBOLRATE/1000


if [ "$BITRATE_VIDEO" -lt 1000000 ]; then
VIDEO_WIDTH=352
VIDEO_HEIGHT=288
else
VIDEO_WIDTH=720
VIDEO_HEIGHT=576
fi



sudo rm videoes
sudo rm videots
sudo rm netfifo
mkfifo videoes
mkfifo videots
mkfifo netfifo 


echo $PATHRPI"/rpidatv" videots $SYMBOLRATE_K $FECNUM 0 $FREQUENCY_OUT $GAIN $DIGITHIN_MODE
echo "************************************"
echo $BITRATE_TS
echo "************************************"
echo "ModeINPUT="$MODE_INPUT

case "$MODE_INPUT" in
"CAMH264")
let BITRATE_TS=SYMBOLRATE*2*188*FECNUM/204/FECDEN-1*SYMBOLRATE*2*188*FECNUM/204/FECDEN/100 
if [ "$AUDIO_CARD" == 0 ]; then
# ******************************* VIDEO ONLY ************************************
#sudo nice -n -30 $PATHRPI"/h264" videoes $BITRATE_VIDEO $VIDEO_WIDTH $VIDEO_HEIGHT &
VIDEO_FPS=25
sudo modprobe bcm2835-v4l2
#OVERLAY ENABLE ! BE SURE NOT TO BE ON RF MODE !!!!
v4l2-ctl --overlay=1
v4l2-ctl --set-fmt-video=width=$VIDEO_WIDTH,height=$VIDEO_HEIGHT,pixelformat=4
v4l2-ctl --set-parm=$VIDEO_FPS
v4l2-ctl --set-ctrl video_bitrate=$BITRATE_VIDEO
v4l2-ctl --set-ctrl repeat_sequence_header=1
v4l2-ctl --overlay=1
sudo nice -n -30 cat /dev/video0 > videoes &

########### TEST UV4L ############################
#uv4l --driver raspicam --auto-video_nr --encoding h264 --width $VIDEO_WIDTH --height $VIDEO_HEIGHT --framerate 25 --bitrate $BITRATE_VIDEO --inline-headers --text-overlay --text-filename /home/pi/rpidatv/text.json 
#cat /dev/video0 > videoes &

#./tsudpsend videots 230.0.0.2 10000 $BITRATE_TS &
#$PATHRPI"/mnc" -l -p 10000 230.0.0.2 > netfifo &

sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
#sudo $PATHRPI"/tsudpsend" videots 230.0.0.1 10000 $BITRATE_TS 7 &
#sudo nice -n -30 $PATHRPI"/ffmpeg"  -loglevel $MODE_DEBUG  -analyzeduration 100 -probesize 2048 -ac 1 -thread_queue_size 512 -f lavfi -i "sine=frequency=500:beep_factor=4:sample_rate=48000:duration=3600" -r 25 -async 25 -fpsprobesize 0 -analyzeduration 0 -thread_queue_size 512 -i videoes  -f h264 -r $VIDEO_FPS -vcodec copy -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -blocksize 1880 -strict experimental  -acodec mp2 -ab 64K -ar 48k -ac 1 -f mpegts -blocksize 1880 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -bufsize 1880 -muxrate $BITRATE_TS -y $OUTPUT &
sudo nice -n -30 $PATHRPI"/ffmpeg"  -loglevel $MODE_DEBUG  -analyzeduration 100  -r 25 -async 25 -fpsprobesize 0 -analyzeduration 0 -thread_queue_size 512 -i videoes  -f h264 -r $VIDEO_FPS -vcodec copy -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -blocksize 1880 -strict experimental  -f mpegts -blocksize 1880 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -bufsize 1880 -muxrate $BITRATE_TS -y $OUTPUT &
else
echo cam with audio
FPS=25
#sudo nice -n -30 $PATHRPI"/h264" videoes $BITRATE_VIDEO $VIDEO_WIDTH $VIDEO_HEIGHT &
sudo modprobe bcm2835-v4l2
v4l2-ctl --overlay=1
v4l2-ctl --set-fmt-video=width=$VIDEO_WIDTH,height=$VIDEO_HEIGHT,pixelformat=4
v4l2-ctl --set-parm=$FPS
v4l2-ctl --set-ctrl video_bitrate=$BITRATE_VIDEO
v4l2-ctl --set-ctrl repeat_sequence_header=1
sudo nice -n -30 cat /dev/video0 > videoes &

#sudo $PATHRPI"/rpidatv" videots $SYMBOLRATE_K $FECNUM 0 $FREQUENCY_OUT $GAIN $DIGITHIN_MODE &
sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
sudo nice -n -30 arecord -f S16_LE -r 48000 -c 1 -M -D hw:1 | sudo nice -n -30 $PATHRPI"/ffmpeg"  -loglevel $MODE_DEBUG -itsoffset -00:00:0.8 -analyzeduration 0 -probesize 2048  -fpsprobesize 0 -ac 1 -thread_queue_size 512  -i -  -analyzeduration 0 -probesize 2048 -r 25 -fpsprobesize 0  -i videoes -max_delay 0  -f h264 -r $VIDEO_FPS  -vcodec copy -blocksize 1504 -strict experimental -async 2 -acodec mp2 -ab 64K -ar 48k -ac 1  -flags -global_header -f mpegts -blocksize 1504 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -bufsize 1880 -muxrate $BITRATE_TS -y $OUTPUT &
fi
;;


# ******************************* VIDEO ONLY MPEG-2 ************************************
"CAMMPEG-2")
VIDEO_WIDTH=352
VIDEO_HEIGHT=288
VIDEO_FPS=25
echo MPEG-2
if [ "$AUDIO_CARD" == 1 ]; then
#-------------- WITHH SOUNDCARD ------------
sudo $PATHRPI"/rpidatv" videots $SYMBOLRATE_K $FECNUM 0 $FREQUENCY_OUT $GAIN $DIGITHIN_MODE &
sudo modprobe bcm2835-v4l2
v4l2-ctl --set-fmt-video=width=$VIDEO_WIDTH,height=$VIDEO_HEIGHT,pixelformat=0
v4l2-ctl --set-parm=$VIDEO_FPS
v4l2-ctl --overlay=1
let DELAY=(BITRATE_VIDEO*8)/10
sudo nice -n -30 arecord -f S16_LE -r 48000 -c 1 -M -D hw:1 |sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -itsoffset -00:00:0.8 -analyzeduration 0 -probesize 2048  -fpsprobesize 0 -ac 1 -thread_queue_size 512 -i -  -f v4l2 -framerate $VIDEO_FPS -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -i /dev/video0 -fflags nobuffer -vcodec mpeg2video -s "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -f mpegts  -blocksize 1880 -strict experimental  -acodec mp2 -ab 64K -ar 48k -ac 1 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -muxrate $BITRATE_TS -y $OUTPUT &
else
#---------------- WITH AUDIO TONE -------------
#buffer -s 1880 -b 100 < videots >netfifo &

#sudo $PATHRPI"/rpidatv" videots $SYMBOLRATE_K $FECNUM 0 $FREQUENCY_OUT $GAIN $DIGITHIN_MODE &
sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
sudo modprobe bcm2835-v4l2
v4l2-ctl --set-fmt-video=width=352,height=288,pixelformat=0
v4l2-ctl --set-parm=15
v4l2-ctl --overlay=1

#echo ezcap
#v4l2-ctl -d /dev/video1 -i 1 -s 9 --set-fmt-video=width=720,height=576,pixelformat=0
#v4l2-ctl -d /dev/video1 --set-parm=15


#let BITRATE_VIDEO=BITRATE_TS*8/10

sudo $PATHRPI"/ffmpeg"  -loglevel $MODE_DEBUG -itsoffset -00:00:0.2 -analyzeduration 0 -probesize 2048  -fpsprobesize 0 -re -ac 1 -f lavfi -thread_queue_size 512 -i "sine=frequency=500:beep_factor=4:sample_rate=48000:duration=3600" -f v4l2 -framerate $VIDEO_FPS -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -i /dev/video0 -fflags nobuffer -vcodec mpeg2video -s "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -f mpegts  -blocksize 1880 -strict experimental  -acodec mp2 -ab 64K -ar 48k -ac 1  -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -muxrate $BITRATE_TS -y $OUTPUT &
fi
;;
# ffmpeg -y -f image2 -r 1/5 -i img%03d.jpg -pix_fmt yuv420p -r 25 output.mp4 
# ******************************* VIDEO ONLY PATERN ************************************
"PATERNAUDIO")
VIDEO_WIDTH=720
VIDEO_HEIGHT=576
FPS=0.5
#BITRATE_TS=1700000
#PATERNFILE=./mire.jpg
# $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -f image2 -framerate $FPS -s 720x576 -loop 1 -i ./mire720.jpg  -r 1/1000 -vcodec mpeg2video -r $FPS -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v $BITRATE_VIDEO -t 0:0:1 -y mire.mpg

#buffer -s 188 -m 188000 < videots >netfifo &
let BITRATE_TS=SYMBOLRATE*2*188*FECNUM/204/FECDEN 
#sudo $PATHRPI"/rpidatv" videots $SYMBOLRATE_K $FECNUM 0 $FREQUENCY_OUT $GAIN $DIGITHIN_MODE &
sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
#sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG  -probesize 2048 -re -fflags flush_packets -f image2 -r $FPS -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -loop 1  -i $PATERNFILE -vf scale="$VIDEO_WIDTH":"$VIDEO_HEIGHT"  -vcodec mpeg2video -r $FPS -s "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -f mpegts -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id 100 -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL  -muxrate $BITRATE_TS -y $OUTPUT &


sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG  -probesize 2048 -itsoffset -00:00:0.2 -ac 1 -f lavfi -thread_queue_size 512 -re  -i "sine=frequency=300:beep_factor=4:sample_rate=48000:duration=3600" -re -fflags flush_packets -f image2 -r $FPS -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -loop 1  -i $PATERNFILE -vf scale="$VIDEO_WIDTH":"$VIDEO_HEIGHT"  -vcodec mpeg2video -r $FPS -s "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -strict experimental  -acodec mp2 -ab 64K -ar 48k -ac 1 -f mpegts   -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id 100 -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL  -muxrate $BITRATE_TS -y $OUTPUT    &



#mkfifo mireloop.ts
#./tsloop mire.ts > mireloop.ts &
#./tspcrrestamp mireloop.ts $BITRATE_TS > videots &


#sudo nice -n -30 arecord -f S16_LE -r 48000 -c 1 -M -D hw:1 |sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -itsoffset -00:00:0.2 -analyzeduration 0 -probesize 2048  -fpsprobesize 0 -ac 1 -fflags nobuffer -i -  -f image2 -framerate $FPS -video_size "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -i $PATERNFILE -vf scale=352:288 -r 15 -vcodec mpeg2video -s "$VIDEO_WIDTH"x"$VIDEO_HEIGHT" -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -f mpegts  -strict experimental -async 2 -acodec mp2 -ab 64K -ar 48k -ac 1 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -muxrate $BITRATE_TS -y $OUTPUT &


#sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG   -max_delay 0 -fflags nobuffer  -i $PATERNFILE -vf scale=352:288  -r 15 -vcodec mpeg2video -s 352x288 -b:v $BITRATE_VIDEO -minrate:v $BITRATE_VIDEO -maxrate:v  $BITRATE_VIDEO -f mpegts -max_delay $DELAY -blocksize 1504  -f mpegts -max_delay $DELAY -blocksize 1504 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -bufsize 1880 -muxrate $BITRATE_TS -y $OUTPUT &
;;

# ******************************* VIDEO ONLY PATERN ************************************
"PATERNH264")
sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i $PATERNFILE -vf scale=352:288 -pix_fmt yuv420p -y patern.yuv
$PATHRPI"/h264yuv" videoes patern.yuv  &
#sudo sudo $PATHRPI"/rpidatv" videots $SYMBOLRATE_K $FECNUM 0 $FREQUENCY_OUT $GAIN $DIGITHIN_MODE &
sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG  -analyzeduration 0 -probesize 2048 -r 25 -async 25 -fpsprobesize 0  -i videoes -max_delay 0 -fflags nobuffer -f h264 -r $VIDEO_FPS -vcodec copy -blocksize 1504  -f mpegts -max_delay $DELAY -blocksize 1504 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id $SERVICEID -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -bufsize 1880 -muxrate $BITRATE_TS -y $OUTPUT &
;;

# ******************************* VIDEO/AUDIO PATERN ************************************
"OLDPATERNAUDIOH264")
$PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -i $PATERNFILE -vf scale=1440:720 -pix_fmt yuv420p -y patern.yuv
$PATHRPI"/h264yuv" videoes patern.yuv 1440 720 $BITRATE_VIDEO 1 &
#sudo $PATHRPI"/rpidatv" videots $SYMBOLRATE_K $FECNUM 0 $FREQUENCY_OUT $GAIN $DIGITHIN_MODE &
sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE &
sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG -probesize 2048  -ac 1 -f lavfi  -thread_queue_size 512 -i "sine=frequency=500:beep_factor=4:sample_rate=48000:duration=3600" -f h264 -r 1 -analyzeduration 0  -thread_queue_size 512 -i videoes   -f h264 -r 1 -vcodec copy -strict experimental -acodec mp2 -ab 64K -ar 48k -ac 1  -flags -global_header -f mpegts   -blocksize 1504 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id 100 -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -muxrate $BITRATE_TS -y $OUTPUT &
;;



# *********************************** TRANSPORT STREAM INPUT THROUGH IP ******************************************
"IPTSIN")
#sudo $PATHRPI"/rpidatv" videots $SYMBOLRATE_K $FECNUM 0 $FREQUENCY_OUT $GAIN $DIGITHIN_MODE &
sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
PORT=10000
$PATHRPI"/mnc" -l -i eth0 -p $PORT $UDPINADDR > videots &
;;

# *********************************** TRANSPORT STREAM INPUT FILE ******************************************
"FILETS")
	
	#sudo $PATHRPI"/rpidatv" $TSVIDEOFILE $SYMBOLRATE_K $FECNUM 1 $FREQUENCY_OUT $GAIN $DIGITHIN_MODE &
sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
;;

# *********************************** CARRIER  ******************************************
"CARRIER")
echo ====================== CARRIER ==========================
#sudo $PATHRPI"/rpidatv" $TSVIDEOFILE $SYMBOLRATE_K 0 1 $FREQUENCY_OUT $GAIN $DIGITHIN_MODE &
sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
;;

# *********************************** TESTMODE  ******************************************
"TESTMODE")
#sudo $PATHRPI"/rpidatv" $TSVIDEOFILE $SYMBOLRATE_K -$FECNUM 1 $FREQUENCY_OUT $GAIN $DIGITHIN_MODE &
sudo $PATHRPI"/rpidatv" -i videots -s $SYMBOLRATE_K -c $FECNUM"/"$FECDEN -f $FREQUENCY_OUT -p $GAIN -m $MODE -x $PIN_I -y $PIN_Q &
;;

# *********************************** BATC  ******************************************
"BATC")
$PATHRPI"/h264" videoes $BITRATE_VIDEO $FPS &
sudo nice -n -30 $PATHRPI"/ffmpeg" -loglevel $MODE_DEBUG  -analyzeduration 0 -probesize 2048 -r 25 -async 25 -fpsprobesize 0  -i videoes -max_delay 0 -fflags nobuffer -f h264 -r $VIDEO_FPS -vcodec copy -blocksize 1504  -f mpegts -max_delay $DELAY -blocksize 1504 -mpegts_original_network_id 1 -mpegts_transport_stream_id 1 -mpegts_service_id 100 -mpegts_pmt_start_pid $PIDPMT -mpegts_start_pid $PIDVIDEO -metadata service_provider=$CALL -metadata service_name=$CHANNEL -bufsize 1880 -muxrate $BITRATE_TS -y $OUTPUT_BATC &
;;

esac


#../mnc/mnc -l -i eth0 -p 10000 230.1.0.1 > videots
#cat videots > /dev/null &

#sudo killall ffmpeg
#sudo killall h264
#sudo killall rpidatv
#sudo killall cat
#sudo killall hello_encode.bin

