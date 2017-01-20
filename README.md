![rpidatv banner](/doc/img/spectreiq.jpg)
# rpidatv
**rpidatv** is a digital television transmitter for Raspberry Pi (B,B+,PI2,PI3,Pizero) which outputs directly to GPIO.  This version has been developed for use with an external synthesized oscillator and modulator/filter board. 
*(Created by Evariste Courjaud F5OEO. Code is GPL)*

# Installation for BATC Portsdown Transmitter Version

The preferred installation method only needs a Windows PC connected to the same (internet-connected) network as your Raspberry Pi.  Do not connect a keyboard or HDMI display directly to your Raspberry Pi.

- First download the 25 November 2016 release of Raspbian Jessie Lite on to your Windows PC from here http://downloads.raspberrypi.org/raspbian_lite/images/raspbian_lite-2016-11-29/.  

- Unzip the image and then transfer it to a Micro-SD Card using Win32diskimager https://sourceforge.net/projects/win32diskimager/

- Before you remove the card from your Windows PC, look at the card with windows explorer; the volume should be labelled "boot".  Create a new empty file called ssh in the top-level (root) directory by right-clicking, selecting New, Text Document, and then change the name to ssh (not ssh.txt).  You should get a window warning about changing the filename extension.  Click OK.  If you do not get this warning, you have created a file called ssh.txt and you need to rename it ssh.

- If you have a Pi Camera and/or touchscreen display, you can connect them now.  Power up the Pi with the new card inserted, and a network connection.  Do not connect a keyboard or HDMI display to the Raspberry Pi. 

- Find the IP address of your Raspberry Pi using an IP Scanner (such as Advanced IP Scanner http://filehippo.com/download_advanced_ip_scanner/ for Windows, or Fing on an iPhone) to get the Pi's IP address 

- From your windows PC use Putty (http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html) to log in to the IP address that you noted earlier.  You will get a Security warning the first time you try; this is normal.

- Log in (user: pi/password: raspberry) then cut and paste the following code in, one line at a time:

```sh
wget https://raw.githubusercontent.com/BritishAmateurTelevisionClub/rpidatv/master/install.sh
chmod +x install.sh
./install.sh
```
- If your ISP is Virgin Media and you receive an error after entering the wget line: 'GnuTLS: A TLS fatal alert has been received.', it may be that your ISP is blocking access to GitHub.  If (only if) you get this error with Virgin Media, paste the following command in, and press return.
```sh
sudo sed -i 's/^#name_servers.*/name_servers=8.8.8.8/' /etc/resolvconf.conf
```
Then reboot, and try again.  The comnmand asks your RPi to use Google's DNS, not your ISP's DNS.

- If your ISP is BT, you will need to make sure that "BT Web Protect" is disabled so that you are able to download the software.

- For French menus and keyboard, replace the last line above with 
```sh
./install.sh fr
```

- When it has finished, accept the reboot offered or type "sudo reboot now", log in again and the console menu should be displayed.  If not, you can start the software by typing:

```sh
/home/pi/rpidatv/scripts/menu.sh menu
```

When you reboot and log-in on the computer, the BATC logo should display on the Waveshare touchscreen and the Console Menu should be displayed on the computer.  You do not need to load any touchscreen drivers- if the touchscreen does not work try powering off and on again.  If your touchscreen appears as if the touch sense is 90 degrees out, try selecting the TonTec display in the Setup menu.

I succeeded in generating a direct RF output (from GPIO pin 32) on 437 MHz at 333KS using the on-board camera as the source; it would not work reliably at higher SRs.  
