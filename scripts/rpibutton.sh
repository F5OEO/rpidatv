PATHRPI=/home/pi/RpiDATV
CONFIGFILE=$PATHRPI"/rpidatvconfig.txt"


############### PIN DEFINITION ###########"
#Source=GPIO 5 / Header 29  
button_source=5
#SYmbolRate=GPIO 6 / Header 31
button_SR=6
#Fec=GPIO 13 / Header 33
button_FEC=13
#OUTLED SOURCECAM=GPIO26 /Header 37
led_source_cam=26
#OUTLED SOURCETEST=GPIO21 /Header 40
led_source_test=21
#OUTLED SR=GPIO20 /Header 38
led_SR=20
#OUTLED FEC=GPIO16 /Header 36
led_FEC=16 


gpio -g mode $button_source in
gpio -g mode $button_SR in
gpio -g mode $button_FEC in

#gpio  edge $button_source falling
#gpio  edge $button_SR falling
#gpio  edge $button_FEC falling

gpio -g mode $button_source up
gpio -g mode $button_SR up
gpio -g mode $button_FEC up

gpio -g mode $led_source_cam out
gpio -g mode $led_source_test out
gpio -g mode $led_SR out
gpio -g mode $led_FEC out



############### IN/OUT CONFIG FILE
set_config_var() {
lua - "$1" "$2" "$3" <<EOF > "$3.bak2"
local key=assert(arg[1])
local value=assert(arg[2])
local fn=assert(arg[3])
local file=assert(io.open(fn))
local made_change=false
for line in file:lines() do
if line:match("^#?%s*"..key.."=.*$") then
line=key.."="..value
made_change=true
end
print(line)
end
if not made_change then
print(key.."="..value)
end
EOF
mv "$3.bak2" "$3"
}

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

##################### TRANSMIT ##############
do_display_off()
{
	tvservice -o
}

do_transmit() 
{
	do_refresh_config
	SCRIPT=$(readlink -f $0)
	# Absolute path this script is in. /home/user/bin
	SCRIPTPATH=`dirname $SCRIPT`

	
	
	#whiptail --title "TRANSMITING" --msgbox "$INFO" 8 78
	#kill -1 $(pidof -x frmenu.sh) 
	sudo killall ffmpeg >/dev/null 2>/dev/null
	sudo killall h264 >/dev/null 2>/dev/null
	sudo killall RpiDATV >/dev/null 2>/dev/null
	sudo killall cat >/dev/null 2>/dev/null
	sudo killall hello_encode.bin >/dev/null 2>/dev/null
	sudo killall h264yuv >/dev/null 2>/dev/null
	do_display_off
	
	sleep 0.5
	$SCRIPTPATH"/a.sh" >/dev/null 2>/dev/null &
	sleep 0.5
	#$SCRIPTPATH"/a.sh" &
	#exit 0
}

do_refresh_config()
{
	MODE_INPUT=$(get_config_var modeinput $CONFIGFILE)
	SYMBOLRATEK=$(get_config_var symbolrate $CONFIGFILE)
	FEC=$(get_config_var fec $CONFIGFILE)
}

do_process_button()
{
	if [ `gpio -g read $button_source` = 0 ]; then
		
		 case "$MODE_INPUT" in
			CAMH264) 
				NEW_MODE_INPUT=CAMMPEG-2;
			;;
			CAMMPEG-2) 
				NEW_MODE_INPUT=PATERNAUDIO;	
			;;
			PATERNAUDIO)
				NEW_MODE_INPUT=CARRIER;
			;;
			CARRIER)
				NEW_MODE_INPUT=CAMH264
			;;
			*)
				NEW_MODE_INPUT=CAMH264
			;;
		esac
		set_config_var modeinput "$NEW_MODE_INPUT" $CONFIGFILE
		
		echo $NEW_MODE_INPUT
		do_transmit
	fi
	if [ `gpio -g read $button_SR` = 0 ]; then 
			case "$SYMBOLRATEK" in
			250) 
				NEW_SR=500;	
			;;
			500)
				NEW_SR=1024;
			;;
			1024)
				NEW_SR=250
			;;
			*)
				NEW_SR=250
			;;
		esac
		set_config_var symbolrate "$NEW_SR" $CONFIGFILE
		#echo $NEW_SR
		do_transmit
	fi
	if [ `gpio -g read $button_FEC` = 0 ];then
	
	case "$FEC" in
			7) 
				NEW_FEC=1;	
			;;
			1)
				NEW_FEC=7;
			;;
			*)
				NEW_FEC=1
			;;
		esac
		set_config_var fec "$NEW_FEC" $CONFIGFILE
		#echo $NEW_FEC
		do_transmit
	fi
}

do_process_output()
{
	#do_refresh_config
	let blink=($blink+1)%2
	case "$MODE_INPUT" in
			CAMH264) 
				gpio -g write $led_source_cam 1
				gpio -g write $led_source_test 0
				
			;;
			CAMMPEG-2) 
				gpio -g write $led_source_cam $blink
				gpio -g write $led_source_test 0
					
			;;
			PATERNAUDIO)
				gpio -g write $led_source_cam 0
				gpio -g write $led_source_test 1
			;;
			CARRIER)
				gpio -g write $led_source_cam 0
				gpio -g write $led_source_test $blink
			;;
			*)
				gpio -g write $led_source_test 0
				gpio -g write $led_source_cam 0 
			;;	
	esac
	case "$NEW_SR" in
			250) 
				gpio -g write $led_SR 0	
			;;
			500) 
				gpio -g write $led_SR $blink	
			;;
			1024)
				gpio -g write $led_SR 1
			;;
			*)
				gpio -g write $led_SR 0
			;;
		esac

	case "$FEC" in
			1) 
				gpio -g write $led_FEC $blink
			;;
			7)
				gpio -g write $led_FEC 1
			;;
			*)
				gpio -g write $led_FEC 0
			;;
		esac
	
}
##################### MAIN PROGRAM ##############

#do_transmit
do_refresh_config
let compteur=0
while true; do
	
	do_process_button
	
	let compteur=(compteur+1)%2
	
	if [ "$compteur" == 0 ]; then
		
	do_process_output
		
		
	fi
sleep 0.5

done

