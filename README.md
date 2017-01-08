![rpidatv banner](/doc/img/spectreiq.jpg)
# rpidatv
**rpidatv** is a digital television transmitter for Raspberry Pi (B,B+,PI2,PI3,Pizero) which outputs directly to GPIO.  This version has been developed for use with an external synthesized oscillator and modulator/filter board. 
*(Created by Evariste Courjaud F5OEO. Code is GPL)*

# Installation for BATC Portsdown Transmitter Version

The preferred installation method only needs a Windows PC connected to the same (internet-connected) network as your Raspberry Pi.

- First download the November 2016 release of Raspbian Jessie Lite on to your Windows PC from here http://downloads.raspberrypi.org/raspbian_lite/images/raspbian_lite-2016-11-29/.  

- Unzip the image and then transfer it to a Micro-SD Card using Win32diskimager https://sourceforge.net/projects/win32diskimager/

- Before you remove the card from your Windows PC, look at the card with windows explorer and go to the \boot directory.  Create a new empty file called ssh in the \boot directory by right-clicking, selecting New, Text Document, and then change the name to ssh (not ssh.txt).  You should get a window warning about changing the filename extension.  Click OK.  If you do not get this warning, you have created a file called ssh.txt and you need to rename it ssh.

- If you have a Pi Camera and/or touchscreen display, you can connect them now.  Power up the Pi with the new card inserted, and a network connection.  No keyboard or HDMI display are required. 

- Find the IP address of your Raspberry Pi using an IP Scanner (such as Advanced IP Scanner http://filehippo.com/download_advanced_ip_scanner/ for Windows, or Fing on an iPhone) to get the Pi's IP address 

- From your windows PC use Putty (http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html) to log in to the IP address that you noted earlier.

- Log in (user: pi/password: raspberry) then cut and paste the following code in, one line at a time:

```sh
wget https://raw.githubusercontent.com/BritishAmateurTelevisionClub/rpidatv/master/install.sh
chmod +x install.sh
./install.sh
```
- For French menus and keyboard, replace the last line above with 
```sh
./install.sh fr
```

- When it has finished, accept the reboot offered or type "sudo reboot now", log in again and the console menu should be displayed.  If not, you can start the software by typing:

```sh
/home/pi/rpidatv/scripts/menu.sh menu
...

When you reboot and log-in on the computer, the BATC logo should display on the Waveshare touchscreen and the Console Menu should be displayed on the computer.  You do not need to load any touchscreen drivers- iof the touchscreen does not work try powering off and on again.  If your touchscreen appears as if the touch sense is 90 degrees out, try selecting the TonTec display in the Setup menu.

I succeeded in generating a direct RF output (from GPIO pin 32) on 437 MHz at 333KS using the on-board camera as the source; it would not work reliably at higher SRs.  
