![rpidatv banner](/doc/img/spectreiq.jpg)
# rpidatv
**rpidatv** is a digital television transmitter for Raspberry Pi (B,B+,PI2,PI3,Pizero) which outputs directly to GPIO.  This version has been developed for use with an external synthesized oscillaotor and modulator/filter board. 
*(Created by Evariste Courjaud F5OEO. Code is GPL)*

# Installation for BATC Version

The preferred installation method only needs a Windows PC connected to the same (internet-connected) network as your Raspberry Pi.

- First download the November 2016 release of Raspbian Jessie Lite on to your Windows PC from here http://downloads.raspberrypi.org/raspbian_lite/images/raspbian_lite-2016-11-29/.  

- Unzip the image and then transfer it to a Micro-SD Card using Win32diskimager https://sourceforge.net/projects/win32diskimager/

- Before you remove the card from your Windows PC, look at the card with windows explorer and go to the \boot directory.  Create a new empty file called ssh in the \boot directory by right-clicking, selecting New, Text Document, and then change the name to ssh (not ssh.txt).  You should get a window warning about changing the filename extension.  Click OK.  If you do not get this warning, you have created a file called ssh.txt and you need to rename it ssh.

- If you have a Pi Camera and/or touchscreen display, you can connect them now.  Power up the Pi with the new card inserted, and a network connection.  No keyboard or HDMI display are required. 

- Find the IP address of your Raspberry Pi using an IP Scanner (such as Advanced IP Scanner http://filehippo.com/download_advanced_ip_scanner/ for Windows, or Fing on an iPhone) to get the Pi's IP address 

- From your windows PC use Putty (http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html) to log in to the IP address that you noted earlier.

- Log in (user: pi/password: raspberry) then cut and paste the following code in, one line at a time:

```sh
wget https://raw.githubusercontent.com/davecrump/rpidatv/master/install.sh
chmod +x install.sh
./install.sh
```
- For French menus and keyboard, replace the last line above with 
```sh
./install.sh fr
```

- When it has finished, accept the reboot offered or type "sudo reboot now", log in again and then start the software by typing:

```sh
/home/pi/rpidatv/scripts/menu.sh menu
```

You can now explore the menu options and play.

I succeeded in generating a direct RF output (from GPIO pin 32) on 437 MHz at 333KS using the on-board camera as the source; it would not work reliably at higher SRs.  The big win for me is that I could feed the I and Q signals from pins 32 and 33 directly into the LC filter on my old DigiLite modulator and generate a 2MS QPSK H264 DVB-S signal from the on-board camera.  Some adjustment of the bias is required as the I and Q signals from the Pi are 3.3v, not 5v as provided by the DigiLite encoder.
