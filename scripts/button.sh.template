camera=4
file=5
FILES=/media/usb0/videots/
start=1

gpio mode $camera in
gpio mode $file in
gpio mode $camera up
gpio mode $file up


while true; do


	if [ `gpio read $camera` = 0 ]; then echo "camera";
		start=0
		sudo killall UglyDATV
		/home/pi/UglyDATV/a.sh &
		
		sleep 2
	fi

	if [ `gpio read $file` = 0 ] || [ $start = 1 ] ; then echo "file";
		sudo killall UglyDATV
		
		ls "$FILES" | while read f && [ `gpio read $camera` = 1 ]
		do
		  sudo /home/pi/UglyDATV/UglyDATV /media/usb0/videots/"$f" 250 7 0 0 &
		  sleep 1
		  while pidof -x UglyDATV > /dev/null && [ `gpio read $camera` = 1 ] ; do 
		  	sleep 0.1  
		  done
		done
		sleep 2
	fi
sleep 0.1
done

