#!/bin/bash
    

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
	;;
	CAMMPEG-2) 
	Radio1=OFF
	Radio2=ON
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	;;
	FILETS) 
	Radio1=OFF
	Radio2=OFF
	Radio3=ON
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	;;
	PATERNAUDIO) 
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=ON
	Radio5=OFF
	Radio6=OFF
	Radio7=OFF
	
	;;
	CARRIER) 
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=ON
	Radio6=OFF
	Radio7=OFF
	
	;;
	TESTMODE) 
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=ON
	Radio7=OFF

	;;
	IPTSIN) 
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
	
	;;
	
esac

chinput=$(whiptail --title "$StrInputSetupTitle" --radiolist \
		"$StrInputSetupDescription" 20 78 8 \
		"CAMH264" "$StrInputSetupCAMH264" $Radio1 \
		"CAMMPEG-2" "$StrInputSetupCAMMPEG_2" $Radio2 \
		"FILETS" "$StrInputSetupFILETS" $Radio3\
		"PATERNAUDIO" "$StrInputSetupPATERNAUDIO" $Radio4 \
		"CARRIER" "$StrInputSetupCARRIER" $Radio5 \
		"TESTMODE" "$StrInputSetupTESTMODE" $Radio6 \
		"IPTSIN" "$StrInputSetupIPTSIN" $Radio7 3>&2 2>&1 1>&3)
		if [ $? -eq 0 ]; then
	     	 case "$chinput" in
		    FILETS)
			TSVIDEOFILE=$(get_config_var tsvideofile $CONFIGFILE)
			filename=$TSVIDEOFILE
			FileBrowserTitle=TS:
			Filebrowser "$PATHTS/"
			whiptail --title "$StrInputSetupFILETSName" --msgbox "$filename" 8 44
			set_config_var tsvideofile "$filename" $CONFIGFILE
		    ;;
	            PATERNAUDIO)
			PATERNFILE=$(get_config_var paternfile $CONFIGFILE)
			filename=$PATERNFILE
			FileBrowserTitle=JPEG:
			Filebrowser "$PATHTS/"
			whiptail --title "$StrInputSetupPATERNAUDIOName" --msgbox "$filename" 8 44
			set_config_var paternfile "$filename" $CONFIGFILE
		    ;;
		    IPTSIN)
		    UDPINADDR=$(get_config_var udpinaddr $CONFIGFILE)
		    
		    UDPINADDR=$(whiptail --inputbox "$StrInputSetupIPTSINName" 8 78 $UDPINADDR --title "$StrInputSetupIPTSINTitle" 3>&1 1>&2 2>&3)
		    if [ $? -eq 0 ]; then
			set_config_var udpinaddr "$UDPINADDR" $CONFIGFILE
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
	;;
	QPSKRF)
	Radio1=OFF
	Radio2=ON
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	;;
	BATC)
	Radio1=OFF
	Radio2=OFF
	Radio3=ON
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
	;;
	DIGITHIN)
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=ON
	Radio5=OFF
	Radio6=OFF
	;;
	DTX1)
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=ON
	Radio6=OFF
	;;
	DATVEXPRESS)
	Radio1=OFF
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=ON
	;;
	*)
	Radio1=ON
	Radio2=OFF
	Radio3=OFF
	Radio4=OFF
	Radio5=OFF
	Radio6=OFF
esac

choutput=$(whiptail --title "$StrOutputSetupTitle" --radiolist \
		"$StrOutputSetupContext" 20 78 8 \
		"IQ" "$StrOutputSetupIQ" $Radio1 \
		"QPSKRF" "$StrOutputSetupRF" $Radio2 \
		"BATC" "$StrOutputSetupBATC" $Radio3 \
		"DIGITHIN" "$StrOutputSetupDigithin" $Radio4 \
 		"DTX1" "$StrOutputSetupDTX1" $Radio5 \
		"DATVEXPRESS" "$StrOutputSetupDATVExpress" $Radio6 3>&2 2>&1 1>&3)
if [ $? -eq 0 ]; then		
		
		case "$choutput" in
		    IQ) ;;	
		    QPSKRF)
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
	            BATC)
			BATC_OUTPUT=$(get_config_var batcoutput $CONFIGFILE)
			ADRESS=$(whiptail --inputbox "$StrOutputBATCContext" 8 78 $BATC_OUTPUT --title "$StrOutputBATCTitle" 3>&1 1>&2 2>&3)
			if [ $? -eq 0 ]; then
				set_config_var batcoutput "$ADRESS" $CONFIGFILE	
			fi		    
			;;
		    DIGITHIN)
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

do_output_setup() {
menuchoice=$(whiptail --title "$StrOutputTitle" --menu "$StrOutputContext" 16 78 5 \
        "1 SymbolRate" "$StrOutputSR"  \
        "2 FEC" "$StrOutputFEC" \
	"3 Output mode" "$StrOutputMode" \
	3>&2 2>&1 1>&3)
	case "$menuchoice" in
            1\ *) do_symbolrate_setup ;;
            2\ *) do_fec_setup   ;;
	    3\ *) do_output_setup_mode ;;
        esac
}


do_transmit() 
{
		
	$PATHSCRIPT"/a.sh" >/dev/null 2>/dev/null &
	
}

do_stop_transmit()
{
	sudo killall rpidatv >/dev/null 2>/dev/null
	sudo killall ffmpeg >/dev/null 2>/dev/null

}

do_display_on()
{
	#tvservice -p
	#sudo chvt 2
	#sudo chvt 1
	v4l2-ctl --overlay=1
}

do_display_off()
{
	v4l2-ctl --overlay=0
	#tvservice -o
}

do_status()
{
	do_display_on
	whiptail --title "$StrStatusTitle" --msgbox "$INFO" 8 78
	do_stop_transmit
	do_display_off
}

#********************************************* MAIN MENU **************************************************
status="0"

#$PATHRPI"/rpibutton.sh" &
sleep 0.2

    while [ "$status" -eq 0 ] 
    do
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
INFO=$CALL":"$MODE_INPUT"-->"$MODE_OUTPUT"("$SYMBOLRATEK"KSymbol FEC "$FECNUM"/"$FECDEN") sur "$FREQ_OUTPUT"Mhz Gain "$GAIN_OUTPUT



do_transmit
do_status
#do_display_on
#"1 Transmission" "Demarre la transmission"\
 menuchoice=$(whiptail --title "$StrMainMenuTitle" --menu "$INFO" 16 82 5 \
        "1 Source" "$StrMainMenuSource" \
	"2 Sortie" "$StrMainMenuOutput" \
	"3 Station" "$StrMainMenuCall" \
	3>&2 2>&1 1>&3)
      
        case "$menuchoice" in
            1\ *) do_input_setup   ;;
	    2\ *) do_output_setup ;;
   	    3\ *) do_station_setup ;;
            *)
		
		 whiptail --title "$StrMainMenuExitTitle" --msgbox "$StrMainMenuExitContext" 8 78
                status=1
		
		kill -1 $(pidof -x frmenu.sh) >/dev/null 2>/dev/null
		kill -1 $(pidof -x gbmenu.sh) >/dev/null 2>/dev/null 
		sleep 1
                exit
		;;
        esac
        exitstatus1=$status1
    done
else
    whiptail --title "Testing" --msgbox "Bye" 8 78
exit

