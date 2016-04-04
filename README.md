![rpidatv banner](/doc/img/spectreiq.jpg)

**rpidatv** is a digital television transmitter for Raspberry Pi (B,B+ and PI2) which output directly to GPIO. 
*(Created by Evariste Courjaud F5OEO. Code is GPL)*

<h1> rpidatv Transmitter installation </h1>
```
sudo apt-get update && 
sudo apt-get install rpi-update && 
sudo rpi-update && 
sudo apt-get install apt-transport-https && 
sudo apt-get install git && 
git clone git://github.com/f5oeo/rpidatv && 
cd rpidatv/src && 
make -j 4
```

<h1> leandvb rtlsdr receiver installation </h1>
`./install.sh`   
This install leandvb : a simple dvb-s receiver with rtl-sdr usb dongle
([leandvb dvb-s receiver](http://www.pabr.org/radio/leandvb/leandvb.en.html))    
You can run the receiver : `./scripts/leandvb2video.sh`

<h1>Hardware</h1>
Plug a wire on GPIO 12, means Pin 32 of the GPIO header ([header P1](http://elinux.org/RPi_Low-level_peripherals#General_Purpose_Input.2FOutput_.28GPIO.29)): this act as the antenna. Length depend on transmit frequency, but with few centimeters it works for local testing.

<h1>Short manual</h1>
<h2> General </h2>
rpidatv is located in /bin folder

```
rpidatv -1.3.0
Usage:
rpidatv -i File Input -s Symbolrate -c Fec [-o OutputMode] [-f frequency output]  [-l] [-p Power] [-h]   
	-i            path to Transport File Input    
	-s            SymbolRate in KS (125-4000)    
	-c            Fec : 1/2 or 3/4 or 5/6 or 7/8    
	-m            OutputMode   
		{RF(Modulate QSK in RF need -f option to set frequency)}   
		{IQ(Output QPSK I/Q}   
		{PARALLEL(Output parallel (DTX1,MINIMOD..)}   
	{IQWITHCLK(Output I/Q with CLK (F5LGJ)}   
	{DIGITHIN (Output I/Q for Digithin)}   
	-f 	      Frequency to output in RF Mode in MHZ   
	-l            loop file input   
	-p 	      Power on output 1..7   
	-x 	      GPIO Pin output for I or RF {12,18,40}   
	-y	      GPIO Pin output for Q {13,19,41,45}   
	-h            help (print this help).   
```
Example : `sudo ./rpidatv -i sample.ts -s 250 -c 1/2 -o RF -f 437.5 -l`   

<h2> Minimal graphical menu </h2>
You can launch a graphical menu located in /scripts folder   
`./frmenu.sh` for french language   
`./gbmenu.sh` for english language   


