#!/bin/bash

############ Set Environment Variables ###############

PATHSCRIPT=/home/pi/rpidatv/scripts
PATHRPI=/home/pi/rpidatv/bin
CONFIGFILE=$PATHSCRIPT"/rpidatvconfig.txt"
PATHCONFIGS="/home/pi/rpidatv/scripts/configs"  ## Path to config files

############ Function to Write to Config File ###############

set_config_var() {
lua - "$1" "$2" "$3" <<EOF > "$3.bak"
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
mv "$3.bak" "$3"
}

############ Function to Read from Config File ###############

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

############ Function to Select Files ###############

Filebrowser() {
if [ -z $1 ]; then
imgpath=$(ls -lhp / | awk -F ' ' ' { print $9 " " $5 } ')
else
imgpath=$(ls -lhp "/$1" | awk -F ' ' ' { print $9 " " $5 } ')
fi
if [ -z $1 ]; then
pathselect=$(whiptail --menu "$FileBrowserTitle""$filename" 20 50 10 --cancel-button Cancel --ok-button Select $imgpath 3>&1 1>&2 2>&3)
else
pathselect=$(whiptail --menu "$FileBrowserTitle""$filename" 20 50 10 --cancel-button Cancel --ok-button Select ../ BACK $imgpath 3>&1 1>&2 2>&3)
fi
RET=$?
if [ $RET -eq 1 ]; then
## This is the section where you control what happens when the user hits Cancel
Cancel
elif [ $RET -eq 0 ]; then
	if [[ -d "/$1$pathselect" ]]; then
		Filebrowser "/$1$pathselect"
	elif [[ -f "/$1$pathselect" ]]; then
		## Do your thing here, this is just a stub of the code I had to do what I wanted the script to do.
		fileout=`file "$1$pathselect"`
		filename=`readlink -m $1$pathselect`
	else
		echo pathselect $1$pathselect
		whiptail --title "$FileMenuTitle" --msgbox "$FileMenuContext" 8 44
		unset base
		unset imgpath
		Filebrowser
	fi
fi
}

############ Function to Select Paths ###############

Pathbrowser() {
if [ -z $1 ]; then
imgpath=$(ls -lhp / | awk -F ' ' ' { print $9 " " $5 } ')
else
imgpath=$(ls -lhp "/$1" | awk -F ' ' ' { print $9 " " $5 } ')
fi
if [ -z $1 ]; then
pathselect=$(whiptail --menu "$FileBrowserTitle""$filename" 20 50 10 --cancel-button Cancel --ok-button Select $imgpath 3>&1 1>&2 2>&3)
else
pathselect=$(whiptail --menu "$FileBrowserTitle""$filename" 20 50 10 --cancel-button Cancel --ok-button Select ../ BACK $imgpath 3>&1 1>&2 2>&3)
fi
RET=$?
if [ $RET -eq 1 ]; then
## This is the section where you control what happens when the user hits Cancel
Cancel	
elif [ $RET -eq 0 ]; then
	if [[ -d "/$1$pathselect" ]]; then
		Pathbrowser "/$1$pathselect"
	elif [[ -f "/$1$pathselect" ]]; then
		## Do your thing here, this is just a stub of the code I had to do what I wanted the script to do.
		fileout=`file "$1$pathselect"`
		filenametemp=`readlink -m $1$pathselect`
		filename=`dirname $filenametemp` 

	else
		echo pathselect $1$pathselect
		whiptail --title "$FileMenuTitle" --msgbox "$FileMenuContext" 8 44
		unset base
		unset imgpath
		Pathbrowser
	fi

fi
}

do_input_setup() {
MODE_INPUT=$(get_config_var modeinput $CONFIGFILE)
case "$MODE_INPUT" in
	CAMH264) 
	Radio1=ON
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	Radio8=OFF
	Radio9=OFF
	Radio10=OFF
	;;
	CAMMPEG-2) 
	Radio1=OFF
	Radio2=ON
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	Radio8=OFF
	Radio9=OFF
	Radio10=OFF
	;;
	FILETS) 
	Radio1=OFF
	Radio2=OFF
	Radio3=ON
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	Radio8=OFF
	Radio9=OFF
	Radio10=OFF
	;;
	PATERNAUDIO) 
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=ON
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	Radio8=OFF
	Radio9=OFF
	Radio10=OFF
	;;
	CARRIER) 
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=ON
	Radio6=OFF
	Radio7=OFF
	Radio8=OFF
	Radio9=OFF
	Radio10=OFF
	;;
	TESTMODE) 
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=ON
	Radio7=OFF
	Radio8=OFF
	Radio9=OFF
	Radio10=OFF
	;;
	IPTSIN) 
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=ON
	Radio8=OFF
	Radio9=OFF
	Radio10=OFF
	;;
	ANALOGCAM) 
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	Radio8=ON
	Radio9=OFF
	Radio10=OFF
	;;
	VNC) 
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	Radio8=OFF
	Radio9=ON
	Radio10=OFF
	;;
	DESKTOP) 
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	Radio8=OFF
	Radio9=OFF
	Radio10=ON
	;;

	*) 
	Radio1=ON
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	Radio8=OFF
	Radio9=OFF
	Radio10=OFF
	;;
esac

chinput=$(whiptail --title "$StrInputSetupTitle" --radiolist \
		"$StrInputSetupDescription" 20 78 10 \
		"CAMH264" "$StrInputSetupCAMH264" $Radio1 \
		"CAMMPEG-2" "$StrInputSetupCAMMPEG_2" $Radio2 \
		"FILETS" "$StrInputSetupFILETS" $Radio3\
		"PATERNAUDIO" "$StrInputSetupPATERNAUDIO" $Radio4 \
		"CARRIER" "$StrInputSetupCARRIER" $Radio5 \
		"TESTMODE" "$StrInputSetupTESTMODE" $Radio6 \
		"IPTSIN" "$StrInputSetupIPTSIN" $Radio7 \
		"ANALOGCAM" "$StrInputSetupANALOGCAM" $Radio8 \
		"VNC" "$StrInputSetupVNC" $Radio9 \
		"DESKTOP" "$StrInputSetupDESKTOP" $Radio10 3>&2 2>&1 1>&3)
		if [ $? -eq 0 ]; then
	     	 case "$chinput" in
		    FILETS)
			TSVIDEOFILE=$(get_config_var tsvideofile $CONFIGFILE)
			filename=$TSVIDEOFILE
			FileBrowserTitle=TS:
			Filebrowser "$PATHTS/"
			whiptail --title "$StrInputSetupFILETSName" --msgbox "$filename" 8 44
			set_config_var tsvideofile "$filename" $CONFIGFILE
			PATHTS=`dirname $filename`
	 		set_config_var pathmedia "$PATHTS" $CONFIGFILE
		    ;;
	            PATERNAUDIO)
			PATERNFILE=$(get_config_var paternfile $CONFIGFILE)
			filename=$PATERNFILE
			FileBrowserTitle=JPEG:
			Pathbrowser "$PATHTS/"
			whiptail --title "$StrInputSetupPATERNAUDIOName" --msgbox "$filename" 8 44
			set_config_var paternfile "$filename" $CONFIGFILE
			set_config_var pathmedia "$filename" $CONFIGFILE	
		    ;;
		    IPTSIN)
		    UDPINADDR=$(get_config_var udpinaddr $CONFIGFILE)

		    UDPINADDR=$(whiptail --inputbox "$StrInputSetupIPTSINName" 8 78 $UDPINADDR --title "$StrInputSetupIPTSINTitle" 3>&1 1>&2 2>&3)
		    if [ $? -eq 0 ]; then
			set_config_var udpinaddr "$UDPINADDR" $CONFIGFILE
		    fi
		    ;;
		    ANALOGCAM)
		    ANALOGCAMNAME=$(get_config_var analogcamname $CONFIGFILE)
		    ANALOGCAMNAME=$(whiptail --inputbox "$StrInputSetupANALOGCAMName" 8 78 $ANALOGCAMNAME --title "$StrInputSetupANALOGCAMTitle" 3>&1 1>&2 2>&3)
		    if [ $? -eq 0 ]; then
			set_config_var analogcamname "$ANALOGCAMNAME" $CONFIGFILE
		    fi
		    ;;
		 VNC)
		    VNCADDR=$(get_config_var vncaddr $CONFIGFILE)

		    VNCADDR=$(whiptail --inputbox "$StrInputSetupVNCName" 8 78 $VNCADDR --title "$StrInputSetupVNCTitle" 3>&1 1>&2 2>&3)
		    if [ $? -eq 0 ]; then
			set_config_var vncaddr "$VNCADDR" $CONFIGFILE
		    fi
		    ;;
		esac
		set_config_var modeinput "$chinput" $CONFIGFILE
		fi
}

do_station_setup() {
CALL=$(get_config_var call $CONFIGFILE)
CALL=$(whiptail --inputbox "$StrCallContext" 8 78 $CALL --title "$StrCallTitle" 3>&1 1>&2 2>&3)
if [ $? -eq 0 ]; then
set_config_var call "$CALL" $CONFIGFILE
fi

LOCATOR=$(get_config_var locator $CONFIGFILE)
LOCATOR=$(whiptail --inputbox "$StrLocatorContext" 8 78 $LOCATOR --title "$StrLocatorTitle" 3>&1 1>&2 2>&3)
if [ $? -eq 0 ]; then
set_config_var locator "$LOCATOR" $CONFIGFILE
fi
}


do_output_setup_mode() {
MODE_OUTPUT=$(get_config_var modeoutput $CONFIGFILE)
case "$MODE_OUTPUT" in
	IQ) 
	Radio1=ON
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	;;
	QPSKRF)
	Radio1=OFF
	Radio2=ON
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	;;
	BATC)
	Radio1=OFF
	Radio2=OFF
	Radio3=ON
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	;;
	DIGITHIN)
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=ON
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	;;
	DTX1)
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=ON
	Radio6=OFF
	Radio7=OFF
	;;
	DATVEXPRESS)
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=ON
	Radio7=OFF
	;;
	IP)
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=ON
	;;
	*)
	Radio1=ON
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
esac

choutput=$(whiptail --title "$StrOutputSetupTitle" --radiolist \
		"$StrOutputSetupContext" 20 78 8 \
		"IQ" "$StrOutputSetupIQ" $Radio1 \
		"QPSKRF" "$StrOutputSetupRF" $Radio2 \
		"BATC" "$StrOutputSetupBATC" $Radio3 \
		"DIGITHIN" "$StrOutputSetupDigithin" $Radio4 \
 		"DTX1" "$StrOutputSetupDTX1" $Radio5 \
		"DATVEXPRESS" "$StrOutputSetupDATVExpress" $Radio6 \
	 	"IP" "$StrOutputSetupIP" $Radio7 3>&2 2>&1 1>&3)
if [ $? -eq 0 ]; then

		case "$choutput" in
		    IQ)
				PIN_I=$(get_config_var gpio_i $CONFIGFILE)
				PIN_I=$(whiptail --inputbox "$StrPIN_IContext" 8 78 $PIN_I --title "$StrPIN_ITitle" 3>&1 1>&2 2>&3)
			if [ $? -eq 0 ]; then
				set_config_var gpio_i "$PIN_I" $CONFIGFILE
			fi
				PIN_Q=$(get_config_var gpio_q $CONFIGFILE)
				PIN_Q=$(whiptail --inputbox "$StrPIN_QContext" 8 78 $PIN_Q --title "$StrPIN_QTitle" 3>&1 1>&2 2>&3)
			if [ $? -eq 0 ]; then
				set_config_var gpio_q "$PIN_Q" $CONFIGFILE
			fi
				;;
		    QPSKRF)
			FREQ_OUTPUT=$(get_config_var freqoutput $CONFIGFILE)
			##FREQ=$(whiptail --inputbox "$StrOutputRFFreqContext" 8 78 $FREQ_OUTPUT --title "$StrOutputRFFreqTitle" 3>&1 1>&2 2>&3)
			##if [ $? -eq 0 ]; then
			##	set_config_var freqoutput "$FREQ" $CONFIGFILE
			##fi
			GAIN_OUTPUT=$(get_config_var rfpower $CONFIGFILE)
			GAIN=$(whiptail --inputbox "$StrOutputRFGainContext" 8 78 $GAIN_OUTPUT --title "$StrOutputRFGainTitle" 3>&1 1>&2 2>&3)
			if [ $? -eq 0 ]; then
				set_config_var rfpower "$GAIN" $CONFIGFILE
			fi
		    ;;
	            BATC)
			BATC_OUTPUT=$(get_config_var batcoutput $CONFIGFILE)
			ADRESS=$(whiptail --inputbox "$StrOutputBATCContext" 8 78 $BATC_OUTPUT --title "$StrOutputBATCTitle" 3>&1 1>&2 2>&3)
			if [ $? -eq 0 ]; then
				set_config_var batcoutput "$ADRESS" $CONFIGFILE
			fi
			;;
		    DIGITHIN)
			PIN_I=$(get_config_var gpio_i $CONFIGFILE)
				PIN_I=$(whiptail --inputbox "$StrPIN_IContext" 8 78 $PIN_I --title "$StrPIN_ITitle" 3>&1 1>&2 2>&3)
			if [ $? -eq 0 ]; then
				set_config_var gpio_i "$PIN_I" $CONFIGFILE
			fi
				PIN_Q=$(get_config_var gpio_q $CONFIGFILE)
				PIN_Q=$(whiptail --inputbox "$StrPIN_QContext" 8 78 $PIN_Q --title "$StrPIN_QTitle" 3>&1 1>&2 2>&3)
			if [ $? -eq 0 ]; then
				set_config_var gpio_q "$PIN_Q" $CONFIGFILE
			fi
			FREQ_OUTPUT=$(get_config_var freqoutput $CONFIGFILE)
			FREQ=$(whiptail --inputbox "$StrOutputRFFreqContext" 8 78 $FREQ_OUTPUT --title "$StrOutputRFFreqTitle" 3>&1 1>&2 2>&3)
			if [ $? -eq 0 ]; then
				set_config_var freqoutput "$FREQ" $CONFIGFILE
			fi
		        sudo ./si570 -f $FREQ -m off
			;;
		    DTX1)	;;
		    DATVEXPRESS)
			FREQ_OUTPUT=$(get_config_var freqoutput $CONFIGFILE)
			FREQ=$(whiptail --inputbox "$StrOutputRFFreqContext" 8 78 $FREQ_OUTPUT --title "$StrOutputRFFreqTitle" 3>&1 1>&2 2>&3)
			if [ $? -eq 0 ]; then
				set_config_var freqoutput "$FREQ" $CONFIGFILE
			fi
			GAIN_OUTPUT=$(get_config_var rfpower $CONFIGFILE)
			GAIN=$(whiptail --inputbox "$StrOutputRFGainContext" 8 78 $GAIN_OUTPUT --title "$StrOutputRFGainTitle" 3>&1 1>&2 2>&3)
			if [ $? -eq 0 ]; then
				set_config_var rfpower "$GAIN" $CONFIGFILE
			fi
		    ;;
		    IP)
			UDPOUTADDR=$(get_config_var udpoutaddr $CONFIGFILE)

		    UDPOUTADDR=$(whiptail --inputbox "$StrOutputSetupIPTSOUTName" 8 78 $UDPOUTADDR --title "$StrOutputSetupIPTSOUTTitle" 3>&1 1>&2 2>&3)
		    if [ $? -eq 0 ]; then
			set_config_var udpoutaddr "$UDPOUTADDR" $CONFIGFILE
		    fi
		    ;;
		esac
		set_config_var modeoutput "$choutput" $CONFIGFILE
fi
}

do_symbolrate_setup()
{
	SYMBOLRATE=$(get_config_var symbolrate $CONFIGFILE)
	SYMBOLRATE=$(whiptail --inputbox "$StrOutputSymbolrateContext" 8 78 $SYMBOLRATE --title "$StrOutputSymbolrateTitle" 3>&1 1>&2 2>&3)
	if [ $? -eq 0 ]; then
		set_config_var symbolrate "$SYMBOLRATE" $CONFIGFILE
	fi
}

do_fec_setup()
{
	FEC=$(get_config_var fec $CONFIGFILE)
	case "$FEC" in
	1) 
	Radio1=ON
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	;;
	2)
	Radio1=OFF
	Radio2=ON
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	;;
	3)
	Radio1=OFF
	Radio2=OFF
	Radio3=ON
	Radio4=OFF
	Radio5=OFF
	;;
	5)
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=ON
	Radio5=OFF
	;;
	7)
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=ON
	;;
	*)
	Radio1=ON
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	;;
	esac
	FEC=$(whiptail --title "$StrOutputFECTitle" --radiolist \
		"$StrOutputFECContext" 20 78 8 \
		"1" "1/2" $Radio1 \
		"2" "2/3" $Radio2 \
		"3" "3/4" $Radio3 \
		"5" "5/6" $Radio4 \
		"7" "7/8" $Radio5 3>&2 2>&1 1>&3)
if [ $? -eq 0 ]; then
	set_config_var fec "$FEC" $CONFIGFILE
fi
}

do_PID_setup()
{
PID=$(get_config_var pidstart $CONFIGFILE)
PID=$(whiptail --inputbox "$StrPIDSetupContext" 8 78 $PID --title "$StrPIDSetupTitle" 3>&1 1>&2 2>&3)
if [ $? -eq 0 ]; then
set_config_var pidstart "$PID" $CONFIGFILE
set_config_var pidpmt "$PID" $CONFIGFILE
#PID Video is PMT+1
let PID=PID+1
set_config_var pidvideo "$PID" $CONFIGFILE
#PID Audiop is PMT+1
let PID=PID+1
set_config_var pidaudio "$PID" $CONFIGFILE
fi
}

do_freq_setup()
{
FREQ_OUTPUT=$(get_config_var freqoutput $CONFIGFILE)
FREQ=$(whiptail --inputbox "$StrOutputRFFreqContext" 8 78 $FREQ_OUTPUT --title "$StrOutputRFFreqTitle" 3>&1 1>&2 2>&3)
if [ $? -eq 0 ]; then
    set_config_var freqoutput "$FREQ" $CONFIGFILE
fi
}

do_output_setup() {
menuchoice=$(whiptail --title "$StrOutputTitle" --menu "$StrOutputContext" 16 78 5 \
        "1 SymbolRate" "$StrOutputSR"  \
        "2 FEC" "$StrOutputFEC" \
	"3 Output mode" "$StrOutputMode" \
	"4 PID" "$StrPIDSetup" \
	"5 Frequency" "$StrOutputRFFreqContext" \
	3>&2 2>&1 1>&3)
	case "$menuchoice" in
            1\ *) do_symbolrate_setup ;;
            2\ *) do_fec_setup   ;;
	    3\ *) do_output_setup_mode ;;
	    4\ *) do_PID_setup ;;
	    5\ *) do_freq_setup ;;
        esac
}


do_transmit() 
{
  # Call a.sh in a an additional process to start the transmitter
  $PATHSCRIPT"/a.sh" >/dev/null 2>/dev/null &

  # do_status was called here

  # Wait here transmitting until user presses a key

  # Not sure about this
  do_display_on

  # Wait here transmitting until user presses a key
  whiptail --title "$StrStatusTitle" --msgbox "$INFO" 8 78

  # Stop the transmit processes and clean up
  do_stop_transmit
  do_display_off
}

do_stop_transmit()
{
  # Turn the Local Oscillator off
  sudo $PATHRPI"/adf4351" off

  # Kill the key processes as nicely as possible
  sudo killall rpidatv >/dev/null 2>/dev/null
  sudo killall ffmpeg >/dev/null 2>/dev/null
  sudo killall tcanim >/dev/null 2>/dev/null
  sudo killall avc2ts >/dev/null 2>/dev/null

  # Then pause and make sure that avc2ts has really been stopped (needed at high SRs)
  sleep 0.1
  sudo killall -9 avc2ts >/dev/null 2>/dev/null

  # And make sure rpidatv has been stopped (required for brief transmit selections)
  sudo killall -9 rpidatv >/dev/null 2>/dev/null
}

do_display_on()
{
	#tvservice -p
	#sudo chvt 2
	#sudo chvt 1
	v4l2-ctl --overlay=1 >/dev/null 2>/dev/null
}

do_display_off()
{
	v4l2-ctl --overlay=0 >/dev/null 2>/dev/null
	#tvservice -o
}

do_receive_status()
{
	whiptail --title "RECEIVE" --msgbox "$INFO" 8 78
	sudo killall rpidatvgui >/dev/null 2>/dev/null
	sudo killall leandvb >/dev/null 2>/dev/null
#	sudo killall fbi >/dev/null 2>/dev/null
	sudo fbi -T 1 -noverbose -a /home/pi/rpidatv/scripts/images/BATC_Black.png
}

do_receive()
{
	MODE_OUTPUT=$(get_config_var modeoutput $CONFIGFILE)
	sudo killall -9 fbcp >/dev/null 2>/dev/null
	fbcp &
	case "$MODE_OUTPUT" in
	BATC)
	ORGINAL_MODE_INPUT=$(get_config_var modeinput $CONFIGFILE)
	sleep 0.1
	set_config_var modeinput "DESKTOP" $CONFIGFILE
	sleep 0.1
	/home/pi/rpidatv/bin/rpidatvgui 0 1  >/dev/null 2>/dev/null & 
	$PATHSCRIPT"/a.sh" >/dev/null 2>/dev/null &
	do_receive_status
	set_config_var modeinput "$ORGINAL_MODE_INPUT" $CONFIGFILE
	;;
	*)
	/home/pi/rpidatv/bin/rpidatvgui 0 1  >/dev/null 2>/dev/null & 
	do_receive_status;;
	esac
}

do_autostart_setup()
{
    MODE_STARTUP=$(get_config_var startup $CONFIGFILE)

    Radio1=OFF
    Radio2=OFF
    Radio3=OFF
    Radio4=OFF
    Radio5=OFF
    Radio6=OFF
    Radio7=OFF

    case "$MODE_STARTUP" in
        Prompt)
            Radio1=ON;;
        Console)
            Radio2=ON;;
        Display)
            Radio3=ON;;
        Button)
            Radio4=ON;;
        TX_boot)
            Radio5=ON;;
        Display_boot)
            Radio6=ON;;
        Button_boot)
            Radio7=ON;;
        *)
            Radio1=ON;;
    esac

    chstartup=$(whiptail --title "$StrAutostartSetupTitle" --radiolist \
        "$StrAutostartSetupContext" 20 78 8 \
        "Prompt" "$AutostartSetupPrompt" $Radio1 \
        "Console" "$AutostartSetupConsole" $Radio2 \
        "Display" "$AutostartSetupDisplay" $Radio3 \
        "Button" "$AutostartSetupButton" $Radio4 \
        "TX_boot" "$AutostartSetupTX_boot" $Radio5 \
        "Display_boot" "$AutostartSetupDisplay_boot" $Radio6 \
        "Button_boot" "$AutostartSetupButton_boot" $Radio7 \
        3>&2 2>&1 1>&3)

    if [ $? -eq 0 ]; then
        case "$chstartup" in
            Prompt)
                sudo rm /etc/systemd/system/getty.target.wants/getty@tty1.service >/dev/null 2>/dev/null
                cp $PATHCONFIGS"/prompt.bashrc" /home/pi/.bashrc;;
            Console)
                sudo rm /etc/systemd/system/getty.target.wants/getty@tty1.service >/dev/null 2>/dev/null
                cp $PATHCONFIGS"/console.bashrc" /home/pi/.bashrc;;
            Display)
                sudo rm /etc/systemd/system/getty.target.wants/getty@tty1.service >/dev/null 2>/dev/null
                MODE_DISPLAY=$(get_config_var display $CONFIGFILE)
                case "$MODE_DISPLAY" in
                    Waveshare)
                        cp $PATHCONFIGS"/displaywaveshare.bashrc" /home/pi/.bashrc;;
                    *)
                        cp $PATHCONFIGS"/display.bashrc" /home/pi/.bashrc;;
                esac;;
            Button)
                sudo rm /etc/systemd/system/getty.target.wants/getty@tty1.service /dev/null 2>/dev/null
                cp $PATHCONFIGS"/button.bashrc" /home/pi/.bashrc;;
            TX_boot)
                sudo ln -fs /etc/systemd/system/autologin@.service \
/etc/systemd/system/getty.target.wants/getty@tty1.service
                cp $PATHCONFIGS"/console_tx.bashrc" /home/pi/.bashrc;;
            Display_boot)
                sudo ln -fs /etc/systemd/system/autologin@.service \
/etc/systemd/system/getty.target.wants/getty@tty1.service
                MODE_DISPLAY=$(get_config_var display $CONFIGFILE)
                case "$MODE_DISPLAY" in
                    Waveshare)
                        cp $PATHCONFIGS"/displaywaveshare.bashrc" /home/pi/.bashrc >/dev/null 2>/dev/null;;
                    *)
                        cp $PATHCONFIGS"/display.bashrc" /home/pi/.bashrc >/dev/null 2>/dev/null;;
                esac;;
            Button_boot)
                sudo ln -fs /etc/systemd/system/autologin@.service \
/etc/systemd/system/getty.target.wants/getty@tty1.service
                cp $PATHCONFIGS"/button.bashrc" /home/pi/.bashrc;;
        esac
        set_config_var startup "$chstartup" $CONFIGFILE
    fi
}

do_display_setup()
{
MODE_DISPLAY=$(get_config_var display $CONFIGFILE)
case "$MODE_DISPLAY" in

	Tontec35)
	Radio1=ON
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	;;
	HDMITouch)
	Radio1=OFF
	Radio2=ON
	Radio3=OFF
	Radio4=OFF
 	;;
	Waveshare)
	Radio1=OFF
	Radio2=OFF
	Radio3=ON
	Radio4=OFF
	;;
        Console)
        Radio1=OFF
        Radio2=OFF
        Radio3=OFF
        Radio4=ON
        ;;
	*)
	Radio1=ON
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
esac

chdisplay=$(whiptail --title "$StrDisplaySetupTitle" --radiolist \
	"$StrDisplaySetupContext" 20 78 8 \
	"Tontec35" "$DisplaySetupTontec" $Radio1 \
	"HDMITouch" "$DisplaySetupHDMI" $Radio2 \
	"Waveshare" "$DisplaySetupRpiLCD" $Radio3 \
        "Console" "$DisplaySetupConsole" $Radio4 \
 	 3>&2 2>&1 1>&3)

## This section modifies and replaces the end of /boot/config.txt
## to allow (only) the correct LCD drivers to be loaded at next boot

## Set constants for the amendment of /boot/config.txt below

PATHCONFIGS="/home/pi/rpidatv/scripts/configs"  ## Path to config files
lead='^## Begin LCD Driver'               ## Marker for start of inserted text
tail='^## End LCD Driver'                 ## Marker for end of inserted text
CHANGEFILE="/boot/config.txt"             ## File requiring added text
APPENDFILE=$PATHCONFIGS"/lcd_markers.txt" ## File containing both markers
TRANSFILE=$PATHCONFIGS"/transfer.txt"     ## File used for transfer

if [ $? -eq 0 ]; then                     ## If the selection has changed

	grep -q "$lead" "$CHANGEFILE"     ## Is the first marker already present?
	if [ $? -ne 0 ]; then
		sudo bash -c 'cat '$APPENDFILE' >> '$CHANGEFILE' '  ## If not append the markers
	fi

	case "$chdisplay" in              ## Select the correct driver text

		Tontec35)  INSERTFILE=$PATHCONFIGS"/tontec35.txt" ;; ## Message to be added
		HDMITouch) INSERTFILE=$PATHCONFIGS"/hdmitouch.txt" ;;
	        Waveshare) INSERTFILE=$PATHCONFIGS"/waveshare.txt" ;;
		Console)   INSERTFILE=$PATHCONFIGS"/console.txt" ;;

	esac

	## Replace whatever is between the markers with the driver text

	sed -e "/$lead/,/$tail/{ /$lead/{p; r $INSERTFILE
	        }; /$tail/p; d }" $CHANGEFILE >> $TRANSFILE

	sudo cp "$TRANSFILE" "$CHANGEFILE"          ## Copy from the transfer file
	rm $TRANSFILE                               ## Delete the transfer file

	set_config_var display "$chdisplay" $CONFIGFILE
fi
}

do_IP_setup()
{
CURRENTIP=$(ifconfig | grep -Eo 'inet (addr:)?([0-9]*\.){3}[0-9]*' | grep -Eo '([0-9]*\.){3}[0-9]*' | grep -v '127.0.0.1')
whiptail --title "IP" --msgbox "$CURRENTIP" 8 78
}

do_WiFi_setup()
{
$PATHSCRIPT"/wifisetup.sh"
}

do_WiFi_Off()
{
sudo ifconfig wlan0 down                           ## Disable it now
cp $PATHCONFIGS"/text.wifi_off" /home/pi/.wifi_off ## Disable at start-up
}

do_Enable_DigiThin()
{
whiptail --title "Not implemented yet" --msgbox "Not Implemented yet.  Please press enter to continue" 8 78
}

do_EasyCap()
{
    ACINPUT=$(get_config_var analogcaminput $CONFIGFILE)
    ACINPUT=$(whiptail --inputbox "Enter 0 for Composite, 1 for S-VHS, - for not set" 8 78 $ACINPUT --title "SET EASYCAP INPUT NUMBER" 3>&1 1>&2 2>&3)
    if [ $? -eq 0 ]; then
        set_config_var analogcaminput "$ACINPUT" $CONFIGFILE
    fi

    ACSTANDARD=$(get_config_var analogcamstandard $CONFIGFILE)
    ACSTANDARD=$(whiptail --inputbox "Enter 0 for NTSC, 6 for PAL, - for not set" 8 78 $ACSTANDARD --title "SET EASYCAP VIDEO STANDARD" 3>&1 1>&2 2>&3)
    if [ $? -eq 0 ]; then
        set_config_var analogcamstandard "$ACSTANDARD" $CONFIGFILE
    fi
}

do_Update()
{
reset
$PATHSCRIPT"/check_for_update.sh"
}

do_system_setup()
{
menuchoice=$(whiptail --title "$StrSystemTitle" --menu "$StrSystemContext" 16 78 9 \
    "1 Autostart" "$StrAutostartMenu"  \
    "2 Display" "$StrDisplayMenu" \
    "3 IP" "$StrIPMenu" \
    "4 WiFi Set-up" "SSID and password"  \
    "5 WiFi Off" "Turn the WiFi Off" \
    "6 Enable DigiThin" "Not Implemented Yet" \
    "7 Set-up EasyCap" "Set input socket and PAL/NTSC"  \
    "8 Update" "Check for Updated rpidatv Software"  \
    3>&2 2>&1 1>&3)
    case "$menuchoice" in
        1\ *) do_autostart_setup ;;
        2\ *) do_display_setup   ;;
	3\ *) do_IP_setup ;;
        4\ *) do_WiFi_setup ;;
        5\ *) do_WiFi_Off   ;;
        6\ *) do_Enable_DigiThin ;;
        7\ *) do_EasyCap ;;
        8\ *) do_Update ;;
     esac
}

do_language_setup()
{
menuchoice=$(whiptail --title "$StrLanguageTitle" --menu "$StrOutputContext" 16 78 6 \
        "1 French Menus" "Menus Francais"  \
        "2 English Menus" "Change Menus to English" \
        "3 French Keyboard" "$StrKeyboardChange" \
        "4 UK Keyboard" "$StrKeyboardChange" \
        "5 US Keyboard" "$StrKeyboardChange" \
         3>&2 2>&1 1>&3)
        case "$menuchoice" in
            1\ *) set_config_var menulanguage "fr" $CONFIGFILE ;;
            2\ *) set_config_var menulanguage "en" $CONFIGFILE ;;
            3\ *) sudo cp $PATHCONFIGS"/keyfr" /etc/default/keyboard ;;
            4\ *) sudo cp $PATHCONFIGS"/keygb" /etc/default/keyboard ;;
            5\ *) sudo cp $PATHCONFIGS"/keyus" /etc/default/keyboard ;;
        esac

        # Check Language

        MENU_LANG=$(get_config_var menulanguage $CONFIGFILE)

        # Set Language

        if [ "$MENU_LANG" == "en" ]; then
          source $PATHSCRIPT"/langgb.sh"
        else
          source $PATHSCRIPT"/langfr.sh"
        fi
}

do_Exit()
{
exit
}

do_Reboot()
{
sudo reboot now
}

do_Shutdown()
{
sudo shutdown now
}

do_TouchScreen()
{
reset
sudo killall fbcp
fbcp &
~/rpidatv/bin/rpidatvgui 1
}

do_EnableButtonSD()
{
cp $PATHCONFIGS"/text.pi-sdn" ~/.pi-sdn  ## Load it at logon
~/.pi-sdn                                ## Load it now
}

do_DisableButtonSD()
{
rm ~/.pi-sdn             ## Stop it being loaded at log-on
sudo pkill -x pi-sdn     ## kill the current process
} 

do_shutdown_menu()
{
menuchoice=$(whiptail --title "Shutdown Menu" --menu "Select Choice" 16 78 7 \
    "1 Shutdown now" "Immediate Shutdown"  \
    "2 Reboot now" "Immediate reboot" \
    "3 Exit to Linux" "Exit menu to Command Prompt" \
    "4 Restore TouchScreen" "Exit Menu, restart LCD" \
    "5 Button Enable" "Enable Shutdown Button" \
    "6 Button Disable" "Disable Shutdown Button" \
      3>&2 2>&1 1>&3)
    case "$menuchoice" in
        1\ *) do_Shutdown ;;
        2\ *) do_Reboot ;;
        3\ *) do_Exit ;;
        4\ *) do_TouchScreen ;;
        5\ *) do_EnableButtonSD ;;
        6\ *) do_DisableButtonSD ;;
    esac
}

display_splash()
{
sudo killall -9 fbcp >/dev/null 2>/dev/null
fbcp & >/dev/null 2>/dev/null  ## fbcp gets started here and stays running. Not called by a.sh
sudo fbi -T 1 -noverbose -a $PATHSCRIPT"/images/BATC_Black.png" >/dev/null 2>/dev/null
(sleep 1; sudo killall -9 fbi >/dev/null 2>/dev/null) &  ## kill fbi once it has done its work
}

OnStartup()
{
CALL=$(get_config_var call $CONFIGFILE)
MODE_INPUT=$(get_config_var modeinput $CONFIGFILE)
MODE_OUTPUT=$(get_config_var modeoutput $CONFIGFILE)
SYMBOLRATEK=$(get_config_var symbolrate $CONFIGFILE)
FEC=$(get_config_var fec $CONFIGFILE)
PATHTS=$(get_config_var pathmedia $CONFIGFILE)
FREQ_OUTPUT=$(get_config_var freqoutput $CONFIGFILE)
GAIN_OUTPUT=$(get_config_var rfpower $CONFIGFILE)
let FECNUM=FEC
let FECDEN=FEC+1
INFO=$CALL":"$MODE_INPUT"-->"$MODE_OUTPUT"("$SYMBOLRATEK"KSymbol FEC "$FECNUM"/"$FECDEN") on "$FREQ_OUTPUT"Mhz Gain "$GAIN_OUTPUT

do_transmit
}

#********************************************* MAIN MENU *********************************
#************************* Execution of Console Menu starts here *************************

# Check Language
MENU_LANG=$(get_config_var menulanguage $CONFIGFILE)

# Set Language
if [ "$MENU_LANG" == "en" ]; then
  source $PATHSCRIPT"/langgb.sh"
else
  source $PATHSCRIPT"/langfr.sh"
fi

# Display Splash on Touchscreen if fitted
display_splash
status="0"

# Check whether to go straight to transmit or display the menu
if [ "$1" != "menu" ]; then # if tx on boot
  OnStartup               # go straight to transmit
fi

sleep 0.2

# Loop round main menu
while [ "$status" -eq 0 ] 
  do

    # Lookup parameters for Menu Info Message
    CALL=$(get_config_var call $CONFIGFILE)
    MODE_INPUT=$(get_config_var modeinput $CONFIGFILE)
    MODE_OUTPUT=$(get_config_var modeoutput $CONFIGFILE)
    SYMBOLRATEK=$(get_config_var symbolrate $CONFIGFILE)
    FEC=$(get_config_var fec $CONFIGFILE)
    PATHTS=$(get_config_var pathmedia $CONFIGFILE)
    FREQ_OUTPUT=$(get_config_var freqoutput $CONFIGFILE)
    GAIN_OUTPUT=$(get_config_var rfpower $CONFIGFILE)
    let FECNUM=FEC
    let FECDEN=FEC+1
    INFO=$CALL":"$MODE_INPUT"-->"$MODE_OUTPUT"("$SYMBOLRATEK"KSymbol FEC "$FECNUM"/"$FECDEN") on "$FREQ_OUTPUT"Mhz Gain "$GAIN_OUTPUT

    # Display main menu
    menuchoice=$(whiptail --title "$StrMainMenuTitle" --menu "$INFO" 16 82 8 \
      "0 Transmit" "Go to transmit" \
      "1 Source" "$StrMainMenuSource" \
      "2 Output" "$StrMainMenuOutput" \
      "3 Station" "$StrMainMenuCall" \
      "4 Receive" "Receive via rtlsdr" \
      "5 System" "$StrMainMenuSystem" \
      "6 Language" "Set Language and Keyboard" \
      "7 Shutdown" "Shutdown and reboot options" \
      3>&2 2>&1 1>&3)

    # Take Action based on Menu Choice
    case "$menuchoice" in
      0\ *) do_transmit   ;;
      1\ *) do_input_setup   ;;
      2\ *) do_output_setup ;;
      3\ *) do_station_setup ;;
      4\ *) do_receive ;;
      5\ *) do_system_setup ;;
      6\ *) do_language_setup ;;
      7\ *) do_shutdown_menu ;;
         *)

        # Display exit message if user jumps out of menu
        whiptail --title "$StrMainMenuExitTitle" --msgbox "$StrMainMenuExitContext" 8 78

        # Set status to exit
        status=1

        # Sleep while user reads message, then exit
        sleep 1
      exit ;;
    esac
    exitstatus1=$status1
  done
exit

