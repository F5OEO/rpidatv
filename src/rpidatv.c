/*
 
   <rpidatv is a software which use the GPIO of Raspberry Pi to transmit Digital Television over HF>

    Copyright (C) 2015  Evariste COURJAUD F5OEO (evaristec@gmail.com)

    Transmitting on HF band is surely not permitted without license (Hamradio for example).
    Usage of this software is not the responsability of the author.
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   
   Thanks to first test of RF with Pifm by Oliver Mattos and Oskar Weigl 	
   INSPIRED BY THE IMPLEMENTATION OF PIFMDMA by Richard Hirst <richardghirst@gmail.com>  December 2012
  
   Thanks to Brian Jordan G4EWJ for Channel modulation implementation (dvbsenco)
 */
 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "fec100.h"
#include "mailbox.h"
#include <getopt.h>
#include <termios.h>		//Used for UART
#include "rpigpio.h"
#include "rpidma.h"
#include <pthread.h>

#include <sys/prctl.h>

extern void 	dvbsenco_init	(void) ;
extern uchar*	dvbsenco	(uchar*) ;	

extern void energy (uchar* input,uchar *output) ;
extern void reed (uchar *input188) ;
extern uchar*	interleave 	(uchar* packetin) ; 

#define PROGRAM_VERSION "2.0.0"

//Minimum Time in us to sleep
#define KERNEL_GRANULARITY 20000 

#define SCHED_PRIORITY 30 //Linux scheduler priority. Higher = more realtime

#define WITH_MEMORY_BUFFER




#define PLLFREQ_PCM		1000000000	// PLLD is running at 500MHz
#define PLL_PCM 		0x6

//#define PLLFREQ_PWM             1000000000	//PLLC = 1GHZ , 1.2GHZ ON PIZERO ! But Unstable -> Go back to PLL_D
//#define PLL_PWM			0x5

#define PLLFREQ_PWM             1000000000	//PLLC = 1GHZ , 1.2GHZ ON PIZERO ! But Unstable -> Go back to PLL_D
#define PLL_PWM			0x6


#define CARRIERFREQ		100000000	// Carrier frequency is 100MHz


//F5OEO Variable
uint32_t TabIQ[4]={0xCCCCCCCC,0x66666666,0x99999999,0x33333333};//0,-pi/2,pi/2,pi
uint32_t TabIQTest[4]={0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC,0xCCCCCCCC};//0,-pi/2,pi/2,pi
uint32_t TabIQTestI[4]={0x00110011,0x11111111,0x11111111,0x11111111};
uint32_t TabIQTestQ[4]={0x01010101,0x00000000,0x00000000,0x00000000};
int PinOutput[2]={18,19}; //Output signal I/Q on GPIO pin number 					  

//uint32_t TabIQ[4]={0x00000000,0xFFFFFFFF,0x00000000,0xFFFFFFFF};
int SymbolRate=0;
int FEC=1;
double TuneFrequency=62500000;
unsigned char FreqDivider=2;
//End F5OEO


int uart0_filestream = -1; // Handle to Serial Port for Digithin
char DigithinCommand[]="*A";

pthread_t th1; // Thread filling BigBUffer
char EndOfApp=0;
unsigned char Loop=0;
char *FileName;
int fdts; //Handle in Transport Stream File
static void
udelay(int us)
{
	struct timespec ts = { 0, us * 1000 };

	nanosleep(&ts, NULL);
}

static void
terminate(int dummy)
{
	#ifdef WITH_MEMORY_BUFFER
	//pthread_cancel(th1);
	EndOfApp=1;
	 pthread_join(th1, NULL);
	#endif
	close(fdts);
	if (dma_reg) {
		dma_reg[DMA_CS+DMA_CHANNEL*0x40] = BCM2708_DMA_INT | BCM2708_DMA_END;
		udelay(100);
		dma_reg[DMA_CS+DMA_CHANNEL*0x40] = BCM2708_DMA_RESET;
		udelay(100);
		//printf("Reset DMA Done\n");
	clk_reg[GPCLK_CNTL] = 0x5A << 24  | 0 << 9 | 1 << 4 | 6; //NO MASH !!!
	udelay(500);
	gpio_reg[GPFSEL0] = (gpio_reg[GPFSEL0] & ~(7 << 12)) | (0 << 12); //DISABLE CLOCK - In case used by digilite
	clk_reg[PWMCLK_CNTL] = 0x5A000006 | (0 << 9) ;
	udelay(500);	
	clk_reg[PCMCLK_CNTL] = 0x5A000006;	
	udelay(500);
	//printf("Resetpcm Done\n");
	pwm_reg[PWM_DMAC] = 0;
	udelay(100);
	pwm_reg[PWM_CTL] = PWMCTL_CLRF;
	udelay(100);
	//printf("Reset pwm Done\n");
	}
	if (mbox.virt_addr != NULL) {
		unmapmem(mbox.virt_addr, NUM_PAGES * PAGE_SIZE);
		//printf("Unmapmem Done\n");
		mem_unlock(mbox.handle, mbox.mem_ref);
		//printf("Unmaplock Done\n");
		mem_free(mbox.handle, mbox.mem_ref);
		//printf("Unmapfree Done\n");
	}
	
	//munmap(virtbase,NUM_PAGES * PAGE_SIZE); 
	printf("END OF RPIDATV\n");
	exit(1);
}

static void
fatal(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	terminate(0);
}





#define DATA_FILE_SIZE 4080 // Not used anymore

void setSchedPriority(int priority) {
//In order to get the best timing at a decent queue size, we want the kernel to avoid interrupting us for long durations.
//This is done by giving our process a high priority. Note, must run as super-user for this to work.
struct sched_param sp;
sp.sched_priority=priority;
int ret;
if ((ret = pthread_setschedparam(pthread_self(), SCHED_RR, &sp))) {
printf("Warning: pthread_setschedparam (increase thread priority) returned non-zero: %i\n", ret);
}
/*
 if ((ret = pthread_setschedparam(th1, SCHED_RR, &sp))) {
printf("Warning: pthread_setschedparam (increase thread priority) returned non-zero: %i\n", ret);
}
*/
}

/*
void shift(char* window, int len) {
  int i;
  for (i = 1; i < len; ++i) {
    window[i-1] = window[i];
  }
  window[len-1] = 0;
}

*/
void SetUglyFrequency(double Frequency)
{	
	 int harmonic;
	 double FreqFound;
	uint16_t FreqFractionnal=0; 
			
			
			for(harmonic=1;(harmonic<41);harmonic+=2)
			{
				//printf("->%lf harmonic %d\n",(Frequency/(double)harmonic),harmonic);
				if((Frequency/(double)harmonic)<=(double)PLLFREQ_PWM/8.0) break;
			}
			
			harmonic-=2;
			//do
			{
			harmonic+=2;
			FreqFound = (double) (Frequency*4.0/(double)harmonic);
			FreqDivider=(int) ((double)PLLFREQ_PWM/FreqFound);
			FreqFractionnal=4096.0 * (((double)PLLFREQ_PWM/FreqFound)-FreqDivider);
			//printf("Ecart = %lf\n", (PLLFREQ/(double)FreqDivider)-(PLLFREQ/(double)(FreqDivider+1.0)));
			}
			//while((PLLFREQ/(double)FreqDivider)-(PLLFREQ/(double)(FreqDivider+1.0))>25e6); // To Avoir 25MHZ of MASH : seems not a limit, removed
			clk_reg[PWMCLK_DIV] = 0x5A000000 | (FreqDivider<<12) | FreqFractionnal;
			printf("Tuning on %lf MHZ (harmonic %d):DIV%d/FRAC%d\n",1e-6*(double)PLLFREQ_PWM*(double)harmonic/(4.0*(FreqDivider+(double) FreqFractionnal/4096.0)),harmonic,FreqDivider,FreqFractionnal);

}
//************************************ INIT MODE UGLY ***********************************************

int InitUgly()
{
	char MASH=1;
		SetUglyFrequency(TuneFrequency);
			//gpioSetMode(18, 2); /* set to ALT5, PWM1 : RF */
			if(PinOutput[0]==18) {gpioSetMode(18, 2);printf("\n Using GPIO 18");}; //ALT 5
			if(PinOutput[0]==12) {gpioSetMode(12, 4);printf("\n Using GPIO 12");} //ALT 0
			if(PinOutput[0]==40) gpioSetMode(40, 4); //ALT 0

pwm_reg[PWM_CTL] = 0;
//ALWAYS USE MASH
		{
			//printf("MASH ENABLE\n");
			clk_reg[PWMCLK_CNTL] = 0x5A000000 | (MASH << 9)|PLL_PWM ; //1<<9
			//clk_reg[PWMCLK_CNTL] = 0x5A000005 | (1 << 9) ;
		}
	
		udelay(300);
	
		
		clk_reg[PWMCLK_CNTL]= 0x5A000010 | (MASH << 9) | PLL_PWM;

		pwm_reg[PWM_RNG1] = 32;// 32 Mandatory for Serial Mode without gap
	udelay(100);
	pwm_reg[PWM_RNG2] = 32;// 32 Mandatory for Serial Mode without gap
	udelay(100);
	pwm_reg[PWM_DMAC] = PWMDMAC_ENAB | PWMDMAC_THRSHLD;
	udelay(100);
	pwm_reg[PWM_CTL] = PWMCTL_CLRF;
	udelay(100);

	//------------------- Init PCM ------------------
	pcm_reg[PCM_CS_A] = 1;				// Disable Rx+Tx, Enable PCM block
		udelay(100);
		clk_reg[PCMCLK_CNTL] = 0x5A000000|PLL_PWM;		// Source=PLLD (500MHz) //Seems Both should be on same PLL
		udelay(1000);
		
		int NbStep;
		if(SymbolRate>=250)
		{
			clk_reg[PCMCLK_DIV] = 0x5A000000 | (8<<12);	// Set pcm div to 2, giving 250MHz step
			NbStep= PLLFREQ_PCM/(8*SymbolRate*1000) -1;
		}
		else
		{
			int prescale=4*(250/SymbolRate);
			clk_reg[PCMCLK_DIV] = 0x5A000000 | ((prescale*2*2)<<12);	// Set pcm div to 2, giving 250MHz step
			NbStep= PLLFREQ_PCM/(4*prescale*SymbolRate*1000) -1;
			printf("Low SymbolRate\n");
		}
		
		pcm_reg[PCM_TXC_A] = 0<<31 | 1<<30 | 0<<20 | 0<<16; // 1 channel, 8 bits
		udelay(100);
		
		
		printf("Nb PCM STEP (<1000):%d\n",NbStep);
		pcm_reg[PCM_MODE_A] = NbStep<<10; // SHOULD NOT EXCEED 1000 !!!
		udelay(100);
		pcm_reg[PCM_CS_A] |= 1<<4 | 1<<3;		// Clear FIFOs
		udelay(100);
		pcm_reg[PCM_DREQ_A] = 64<<24 | /*64<<8 |*/ 64<<8 ;		//TX Fifo PCM=64 DMA Req when one slot is free?
		udelay(100);
		pcm_reg[PCM_CS_A] |= 1<<9;			// Enable DMA
		udelay(1000);
		clk_reg[PCMCLK_CNTL] = 0x5A000010 |PLL_PWM;		// Source=PLLD and enable
	
	
		//printf("Playing File =%s at %d KSymbol FEC=%d ",argv[1],PLLFREQ_PCM/((NbStep+1)*4L),abs(FEC));

		// ========================== INIT DMA ================================================
		ctl = (struct control_data_s *)virtbase;
		dma_cb_t *cbp = ctl->cb;

	

		
		uint32_t phys_pwm_fifo_addr = 0x7e20c000 + 0x18;//PWM
		uint32_t phys_fifo_addr = (0x00203000 | 0x7e000000) + 0x04; //PCM
		//uint32_t dummy_gpio = 0x7e20b000;
		int samplecnt;

		for (samplecnt = 0; samplecnt < NUM_SAMPLES; samplecnt++) {
			
		
			// Write a frequency sample

			cbp->info = BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP |BCM2708_DMA_D_DREQ /*| BCM2708_DMA_PER_MAP(5)*/;
			cbp->src = mem_virt_to_phys(ctl->sample + samplecnt);
			cbp->dst = phys_pwm_fifo_addr;
			cbp->length = 4;
			cbp->stride = 0;
			cbp->next = mem_virt_to_phys(cbp + 1);
			//printf("cbp : sample %x src %x dest %x next %x\n",ctl->sample + i,cbp->src,cbp->dst,cbp->next);
			cbp++;
			
					
			// Delay
			
			cbp->info =  /*BCM2708_DMA_SRC_IGNOR  |*/ BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP | BCM2708_DMA_D_DREQ  | BCM2708_DMA_PER_MAP(2);
			cbp->src = mem_virt_to_phys(virtbase);
			cbp->dst = phys_fifo_addr;//Delay with PCM
			cbp->length = 4;
			cbp->stride = 0;
			cbp->next = mem_virt_to_phys(cbp + 1);
			cbp++;
			
		
		}
					
		cbp--;
		cbp->next = mem_virt_to_phys(virtbase);

	// ----------------------------- END DMA ------------------------------------
	pwm_reg[PWM_CTL] =  PWMCTL_USEF1| PWMCTL_MODE1| PWMCTL_PWEN1|PWMCTL_RPTL1 ; //PWM0
	usleep(100);
	pcm_reg[PCM_CS_A] |= 1<<2; //START TX PCM
		
 	return 1;		
}


//************************************ INIT MODE IQ ***********************************************

int InitIQ(int DigithinMode)
{
	//PIN FOR I
	if(PinOutput[0]==18) gpioSetMode(18, 2); //ALT 5
	if(PinOutput[0]==12) gpioSetMode(12, 4); //ALT 0
	if(PinOutput[0]==40) gpioSetMode(40, 4); //ALT 0

	//PIN FOR Q
	if(PinOutput[1]==13) gpioSetMode(13, 4); //ALT 0
	if(PinOutput[1]==19) gpioSetMode(19, 2); //ALT 5
	if(PinOutput[1]==41) gpioSetMode(41, 4); //ALT 0
	if(PinOutput[1]==45) gpioSetMode(45, 4); //ALT 0

	/*  CAM-GPIO IS ON 41 on B+ : DON'T USE*/	
	
	// REMOVE AUTODETECTION PIN FROM PI MODEL - USE PIN MAPPING
	/*
	if (model<3) //ONLY ON B : FOR SOLDERING on PCB near audio output, ON B+ IT CRASH CAM
	{
		gpioSetMode(40, 4); // set to ALT0, PWM1 DIGILITE Model B
		gpioSetMode(41, 4); // set to ALT0, PWM2 DIGILITE Model B !!! CAM-GPIO IS ON 41 on B+ 	
	}
	*/
	//unsigned int SRClock=PLLFREQ_PCM/(SymbolRate*1000);
	
	unsigned int SRClock=PLLFREQ_PCM/(1000*SymbolRate);
	//unsigned int SRClockPCM=(PLLFREQ_PCM/(SymbolRate*1000*64))*64;
	
	//SymbolRate = PLLFREQ/(SRClockPCM*1000);

	
	uint32_t DigiThin_ClockBySymbol=0;
	// CLK_DIGITHIN 500MHZ(PLLD)/4MHZ = 125
	#define CLK_4MHZ 125
	if(DigithinMode==1)
	{
				
	
		// GPIO4 needs to be ALT FUNC 0 to otuput the clock
		gpio_reg[GPFSEL0] = (gpio_reg[GPFSEL0] & ~(7 << 12)) | (4 << 12); //ENABLE CLOCK - In case used by digilite
		usleep(1000);
		clk_reg[GPCLK_CNTL] = 0x5A << 24 |0<<4 | PLL_PCM;
		usleep(1000);
		clk_reg[GPCLK_DIV] = 0x5A << 24 | (CLK_4MHZ<<12) ; //CLK FREQ = 4MHZ: Fixed for Digithin
		usleep(100);
		DigiThin_ClockBySymbol=( PLLFREQ_PCM/(CLK_4MHZ));
		//SRClock=DigiThin_ClockBySymbol*CLK_4MHZ;
		printf("Digithin Clock at 4MHZ:%ld clock by Symbol (SR=%d)\n",(long int)DigiThin_ClockBySymbol,4000000/(SymbolRate*1000));
		udelay(500);
		clk_reg[GPCLK_CNTL] = 0x5A << 24  | 0 << 9 | 1 << 4 | PLL_PCM; //NO MASH !!!
		udelay(500);

	
		uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
		if (uart0_filestream == -1)
		{
			//ERROR - CAN'T OPEN SERIAL PORT
			printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
		}
	
		//CONFIGURE THE UART
		//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	
		struct termios options;
		tcgetattr(uart0_filestream, &options);
		options.c_cflag = B57600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
		options.c_iflag = IGNPAR;
		options.c_oflag = 0;
		options.c_lflag = 0;
		tcflush(uart0_filestream, TCIFLUSH);
		tcsetattr(uart0_filestream, TCSANOW, &options);
		
		//GPIO7 (pin 26) is connected to the reset pin of the dsPIC, so it should always be high. It may never be necessary to reset the dsPIC, but it gives the option.

		//   DigiThin UART commands (CRLF not required)
		//  *0         reboot
		//   *1         test mode, no mod
		//   *2         test mode, LSB
		//   *3         test mode, USB
		//   *4         test mode, in phase
		//   *5         test mode, ALT1
		//   *6         test mode, ALT2
		//   *7         test mode, ALT4
		//  
		//   4MHz reference clock
		//
		//   *A         running, 12 cycles per bit, nominal 333kS
		//   *B         running, 15 cycles per bit, nominal 266kS
		//   *C         running, 18 cycles per bit, nominal 222kS
		//   *D         running, 21 cycles per bit, nominal 190kS
		//   *E         running, 24 cycles per bit, nominal 166kS
		//   *F         running, 27 cycles per bit, nominal 148kS
		//   *G         running, 30 cycles per bit, nominal 133kS
		//   *H         running, 33 cycles per bit, nominal 121kS
		//
		//   *?         request status

		switch(SymbolRate)
		{
			case 333:DigithinCommand[1]='A';SymbolRate=333333;break;
			case 266:DigithinCommand[1]='B';SymbolRate=266666;break;
			case 222:DigithinCommand[1]='C';SymbolRate=222222;break;
			case 190:DigithinCommand[1]='D';SymbolRate=190476;break;
			case 166:DigithinCommand[1]='E';SymbolRate=166666;break;
			case 148:DigithinCommand[1]='F';SymbolRate=148148;break;
			case 133:DigithinCommand[1]='G';SymbolRate=133333;break;
			case 121:DigithinCommand[1]='H';SymbolRate=121121;break;
			default:DigithinCommand[1]='A';SymbolRate=333333;break;
			
		}
		SRClock=PLLFREQ_PCM/(SymbolRate);
		SymbolRate=SymbolRate/1000;

	}
	else
	{	
			//#define VCO_MODE
		        
			
			
			
			#ifdef DIGILITE_CLOCK_MODE
			printf("\n ******** DIGILITE CLOCK MODE*********** \n");
			printf("SRClok=%d SYmbolRate=%dKSymb\n",SRClock,500000/SRClock);
			gpio_reg[GPFSEL0] = (gpio_reg[GPFSEL0] & ~(7 << 12)) | (4 << 12); //ENABLE CLOCK - In case used by digilite
			clk_reg[GPCLK_CNTL] = 0x5A << 24 |0<<4 | PLL_PCM;
			udelay(2000);
			//clk_reg[GPCLK_DIV] = 0x5A << 24 | ((SRClock)<<12) ; //CLK FREQ = SR for Digilite
			clk_reg[GPCLK_DIV] = 0x5A << 24 | ((SRClock>>2)<<12) ; //CLK FREQ = SR for F5LGJ
			udelay(500);
			clk_reg[GPCLK_CNTL] = 0x5A << 24  | 1 << 9 | 1 << 4 | PLL_PCM; //NO MASH !!!
			udelay(500);
			#endif
			#ifdef VCO_MODE
			printf("\n ******** VCO CLOCK MODE*********** \n");

			uint32_t FreqVCO=437000000;
			uint32_t FreqDivider,FreqFractionnal;

			FreqDivider=(int) ((double)PLLFREQ_PWM/FreqVCO);
			FreqFractionnal=4096.0 * (((double)PLLFREQ_PWM/FreqVCO)-FreqDivider);
			FreqDivider=FreqDivider%4096;
			FreqFractionnal=FreqFractionnal%4096;
			printf("Freq Divider %d Freq Frac %d\n",FreqDivider,FreqFractionnal);

			gpio_reg[GPFSEL0] = (gpio_reg[GPFSEL0] & ~(7 << 12)) | (4 << 12); //ENABLE CLOCK - In case used by digilite
			clk_reg[GPCLK_CNTL] = 0x5A << 24 |0<<4 | PLL_PWM;
			udelay(2000);
			clk_reg[GPCLK_DIV] = 0x5A << 24 | (FreqDivider<<12) | FreqFractionnal ; //CLK FREQ = SR for Digilite
			udelay(500);
			clk_reg[GPCLK_CNTL] = 0x5A << 24  | 1 << 9 | 1 << 4 | PLL_PWM; //MASH !!!
			udelay(500);
			#endif
		
	}

	pwm_reg[PWM_CTL] = 0;
	clk_reg[PWMCLK_CNTL] = 0x5A000000 | (0 << 9) |PLL_PCM ;
	udelay(300);
	clk_reg[PWMCLK_DIV] = 0x5A000000 | ((SRClock)<<12); //*2: FIXME : Because SRClock is normaly based on 500Mhz not 1GH
	udelay(300);
	clk_reg[PWMCLK_CNTL] = 0x5A000010 | (0 << 9) | PLL_PCM;
	pwm_reg[PWM_RNG1] = 32;// 32 Mandatory for Serial Mode without gap
	udelay(100);
	pwm_reg[PWM_RNG2] = 32;// 32 Mandatory for Serial Mode without gap

	pwm_reg[PWM_DMAC] = PWMDMAC_ENAB | PWMDMAC_THRSHLD;
	udelay(100);
	pwm_reg[PWM_CTL] = PWMCTL_CLRF;
	udelay(100);

	printf("Real SR = %d KSymbol / Clock Divider =%d \n",PLLFREQ_PCM/(SRClock*1000),SRClock);
	//printf("Playing File =%s at %d KSymbol FEC=%d  ",argv[1],PLLFREQ_PCM/SRClock/1000,abs(FEC));

	// --------------------- INIT DMA IQ ------------------------------
	ctl = (struct control_data_s *)virtbase;
	dma_cb_t *cbp = ctl->cb;
	

	uint32_t phys_pwm_fifo_addr = 0x7e20c000 + 0x18;//PWM
	int samplecnt;
	NUM_SAMPLES = NUM_SAMPLES_MAX/2; // Minize the buffer in IQ Mode
			for (samplecnt = 0; samplecnt < NUM_SAMPLES; samplecnt++) {
		
				// Write a PWM sample

				cbp->info = BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP|BCM2708_DMA_D_DREQ | BCM2708_DMA_PER_MAP(5);
				cbp->src = mem_virt_to_phys(ctl->sample + samplecnt);
				cbp->dst = phys_pwm_fifo_addr;
				cbp->length = 4;
				cbp->stride = 0;
				cbp->next = mem_virt_to_phys(cbp + 1);
				//printf("cbp : sample %x src %x dest %x next %x\n",ctl->sample + i,cbp->src,cbp->dst,cbp->next);
				cbp++;

			}
			cbp--;
			cbp->next = mem_virt_to_phys(virtbase);

	// ------------------------------ END DMA INIT ---------------------------------
	pwm_reg[PWM_CTL] =  PWMCTL_USEF2|PWMCTL_PWEN2|PWMCTL_MODE2|PWMCTL_USEF1| PWMCTL_MODE1| PWMCTL_PWEN1; //PWM0
	return 1;
}
 

int InitDTX1()
{
		int i;

		gpioSetMode(2, 1); // D0 GPIO 2 : Header 3
		gpioSetMode(3, 1); // D1 GPIO 3 : Header 5
		gpioSetMode(4, 1); // D2 GPIO 4 : Header 7
		gpioSetMode(14, 1); // D3 GPIO 14 : Header 8
		gpioSetMode(15, 1); // D4 GPIO 15 : Header 10
		gpioSetMode(17, 1); // D5 GPIO 17 : Header 11
		gpioSetMode(18, 1); // D6 GPIO 18 : Header 12
		gpioSetMode(27, 1); // D7 GPIO 27 : Header 13
		
		gpioSetMode(22, 1); // CLK GPIO 22 : Header 15
		//gpioSetMode(23, 1); // TPCLK GPIO 23 : Header 16
		gpioSetMode(24, 1); // TPCLK GPIO 24 : Header 18
		
		/*
		for(i=0;i<40000;i++)
		{
			gpio_reg[0x1C/4]=1<<22;
			usleep(100);
			gpio_reg[0x28/4]=1<<22;
			usleep(100);
		}		
		*/

		uint32_t TSRate=SymbolRate*125/*1000/8*/*2*FEC*188/(204*(FEC+1L));
		//TSRate=100000;//TEST
		uint32_t SRTSClock=PLLFREQ_PCM/(TSRate*2*32); //32 mais SET/CLR *2
		// TEST ********************
		//SRTSClock=1000/32;
		// **************
		printf("DTX1 : TS Rate = %lu ClockDiv=%lu\n",(unsigned long int)TSRate,(unsigned long int)SRTSClock);

		pwm_reg[PWM_CTL] = 0;
		clk_reg[PWMCLK_CNTL] = 0x5A000000 | (0 << 9) |PLL_PCM ;
		udelay(600);
		clk_reg[PWMCLK_DIV] = 0x5A000000 | ((SRTSClock)<<12); //*2: FIXME : Because SRClock is normaly based on 500Mhz not 1GH
		udelay(300);
		clk_reg[PWMCLK_CNTL] = 0x5A000010 | (0 << 9) | PLL_PCM;
		pwm_reg[PWM_RNG1] = 32;// 32 Mandatory for Serial Mode without gap
		udelay(100);
		pwm_reg[PWM_RNG2] = 32;// 32 Mandatory for Serial Mode without gap

		pwm_reg[PWM_DMAC] = PWMDMAC_ENAB | PWMDMAC_THRSHLD;
		udelay(100);
		pwm_reg[PWM_CTL] = PWMCTL_CLRF;
		udelay(100);

		pwm_reg[PWM_CTL] =  /*PWMCTL_USEF2|PWMCTL_PWEN2|PWMCTL_MODE2|*/PWMCTL_USEF1| PWMCTL_MODE1| PWMCTL_PWEN1; //PWM

		//------------------------- INIT DMA DTX1 --------------------
		ctl = (struct control_data_s *)virtbase;
		dma_cb_t *cbp = ctl->cb;
		uint32_t phys_pwm_fifo_addr = 0x7e20c000 + 0x18;//PWM
		uint32_t dummy_gpio = 0x7e20b000;
		NUM_SAMPLES = NUM_SAMPLES_MAX/2; // Minize the buffer in DTX1 Mode
			for (i = 0; i < NUM_SAMPLES; i++)
			 {
				ctl->sample[i] = 0;	// Silence
		
				if((i%2)==0)
				{			
					cbp->info = BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP;
					cbp->src = mem_virt_to_phys(ctl->sample + i );//Surement WORD1=CLR
					cbp->dst = 0x7E200028; //CLEAR BIT
					cbp->length = 4;
					cbp->stride = 0;
					cbp->next = mem_virt_to_phys(cbp + 1);
					//printf("cbp CLEAR : sample %x src %x dest %x next %x\n",ctl->sample + i,cbp->src,cbp->dst,cbp->next);
					cbp++;
			
/*
					cbp->info = BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP|BCM2708_DMA_D_DREQ | BCM2708_DMA_PER_MAP(5);
					cbp->src = mem_virt_to_phys(virtbase);
					cbp->dst = phys_pwm_fifo_addr;
					cbp->length = 4;
					cbp->stride = 0;
					cbp->next = mem_virt_to_phys(cbp + 1);
					//printf("cbp : sample %x src %x dest %x next %x\n",ctl->sample + i,cbp->src,cbp->dst,cbp->next);
					cbp++;*/

				
					cbp->info = BCM2708_DMA_NO_WIDE_BURSTS| BCM2708_DMA_WAIT_RESP;
					cbp->src = mem_virt_to_phys(virtbase);
					cbp->dst = dummy_gpio ;
					cbp->length = 4;
					cbp->stride = 0;
					cbp->next = mem_virt_to_phys(cbp + 1);
					//printf("cbp DUMMY: sample %x src %x dest %x next %x\n",ctl->sample + i,cbp->src,cbp->dst,cbp->next);
					cbp++;

				}
				else
				{
					cbp->info = BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP;
					cbp->src = mem_virt_to_phys(ctl->sample + i );//Surement WORD1=CLR
					cbp->dst = 0x7E20001C; //SET BIT
					cbp->length = 4;
					cbp->stride = 0;
					cbp->next = mem_virt_to_phys(cbp + 1);
					//printf("cbp SET: sample %x src %x dest %x next %x\n",ctl->sample + i,cbp->src,cbp->dst,cbp->next);
					cbp++;

					cbp->info = BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP|BCM2708_DMA_D_DREQ | BCM2708_DMA_PER_MAP(5);
					cbp->src = mem_virt_to_phys(virtbase);
					cbp->dst = phys_pwm_fifo_addr;
					cbp->length = 4;
					cbp->stride = 0;
					cbp->next = mem_virt_to_phys(cbp + 1);
					//printf("cbp PWM: sample %x src %x dest %x next %x\n",ctl->sample + i,cbp->src,cbp->dst,cbp->next);
					cbp++;
				}

			}
			cbp--;
			cbp->next = mem_virt_to_phys(virtbase);
	return 1;
}


#define BIG_BUFFER_SIZE 18800
#define BURST_MEM_SIZE 188
typedef struct circular_buffer
{
  unsigned char *buffer;
  volatile unsigned int head;
  volatile unsigned int tail;
  pthread_mutex_t lock;
}ring_buffer;

ring_buffer my_circular_buffer;


int BufferAvailable()
{
	int Available= my_circular_buffer.head-my_circular_buffer.tail;
	if(Available<0) Available+=BIG_BUFFER_SIZE;
	return Available;
}

void store_in_buffer(unsigned char data)
{
  
  while(((unsigned int)(my_circular_buffer.head + 1) % BIG_BUFFER_SIZE)==my_circular_buffer.tail)
  	 usleep(50000);

  unsigned int next = (unsigned long)(my_circular_buffer.head + 1) % BIG_BUFFER_SIZE;
  
  my_circular_buffer.buffer[my_circular_buffer.head] = data;
  my_circular_buffer.head = next;
  
	
}

void store_in_buffer_1880(unsigned char *data)
{
  
  //while(((unsigned int)(my_circular_buffer.head + 18800) % BIG_BUFFER_SIZE)==my_circular_buffer.tail)
  //	 usleep(50000);
  
  memcpy(my_circular_buffer.buffer+my_circular_buffer.head,data,BURST_MEM_SIZE);
  unsigned int next = (unsigned long)(my_circular_buffer.head + BURST_MEM_SIZE) % BIG_BUFFER_SIZE;
  my_circular_buffer.head = next;
  
	
}

char read_from_buffer()
{
  //printf("#\n");
  // if the head isn't ahead of the tail, we don't have any characters

  while (my_circular_buffer.head == my_circular_buffer.tail)
  {
	 sleep(0);//sched_yield();
  }
  
    char data = my_circular_buffer.buffer[my_circular_buffer.tail];
    my_circular_buffer.tail = (unsigned int)(my_circular_buffer.tail + 1) % BIG_BUFFER_SIZE;
	
    return data;
  
}

void read_from_buffer_188(unsigned char *Dest)
{
	while(BufferAvailable()<188) usleep(0);
	//pthread_mutex_lock(&my_circular_buffer.lock);
	memcpy(Dest,my_circular_buffer.buffer+my_circular_buffer.tail,188);
	my_circular_buffer.tail = (unsigned int)(my_circular_buffer.tail + 188) % BIG_BUFFER_SIZE;
	//pthread_mutex_unlock(&my_circular_buffer.lock);
}

void *FillBigBuffer (void * arg)
{
 
  char name[16];
  static uchar	buff188[BURST_MEM_SIZE];
  static int ByteRead=0;
  static int TotalByteRead=0;
  static int NbWrite=0;
  memset(name, '\0', sizeof(name));
  strcpy(name,"BIGBUFFER");
  //pthread_setname_np(pthread_self(), name);
  prctl(PR_SET_NAME, name, 0, 0, 0);
 pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
 pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  while(EndOfApp==0)
  {
	TotalByteRead=0;
	//usleep(200);
	do
	{
		
		ByteRead=read(fdts,buff188+TotalByteRead,BURST_MEM_SIZE-TotalByteRead);
		
		if(ByteRead<=0)
		{
			if(Loop==1)
			{
				close(fdts);
				fdts = open(FileName, 'r');
				TotalByteRead=0;
				printf("Loop %s\n",FileName);
			}
			else
			{
				printf("Filling BUffer : EOF\n");
				//terminate(0);
				close(fdts);
				 return 0; // END OF FILE
			}
		}
		else
		{
			TotalByteRead+=ByteRead;
		
			//usleep(2000);
		}
													
	}
	while (TotalByteRead<BURST_MEM_SIZE); // Read should be around 20us

	while(((BIG_BUFFER_SIZE-BufferAvailable())<=BURST_MEM_SIZE))
	{
		usleep(1000);
		
	}
	
	        //printf("Lock BigBuffer\n");
		
		pthread_mutex_lock(&my_circular_buffer.lock);
		
		 store_in_buffer_1880(buff188);
		//printf("#");
		/*
		for(NbWrite=0;NbWrite<1880;NbWrite++)
		{
			store_in_buffer(buff188[NbWrite]);
		}
		*/
		pthread_mutex_unlock(&my_circular_buffer.lock);
		// printf("UNLock BigBuffer\n");
	
  }
  pthread_exit (0);
}
void print_usage()
{

fprintf(stderr,\
"\nrpidatv -%s\n\
Usage:\nrpidatv -i File Input -s Symbolrate -c Fec [-o OutputMode] [-f frequency output]  [-l] [-p Power] [-h] \n\
-i            path to Transport File Input \n\
-s            SymbolRate in KS (125-4000) \n\
-c            Fec : 1/2 or 3/4 or 5/6 or 7/8 \n\
-m            OutputMode\n\
	      {RF(Modulate QSK in RF need -f option to set frequency)}\n\
              {IQ(Output QPSK I/Q}\n\
              {PARALLEL(Output parallel (DTX1,MINIMOD..)}\n\
       	      {IQWITHCLK(Output I/Q with CLK (F5LGJ)}\n\
	      {DIGITHIN (Output I/Q for Digithin)}\n\
-f 	      Frequency to output in RF Mode in MHZ\n\
-l            loop file input\n\
-p 	      Power on output 1..7\n\
-x 	      GPIO Pin output for I or RF {12,18,40}\n\
-y	      GPIO Pin output for Q {13,19,41,45}\n\
-h            help (print this help).\n\
Example : sudo ./rpidatv -i sample.ts -s 250 -c 1/2 -o RF -f 437.5 -l\n\
\n",\
PROGRAM_VERSION);

} /* end function print_usage */

int
main(int argc, char **argv)
{
	int i;
	//char pagemap_fn[64];
	
	//unsigned char *data;
	int OutputPower=5;
	int ModeIQ=0;
	
	
	int DigithinMode=0; //1 allow Clock output + SerialOutput

	//int ModeDTX1=0;
	
	my_circular_buffer.buffer=malloc(BIG_BUFFER_SIZE);
	my_circular_buffer.head=0;
	my_circular_buffer.tail=0;

	
	
	fprintf(stdout,"RPIDATV Version %s (F5OEO Evariste) on ",__DATE__);
	


// Nearly there.. open the .ts file specified on the cmdline
	fdts = 0;
	int a;
	int anyargs = 0;
	while(1)
	{
	a = getopt(argc, argv, "i:s:c:hlf:m:p:x:y:");
	
	if(a == -1) 
	{
		if(anyargs) break;
		else a='h'; //print usage and exit
	}
	anyargs = 1;	

	switch(a)
		{
		case 'i': // InputFile
			FileName=optarg;
			fdts = open(FileName, 'r');
			break;
		case 's': // SymbolRate
			SymbolRate = atoi(optarg);
			break;
		case 'c': // FEC
			if(strcmp("1/2",optarg)==0) FEC=1;
			if(strcmp("2/3",optarg)==0) FEC=2;	
			if(strcmp("3/4",optarg)==0) FEC=3;
			if(strcmp("5/6",optarg)==0) FEC=5;
			if(strcmp("7/8",optarg)==0) FEC=7;
			if(strcmp("0",optarg)==0) FEC=0;//CARRIER MODE
			break;
		case 'h': // help
			print_usage();
			terminate(0);
			break;
		case 'l': // loop mode
			Loop = 1;
			break;
		case 'f': // Frequency (Mode RF)
			TuneFrequency = atof(optarg)*1000000;
			break;
		case 'm': // Output mode
			if(strcmp("IQ",optarg)==0) ModeIQ=1;
			if(strcmp("RF",optarg)==0) ModeIQ=0;;	
			if(strcmp("PARALLEL",optarg)==0) ModeIQ=2;
			if(strcmp("IQWITHCLK",optarg)==0) ModeIQ=1;
			if(strcmp("DIGITHIN",optarg)==0) {ModeIQ=1;DigithinMode=1;};
			
			break;
		case 'p': // Power
			OutputPower= atoi(optarg);
			break;
		case 'x': // Pin mapping GPIO I or RF
			PinOutput[0]=atoi(optarg);
			break;
		case 'y': // Pin mapping GPIO Q
			PinOutput[1]=atoi(optarg);
			break;

		case -1:
        	break;
		case '?':
			if (isprint(optopt) )
 				{
 				fprintf(stderr, "rpidatv: unknown option `-%c'.\n", optopt);
 				}
			else
				{
				fprintf(stderr, "rpidatv: unknown option character `\\x%x'.\n", optopt);
				}
			print_usage();

			exit(1);
			break;			
		default:
			print_usage();
			exit(1);
			break;
		}/* end switch a */
	}/* end while getopt() */



	/*   SEARCH FOR NEAR FREQUENCY */
	
	

	
	 

	//data=malloc(DATA_FILE_SIZE);
	
	

	
	dvbsenco_init() ;
	
	if(abs(FEC)>0)
		viterbi_init(abs(FEC));
	else
		viterbi_init(1); 
	
	//setSchedPriority(SCHED_PRIORITY); //Seems to help a lot ---------> REMOVED SINCE MEMORY BUFFERING
	

	

	// Calculate the frequency control word
	// The fractional part is stored in the lower 12 bits
	//freq_ctl = ((float)(PLLFREQ / CARRIERFREQ)) * ( 1 << 12 );
		
	InitGpio();

	InitDma(terminate);

	if(ModeIQ==0)
		InitUgly();
	if(ModeIQ==1)
		InitIQ(DigithinMode);
	if(ModeIQ==2)
		InitDTX1();
	usleep(500);
	//REMOVE TO TEST
	
	pad_gpios_reg[PADS_GPIO_0] = 0x5a000000 + (OutputPower&0x7) + (1<<4) + (0<<3); // Set output power for I/Q GPIO18/GPIO19
	


	//int NbStep; 
	
	// Initialise the DMA
	
	dma_reg[DMA_CS+DMA_CHANNEL*0x40] = BCM2708_DMA_RESET;
	udelay(1000);
	dma_reg[DMA_CS+DMA_CHANNEL*0x40] = BCM2708_DMA_INT | BCM2708_DMA_END;
	udelay(100);

	

	dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40] = mem_virt_to_phys(ctl->cb);
	//printf("DMA CONBLOCK : Virt %x-> Phys %x\n",ctl->cb,mem_virt_to_phys(ctl->cb));

	

	// LET'S START DMA 
	udelay(100);
	dma_reg[DMA_DEBUG+DMA_CHANNEL*0x40] = 7; // clear debug error flags
	udelay(100);
	// START DMA
	//dma_reg[DMA_CS+DMA_CHANNEL*0x40] = 0x10880001;	// go, mid priority, wait for outstanding writes :7 Seems Max Priority

        udelay(200);
	uint32_t last_cb = (uint32_t)ctl->cb;
	uint32_t NbSymbol = 0;

	//terminate(0); // Before going to DMA

	

	//int PosInByte=0;
// 4000 samples * (32/2IQ) / SR/2 (half full) -> 4000*16/1E6Hz= 64 ms
	int TimeToSleep=((8000)*1000)/SymbolRate;
	//printf("TimeToSleep = %d\n",TimeToSleep);
//TRY REMOVING DELAY
//printf("Removing Delay");
static volatile uint32_t cur_cb;
int last_sample;
int this_sample; 
int free_slots;
//int SumDelay=0;
	
	if(Loop==1) printf("(looping)\n"); else printf("\n");
	printf("TS Bitrate should be %lu bit/s\n",(uint32_t)SymbolRate*1000*2*188*abs((long)FEC)/(204L*(abs(FEC)+1L)));
	
	
long int start_time;
static long time_difference=0;
struct timespec gettime_now;

	

// CALIBRATION ************* 


//#define CALIBRATION 1
#ifdef CALIBRATION 
int DiffCB;
dma_reg[DMA_CS+DMA_CHANNEL*0x40] = 0x10880001;
for(i=0;i<30;i++)
{	
		volatile uint32_t phys_cur_cb,phys_cur_cb2;
		int try;
		//for(try=0;try<10;try++) {if((phys_cur_cb=dma_reg[DMA_CONBLK_AD])==0) break;usleep(100);}
		phys_cur_cb=dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40];
		cur_cb = mem_phys_to_virt(phys_cur_cb);
		
	      
		clock_gettime(CLOCK_REALTIME, &gettime_now);

		start_time = gettime_now.tv_nsec;
		usleep(10000);
		phys_cur_cb2=dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40];
		
	    	DiffCB = mem_phys_to_virt(phys_cur_cb2)-cur_cb;
		clock_gettime(CLOCK_REALTIME, &gettime_now);
		time_difference = gettime_now.tv_nsec - start_time;
		  printf("cb_phys =%lx cb_phy2=%lx :Diff %lx \n",phys_cur_cb,phys_cur_cb2,abs(phys_cur_cb2-phys_cur_cb));
		DiffCB=abs(phys_cur_cb-phys_cur_cb2);
if(ModeIQ==0)
		printf("Calibrate NbCB=%d Time=%d : %f\n",DiffCB/(sizeof(dma_cb_t) * 2),time_difference,1000000.0*DiffCB/(sizeof(dma_cb_t) * 2)/(float)time_difference);
else
	printf("Calibrate NbCB=%d Time=%d : %f\n",DiffCB/(sizeof(dma_cb_t) ),time_difference,1000000.0*16*DiffCB/(sizeof(dma_cb_t) )/(float)time_difference);
}
#endif 


//uchar PacketNULL[188];
//PacketNULL[0]=0x47;
//PacketNULL[1]=0x00;
//for(i=2;i<188;i++) PacketNULL[i]=0xFF;


if(ModeIQ==0) //UGLY
{
	cur_cb = (uint32_t)virtbase;
	
	last_cb=(uint32_t)virtbase + 2*204*8* /*NUM_SAMPLES/8* */ sizeof(dma_cb_t) *2  ;//AGAIN *2 TO AVOID ISSUE ? 
}
if(ModeIQ==1)
{
	cur_cb = (uint32_t)virtbase;
	last_cb=(uint32_t)virtbase + 204*8*2*sizeof(dma_cb_t) ;//*2 Should be "paire" else not aligned to I/Q
}
if(ModeIQ==2)
{
	cur_cb = (uint32_t)virtbase;
	last_cb=(uint32_t)virtbase + 4*188*8*sizeof(dma_cb_t)*2 ;//*2 Should be "paire" else not aligned to I/Q
}

dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40]=mem_virt_to_phys(virtbase);

// Pour que Freeslot<0 au 1er coup
//dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40] = mem_virt_to_phys(virtbase);
unsigned char Init=1;
//unsigned char Resync=0;
//unsigned char Start=1;
clock_gettime(CLOCK_REALTIME, &gettime_now);
printf("%ld:%ld : Fulling buffer \n",gettime_now.tv_sec,gettime_now.tv_nsec);




static uint32_t MaxToGetBuffer=0; // Time of the circular buffer : depend on SIZE and SymbolRate
uint64_t TSRate=SymbolRate*FEC*188*1000.0/(4*204*(FEC+1L));
if(ModeIQ==0)
	MaxToGetBuffer=((NUM_SAMPLES-(204*2*4))*1000)/(SymbolRate*2); // ONLY UGLY
if(ModeIQ==1)
	MaxToGetBuffer=((NUM_SAMPLES-(204*2*4))*1000*16*2)/(SymbolRate*2);//
if(ModeIQ==2)
{
	
	MaxToGetBuffer=((NUM_SAMPLES-(188*8*4))*1000)/(TSRate);//
}

#ifdef WITH_MEMORY_BUFFER
// ------------------- START THE BUFFER FILLING THREAD ---------------
{
	//uint64_t TSRate=SymbolRate*FEC*188*1000.0/(4*204*(FEC+1L));
	pthread_attr_t attr;
	pthread_mutex_init (&my_circular_buffer.lock, NULL);
	printf("TSrate in byte =%llu\n",TSRate); 		
	pthread_attr_init (&attr);
	//pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	//pthread_create (&th1,&attr, &FillBigBuffer,NULL);
	pthread_create (&th1,NULL, &FillBigBuffer,NULL);
	pthread_attr_destroy (&attr);
	
	
	while(BufferAvailable()<(TSRate/10)&&(BufferAvailable()<(BIG_BUFFER_SIZE*8/10))) // 1/10 SECOND BUFFERING DEPEND ON SYMBOLRATE OR 80% BUFFERSIZE
	{
		printf("Init Filling Memory buffer %d\n",BufferAvailable());
		//printf(".");
		 usleep(500);
	}
	/*
	int NbByteInitRead=0;
	pthread_mutex_lock(&my_circular_buffer.lock);
	while((BufferAvailable()>TSRate)&&(NbByteInitRead%188!=0))
	{
	 read_from_buffer();
	 NbByteInitRead++;
	}
	pthread_mutex_unlock(&my_circular_buffer.lock);
	*/
	printf("End Init Filling Memory buffer %d\n",BufferAvailable());
}
#endif
// -----------------------------------------------------------------

for (;;) 
	{
		//********************************* MODE UGLY **************************************
		if(ModeIQ==0)
		{ 	
			static int StatusCompteur=0;
			cur_cb = mem_phys_to_virt(dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40]);
			this_sample = (cur_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t) * 2);
			last_sample = (last_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t) * 2);
			free_slots = this_sample - last_sample;
			if (free_slots < 0) // WARNING : ORIGINAL CODE WAS < strictly
			{
				free_slots += NUM_SAMPLES;
				
				
			}
			else
			{
				//if(ctl->OverflowControl[0]==ctl->OverflowStatus[1])
				{
					//printf("Cptoverflow = %d %d \n",ctl->OverflowControl[0],ctl->OverflowStatus[0]);
					
				}
			}

			
			if(Init==0)
			{
				TimeToSleep=((NUM_SAMPLES-free_slots-204*2*4)*1000)/((float)SymbolRate*2);//-22000; // 22ms de Switch process
				//TimeToSleep=15000+KERNEL_GRANULARITY;
				//TimeToSleep=25000;
			}
			else
				TimeToSleep=30000;
			
	
			//printf("cur_cb %lx FreeSlots = %d Time to sleep=%d\n",cur_cb,free_slots,TimeToSleep);
			//printf("Buffer Available=%d\n",BufferAvailable());
			
			clock_gettime(CLOCK_REALTIME, &gettime_now);
			start_time = gettime_now.tv_nsec;		
			if(TimeToSleep>=(2200+KERNEL_GRANULARITY)) // 2ms : Time to process File/Canal Coding
			{
				
				udelay(TimeToSleep-(2200+KERNEL_GRANULARITY));
				TimeToSleep=0;
			}
	
			else
			{
				
				//udelay(TimeToSleep);
				sched_yield();
				//TimeToSleep=0;
				if(free_slots>(NUM_SAMPLES*9/10))
					 printf("Buffer nearly empty...%d/%d\n",free_slots,NUM_SAMPLES);
			}
			
			
			static int free_slots_now;
			cur_cb = mem_phys_to_virt(dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40]);
			this_sample = (cur_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t) * 2);
			last_sample = (last_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t) * 2);
			free_slots_now = this_sample - last_sample;
			if (free_slots_now < 0) // WARNING : ORIGINAL CODE WAS < strictly
				free_slots_now += NUM_SAMPLES;
			
			clock_gettime(CLOCK_REALTIME, &gettime_now);
			time_difference = gettime_now.tv_nsec - start_time;
			if(time_difference<0) time_difference+=1E9;
			
			if(StatusCompteur%10==0)
			{ 
				
				//SetUglyFrequency(TuneFrequency);
				//TuneFrequency+=5000.0;
				
				//printf("Memavailable %d/%d FreeSlot=%d/%d Bitrate : %f\n",BufferAvailable(),BIG_BUFFER_SIZE,free_slots_now,NUM_SAMPLES,(1000000.0*(free_slots_now-free_slots))/(float)time_difference);
			}
			StatusCompteur++;
			//printf(" DiffTime = %ld FreeSlot=%d Bitrate : %f\n",time_difference,free_slots_now-free_slots,(1000000.0*(free_slots_now-free_slots))/(float)time_difference);		

			free_slots=free_slots_now;
			// FIX IT : Max(freeslot et Numsample/8)
			if((Init==1)&&(free_slots <= 204*2*4 /*NUM_SAMPLES/8*/))
			{
				printf("%ld:%ld : End of Fulling buffer \n",gettime_now.tv_sec,gettime_now.tv_nsec);
				dma_reg[DMA_CS+DMA_CHANNEL*0x40] = 0x10880001;	// go, mid priority, wait for outstanding writes :7 Seems Max Priority
				Init=0;
				
			}
			clock_gettime(CLOCK_REALTIME, &gettime_now);
			start_time = gettime_now.tv_nsec;
			
		
			//printf("Process LOCK\n");
			#ifdef WITH_MEMORY_BUFFER
			pthread_mutex_lock(&my_circular_buffer.lock);
			
			#endif
			
			while ((free_slots>204*2*4)&&(BufferAvailable()>188*2)) //204Bytes*2(IQ)*4 paires/octet
			{
				
				static uint32_t BuffAligned[256];
				static uchar	*buff=(uchar *)BuffAligned ;
				
				static uchar *pRS204;
				static int NbIQOutput;

				static unsigned char BuffIQ[204*2];
				static int k=0;
				
				
				
				
				if((abs(FEC)>0)) //NORMAL MODE , 0 : CARRIER MODE
				{
					
					if(FEC>0)
					{
						#ifndef WITH_MEMORY_BUFFER
						static int ByteRead=0;
						if ((ByteRead=read(fdts,buff,188))!=188) // Read should be around 20us
						{
							// printf("END OF FILE OR packet is not 188 long %d\n",data_len);
							if(Loop==1)
								{
									close(fdts);
									 fdts = open(argv[1], 'r');
								}
								else
								{	
									while(ByteRead!=188)
										ByteRead+=read(fdts,buff+ByteRead,188-ByteRead);
									//printf("End of processing file\n");
									//terminate(0);
								}						
						}
						#else
						int ii;
						while(BufferAvailable()<188) 
						{
							//printf("!");
							sleep(0);
						}
						//pthread_mutex_lock(&my_circular_buffer.lock);
						read_from_buffer_188(buff);
						//for(ii=0;ii<188;ii++) buff[ii]=read_from_buffer();
						//pthread_mutex_unlock(&my_circular_buffer.lock);
						//TotalByteRead+=188;
						#endif
					}
					else
					{
						buff[0]=0x47;
						memset(buff+1,0,187);
					}
					

					if(buff[0]!=0x47) printf("SYNC ERROR \n");
			
					// Time to compute is beetween 200 and 1000 us
					
					energy (buff,buff) ;
					 
					
					

					
					if(FEC<0)
					{	
						//unsigned char SyncByte=buff[0];
						
						memset(buff+1,0,187);
					}
					reed (buff) ;
					pRS204=	interleave(buff) ;
					
					NbIQOutput=viterbi (pRS204,BuffIQ);
				}
				else // TEST MODE (CARRIER)
				{
					
					NbIQOutput=204;
				}

			    	for(k=0;k<NbIQOutput;k++)
				{
					for(i=3;i>=0;i--)
					{
						if(abs(FEC)>0)
						{
							if(FEC>0)
							{	
								ctl->sample[last_sample++]=TabIQ[(BuffIQ[k]>>(i*2))&0x3];
							}
							
							else
							{
								if((k<=32)) ctl->sample[last_sample++]=TabIQ[(BuffIQ[k]>>(i*2))&0x3];
								else
								{
								
										ctl->sample[last_sample++]=TabIQ[i];
								}	 
							}
						}
						else
						{
							
							ctl->sample[last_sample++]=TabIQTest[0];
						}	
						if (last_sample == NUM_SAMPLES)	last_sample = 0;
						NbSymbol++;
					}
					free_slots -= 4;
					
				}
			/*
			static int free_slot_process;
			cur_cb = mem_phys_to_virt(dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40]);
			this_sample = (cur_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t) * 2);
			
			free_slot_process = this_sample - last_sample;
			if (free_slot_process < 0) // WARNING : ORIGINAL CODE WAS < strictly
				free_slot_process += NUM_SAMPLES;
			if(free_slot_process>10000) 	printf("FreeSlotP=%d\n",free_slot_process);		
			*/
			}
			//printf("Process UNLOCK\n");
			#ifdef WITH_MEMORY_BUFFER
			pthread_mutex_unlock(&my_circular_buffer.lock);
			#endif
			clock_gettime(CLOCK_REALTIME, &gettime_now);
			time_difference = gettime_now.tv_nsec - start_time;
			if(time_difference<0) time_difference+=1E9;
			//printf("Time to Process file =%ld ByteRead = %ld BitRateTS= %ld\n",time_difference,TotalByteRead,TotalByteRead*8*1000000L/(time_difference));
			if((Init==0)&&(FEC>0)&&(time_difference>MaxToGetBuffer*1000))
				{
					dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40]=mem_virt_to_phys(virtbase);
					last_cb=(uint32_t)virtbase + 2*204*8* /*NUM_SAMPLES/8* */ sizeof(dma_cb_t) *2  ;
					 printf("File to slow !! wait for %ldus\n",(long int)time_difference/1000);
					 
					
				}
			else
				last_cb = (uint32_t)virtbase + last_sample * sizeof(dma_cb_t) * 2;
			
		}
		//********************************* MODE IQ **************************************
		if(ModeIQ==1)
		{
			
			cur_cb = mem_phys_to_virt(dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40]);
			this_sample = (cur_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t) );
			last_sample = (last_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t) );
		    	free_slots = this_sample - last_sample;
			if (free_slots < 0) // WARNING : ORIGINAL CODE WAS < strictly
				free_slots += NUM_SAMPLES;
						
				TimeToSleep=((NUM_SAMPLES-free_slots-204*2*4)*1000*16)/((float)SymbolRate*2); // 22ms de Switch process

				//printf("FreeSlots = %d Time to sleep=%d\n",free_slots,TimeToSleep);

				clock_gettime(CLOCK_REALTIME, &gettime_now);
				start_time = gettime_now.tv_nsec;		
				if(TimeToSleep>=(2000+KERNEL_GRANULARITY)){
					if(TimeToSleep>=150000) TimeToSleep=150000; //Digithin should be every 250ms
					udelay(TimeToSleep-(2000+KERNEL_GRANULARITY));
				}
				else
				{
					//usleep(0);//20 ms mini !!
					if(free_slots>(NUM_SAMPLES*9/10))
						 printf("Buffer nearly empty...%d/%d\n",free_slots,NUM_SAMPLES);
				}
				//printf("FreeSlots = %d Time to sleep=%d\n",free_slots,TimeToSleep);
				
				if(DigithinMode==1)
				{
					
					static unsigned long int serial_start_time;
					clock_gettime(CLOCK_REALTIME, &gettime_now);
					if(Init==1) serial_start_time=gettime_now.tv_nsec;
					static long int DiffSerialTime;
					DiffSerialTime=gettime_now.tv_nsec-serial_start_time;
					 if(DiffSerialTime<0) DiffSerialTime+=1E9;
					//printf("Time = %lu\n",DiffSerialTime/1000000L);
					//if((DiffSerialTime)/1000000L>200) // Every 500 ms
						{
						if (uart0_filestream != -1)
						{
							//printf("Serial %s\n",DigithinCommand);
							int count = write(uart0_filestream, DigithinCommand,sizeof(DigithinCommand));
							//Filestream, bytes to write, number of bytes to write
							if (count < 0)
							{
								printf("UART TX error\n");
							}
							else
							{
								//printf("Serial %d\n",count);
							}
						}
					
						serial_start_time = gettime_now.tv_nsec;
					}
				}
				static int free_slots_now; 
				cur_cb = mem_phys_to_virt(dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40]);
				this_sample = (cur_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t) );
				last_sample = (last_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t));
				free_slots_now = this_sample - last_sample;
				if (free_slots_now < 0) // WARNING : ORIGINAL CODE WAS < strictly
					free_slots_now += NUM_SAMPLES;
			
				clock_gettime(CLOCK_REALTIME, &gettime_now);
				time_difference = gettime_now.tv_nsec - start_time;
				if(time_difference<0) time_difference+=1E9;
				
				//printf("DiffTime = %ld FreeSlot=%ld Bitrate : %f\n",time_difference,free_slots_now-free_slots,(1000000.0*(free_slots_now-free_slots)*16.0)/(float)time_difference);
				free_slots=free_slots_now;


				if((Init==1)&&(free_slots <= (204*8*2)/*NUM_SAMPLES/8*/))
				{
					printf("%ld:%ld : End of Fulling buffer \n",gettime_now.tv_sec,gettime_now.tv_nsec);
					dma_reg[DMA_CS+DMA_CHANNEL*0x40] = 0x10880001;	// go, mid priority, wait for outstanding writes :7 Seems Max Priority
					Init=0;
					sleep(0);
				}
			//printf("Free Slots = %d\n",free_slots);
			//if (time_difference/1000>TimeToSleep*2) printf("Time %d\n",time_difference/1000); 
		
			// FEC 1/2 : 204*2*1/2/8(8=4octets de I et 4 octets de Q) : il faut divisible par 8
			//FEC 1/2 : il faut 2*204 pour divisble par 8
			//FEC 2/3 : 204 OK
			//FEC 3/4 : il faut 4*204
			//FEC 5/6 : 340 Octets : il faut 2
			//FEC 7/8 : 357 8 
						
			clock_gettime(CLOCK_REALTIME, &gettime_now);
			start_time = gettime_now.tv_nsec;
			while ((free_slots > (204*8*2))&&(BufferAvailable()>188*8)) //204Bytes*2(IQ)/32
			{
				
				//static uint32_t BuffAligned[256];
				//static uchar	*buff=(uchar *)BuffAligned ;

				static uchar buff[208];

				static uchar *pRS204;
				static int NbIQOutput=0;
				static  unsigned char BuffIQ[204*8*2];
				static int k=0;
				
				
				//static int data_len_total;
				//data_len_total=0;

			
				NbIQOutput=0;
				// Time to compute is beetween 200 and 1000 us
				
				if(abs(FEC)>0) //NORMAL MODE , 0 : CARRIER MODE
				{
					for(i=0;i<8;i++)
					{
					
						if(FEC>0)
						{
							/*
							static int ByteRead=0;
							if ((ByteRead=read(fdts,buff,188))!=188) // Read should be around 20us
							{
								printf("END OF FILE OR packet is not 188 long %d\n",data_len);
								if(Loop==1)
								{
									close(fdts);
									 fdts = open(argv[1], 'r');
								}
								else
								{	
									while(ByteRead!=188)
										ByteRead+=read(fdts,buff+ByteRead,188-ByteRead);
									//printf("End of processing file\n");
									//terminate(0);
								}	
								
							}
							*/
							int ii;
							pthread_mutex_lock(&my_circular_buffer.lock);
							for(ii=0;ii<188;ii++) buff[ii]=read_from_buffer();
							pthread_mutex_unlock(&my_circular_buffer.lock);
							
						}
						else
						{
							buff[0]=0x47;
							memset(buff+1,0,187);
						}
						
						
						if(buff[0]!=0x47) printf("SYNC 188 ERROR \n");
							
						energy (buff,buff) ;
					 		
							
						if(FEC<0)
						{	
							memset(buff+1,0x183,187);
							//memset(pRS204+1,0,203);
						}
						reed (buff) ;
						pRS204=	interleave(buff) ;
						//pRS204 = dvbsenco (buff) ;
						NbIQOutput+=viterbi (pRS204,BuffIQ+NbIQOutput);
						//printf("NbIQ=%d\n",NbIQOutput);	
						if((NbIQOutput%8)==0) break;
						
						
					}
				}
				else
				{
					NbIQOutput=408;
				}
				
				//printf("NbIQoutput=%d %d\n",NbIQOutput,free_slots);
				if((NbIQOutput%8)!=0) printf("ERROR ALIGNED \n");
				
			    	for(k=0;k<NbIQOutput;k+=8)
				{
					static uint32_t I32,Q32;
					I32=0L;
					Q32=0L;
					for (i=0;i<4;i++)
					{
						I32|=((BuffIQ[k]&(1<<(i*2+1)))>>(i*2+1))<<(28+i);
						I32|=((BuffIQ[k+1]&(1<<(i*2+1)))>>(i*2+1))<<(24+i);
						I32|=((BuffIQ[k+2]&(1<<(i*2+1)))>>(i*2+1))<<(20+i);
						I32|=((BuffIQ[k+3]&(1<<(i*2+1)))>>(i*2+1))<<(16+i);
						I32|=((BuffIQ[k+4]&(1<<(i*2+1)))>>(i*2+1))<<(12+i);
						I32|=((BuffIQ[k+5]&(1<<(i*2+1)))>>(i*2+1))<<(8+i);
						I32|=((BuffIQ[k+6]&(1<<(i*2+1)))>>(i*2+1))<<(4+i);
						I32|=((BuffIQ[k+7]&(1<<(i*2+1)))>>(i*2+1))<<i;
				
					
						Q32|=((BuffIQ[k]&(1<<(i*2)))>>(i*2))<<(28+i);
						Q32|=((BuffIQ[k+1]&(1<<(i*2)))>>(i*2))<<(24+i);
						Q32|=((BuffIQ[k+2]&(1<<(i*2)))>>(i*2))<<(20+i);
						Q32|=((BuffIQ[k+3]&(1<<(i*2)))>>(i*2))<<(16+i);
						Q32|=((BuffIQ[k+4]&(1<<(i*2)))>>(i*2))<<(12+i);
						Q32|=((BuffIQ[k+5]&(1<<(i*2)))>>(i*2))<<(8+i);
						Q32|=((BuffIQ[k+6]&(1<<(i*2)))>>(i*2))<<(4+i);
						Q32|=((BuffIQ[k+7]&(1<<(i*2)))>>(i*2))<<i;
				
					}
					
					if(FEC==0) //CARRIER MODE : OVERWRITE IQ previously calculated
					{
					  //I32=TabIQTestI[NbSymbol%4];
					  //Q32=TabIQTestQ[NbSymbol%4];
						I32=0xFFFFFFFF;
					  	Q32=0;
					}
/*	
					if(FEC<0)
					{
						if(k%204!=0)
						{
							I32=TabIQTestI[0];
					 		Q32=TabIQTestQ[0];
						}
					}
*/
					ctl->sample[last_sample++] =I32;
					if (last_sample == NUM_SAMPLES)	last_sample = 0;
					ctl->sample[last_sample++] =Q32;
					if (last_sample == NUM_SAMPLES)	last_sample = 0;
					//printf("I32 %x Q32 %x \n",I32,Q32);
					NbSymbol++;
					free_slots -= 2;
				}
				
				
			
			}
			clock_gettime(CLOCK_REALTIME, &gettime_now);
			time_difference = gettime_now.tv_nsec - start_time;
			if(time_difference<0) time_difference+=1E9;	
			if((FEC>0)&&(time_difference>MaxToGetBuffer*1000))
			{
				 printf("File to slow !! wait for %ld us/ %ld us\n",(long int)time_difference/1000,(long int)MaxToGetBuffer);
				
					
			}
			last_cb = (uint32_t)virtbase + last_sample * sizeof(dma_cb_t) ;

		}

// ************************************************ MODE DTX 1 ***********************************************
		if(ModeIQ==2)
		{
			//uint32_t static TSRate;
			//TSRate=SymbolRate*FEC*188/(4*204*(FEC+1L));
			cur_cb = mem_phys_to_virt(dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40]);
			this_sample = (cur_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t)*2 );
			last_sample = (last_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t)*2 );
			
		    	free_slots = this_sample - last_sample;
			if (free_slots < 0) // WARNING : ORIGINAL CODE WAS < strictly
				free_slots += NUM_SAMPLES;
						
				//TimeToSleep=((NUM_SAMPLES-free_slots-188*4*2)*800)/((float)TSRate); // 22ms de Switch process
				TimeToSleep=2000+KERNEL_GRANULARITY;
				//printf("FreeSlots = %d Time to sleep=%d\n",free_slots,TimeToSleep);

				clock_gettime(CLOCK_REALTIME, &gettime_now);
				start_time = gettime_now.tv_nsec;		
				if(TimeToSleep>=(2000+KERNEL_GRANULARITY))
				{
					if(TimeToSleep>=200000) TimeToSleep=200000; //Digithin should be every 250ms
					udelay(TimeToSleep-(2000+KERNEL_GRANULARITY));
				}
				else
				{
					//usleep(0);//20 ms mini !!
					if(free_slots>(NUM_SAMPLES*9/10))
						 printf("Buffer nearly empty...%d/%d\n",free_slots,NUM_SAMPLES);
				}
				//printf("FreeSlots = %d Time to sleep=%d\n",free_slots,TimeToSleep);
				
				static int free_slots_now; 
				cur_cb = mem_phys_to_virt(dma_reg[DMA_CONBLK_AD+DMA_CHANNEL*0x40]);
				this_sample = (cur_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t)*2);
				last_sample = (last_cb - (uint32_t)virtbase) / (sizeof(dma_cb_t)*2);
				//printf("%lx %lx\n",this_sample,last_sample);
				free_slots_now = this_sample - last_sample;
				if (free_slots_now < 0) // WARNING : ORIGINAL CODE WAS < strictly
					free_slots_now += NUM_SAMPLES;
			
				clock_gettime(CLOCK_REALTIME, &gettime_now);
				time_difference = gettime_now.tv_nsec - start_time;
				if(time_difference<0) time_difference+=1E9;
				
				//printf("DiffTime = %ld FreeSlot=%ld Bitrate : %f\n",time_difference,free_slots_now-free_slots,(1000000.0*(free_slots_now-free_slots))/(float)time_difference*2.0);
				free_slots=free_slots_now;


				if((Init==1)&&(free_slots <= (188*8*4)/*NUM_SAMPLES/8*/))
				{
					printf("%ld:%ld : End of Fulling buffer \n",gettime_now.tv_sec,gettime_now.tv_nsec);
					dma_reg[DMA_CS+DMA_CHANNEL*0x40] = 0x10880001;	// go, mid priority, wait for outstanding writes :7 Seems Max Priority
					Init=0;
					sleep(0);
				}
			
						
			clock_gettime(CLOCK_REALTIME, &gettime_now);
			start_time = gettime_now.tv_nsec;
			while ((free_slots > (188*8*4))&&(BufferAvailable()>188*8)) //
			{
				
				//static uint32_t BuffAligned[256];
				//static uchar	*buff=(uchar *)BuffAligned ;

				static uchar buff[208];
				
				
				//static int data_len_total;
				//data_len_total=0;

				
				
					for(i=0;i<8;i++)
					{
					
						if(FEC>0)
						{
							/*
							static int ByteRead=0;
							if ((ByteRead=read(fdts,buff,188))!=188) // Read should be around 20us
							{
								printf("END OF FILE OR packet is not 188 long %d\n",data_len);
								if(Loop==1)
								{
									close(fdts);
									 fdts = open(argv[1], 'r');
								}
								else
								{	
									while(ByteRead!=188)
										ByteRead+=read(fdts,buff+ByteRead,188-ByteRead);
									//printf("End of processing file\n");
									//terminate(0);
								}	
								
							}
							*/
							int ii;
							pthread_mutex_lock(&my_circular_buffer.lock);
							for(ii=0;ii<188;ii++) buff[ii]=read_from_buffer();
							pthread_mutex_unlock(&my_circular_buffer.lock);
							
						}
						else
						{
							buff[0]=0x47;
							memset(buff+1,0,187);
						}
						
						static char MapBit[8]={2,3,4,14,15,17,18,27};
						

						if(buff[0]!=0x47) printf("SYNC 188 ERROR \n");
						int BytePacket=0;
						volatile uint32_t ByteClear;
						volatile uint32_t ByteSet;
						int bit;
						
						for(BytePacket=0;BytePacket<188;BytePacket++)
						{
							
							ByteSet=0;
							ByteClear=(1<<22); //CLK LOW
							
						
						
							if((BytePacket==0))
							{
								
								ByteSet|= (1 << 24); //TPCLK HIGH
								
							}
							else
							{
								

								ByteClear|=(1 <<24); //TPCLK 0

							}
							
							//TEST
							/*if(BytePacket==0)
								buff[BytePacket]=0x47;
							else	buff[BytePacket]=0x12;*/
							
						
							
							for(bit=0;bit<8;bit++)
							{
								ByteSet|=((buff[BytePacket]>>bit)&1)<<MapBit[bit];
								ByteClear|=(~((buff[BytePacket]>>bit))&1)<<MapBit[bit];
								
							}
							
							//printf("CYCLE 1:Byte %lx ByteSet %lx ByteClear %lx \n",buff[BytePacket],ByteSet,ByteClear);
							ctl->sample[last_sample++] =ByteClear;
							if (last_sample == NUM_SAMPLES)	last_sample = 0;
							ctl->sample[last_sample++] =ByteSet;
							if (last_sample == NUM_SAMPLES)	last_sample = 0;
							
							//ByteSet=(1<<22);//CLK HIGH
							ByteSet=(1<<22);//CLK HIGH
							ByteClear=0;//(1<<22);
				
							//printf("CYCLE 2:Byte %lx ByteSet %lx ByteClear %lx \n",buff[BytePacket],ByteSet,ByteClear);
							ctl->sample[last_sample++] =ByteClear;
							if (last_sample == NUM_SAMPLES)	last_sample = 0;

							ctl->sample[last_sample++] =ByteSet;
							if (last_sample == NUM_SAMPLES)	last_sample = 0;
							
							free_slots -= 4; // *2 porbleme de Index entre CBP ET DATA
						}
						
					}
				
				
				
				
				
			
			}
			clock_gettime(CLOCK_REALTIME, &gettime_now);
			time_difference = gettime_now.tv_nsec - start_time;
			if(time_difference<0) time_difference+=1E9;	
			if((FEC>0)&&(time_difference>MaxToGetBuffer*4000))
			{
				 printf("File to slow !! wait for %ld us/ %ld us\n",(long int)time_difference/1000,(long int)MaxToGetBuffer);
				
					
			}
			
			last_cb = (uint32_t)virtbase + last_sample * sizeof(dma_cb_t)*2 ;

		}		
	}

	terminate(0);
	return(0);
}

