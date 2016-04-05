#audioconfig=`arecord -l`
audioconfig="arecord -l"
eval $audioconfig | 
 while IFS= read -r line
  do
    case "$line" in
	card* )
	echo YES
	;;
	arecord: )
	echo NO
	;;
	*)
	
	;;
	esac	
  done

detect_audio()
{
devicea="/proc/asound/card1"
if [ -e "$devicea" ]; then
	audio=1
else	
audio=0
fi
echo $result
if [ "$audio" == 1 ]; then
	echo Audio Card present
else
	echo Audio Card Absent
fi
}
detect_audio
