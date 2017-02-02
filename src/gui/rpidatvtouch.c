//
// shapedemo: testbed for OpenVG APIs
// Anthony Starks (ajstarks@gmail.com)
//
#include <linux/input.h>
#include <string.h>


#include "touch.h"


#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

#include "VG/openvg.h"
#include "VG/vgu.h"
#include "fontinfo.h"
#include "shapes.h"



#include <pthread.h>
#include <fftw3.h>
#include <math.h>

#define KWHT  "\x1B[37m"
#define KYEL  "\x1B[33m"

#define PATH_CONFIG "/home/pi/rpidatv/scripts/rpidatvconfig.txt"
char ImageFolder[]="/home/pi/rpidatv/image/";

int fd=0;
int wscreen, hscreen;
float scaleXvalue, scaleYvalue; // Coeff ratio from Screen/TouchArea


typedef struct {
	int r,g,b;
} color_t;


typedef struct {
	char Text[255];
	color_t  Color;
} status_t;

#define MAX_STATUS 10
typedef struct {
	int x,y,w,h;

	status_t Status[MAX_STATUS];
	int IndexStatus;
	int NoStatus;
	int LastEventTime;
} button_t;

#define MAX_BUTTON 25
int IndexButtonInArray=0;
button_t ButtonArray[MAX_BUTTON];
int IsDisplayOn=0;
#define TIME_ANTI_BOUNCE 500

//GLOBAL PARAMETERS

int fec;
int SR;
char ModeInput[255];
char freqtxt[255];

// Values to be stored in and read from rpidatvconfig.txt:

int TabSR[5]= {125,333,1000,2000,4000};
int TabFec[5]={1,2,3,5,7};
char TabModeInput[5][255]={"CAMMPEG-2","CAMH264","PATERNAUDIO","ANALOGCAM","CARRIER"};
char TabFreq[5][255]={"71","146.5","437","1249","1255"};

int Inversed=0;//Display is inversed (Waveshare=1)

pthread_t thfft,thbutton;

/***************************************************************************//**
 * @brief Looks up the value of Param in PathConfigFile and sets value
 *        Used to look up the configuration from rpidatvconfig.txt
 *
 * @param PatchConfigFile (str) the name of the configuration text file
 * @param Param the string labeling the parameter
 * @param Value the looked-up value of the parameter
 *
 * @return void
*******************************************************************************/

void GetConfigParam(char *PathConfigFile,char *Param, char *Value)
{
	char * line = NULL;
	size_t len = 0;
	int read;
	FILE *fp=fopen(PathConfigFile,"r");
	if(fp!=0)
	{
		while ((read = getline(&line, &len, fp)) != -1)
		{
			if(strncmp (line,Param,strlen(Param)) == 0)
			{
				strcpy(Value,line+strlen(Param)+1);
				char *p;
				if((p=strchr(Value,'\n'))!=0) *p=0; //Remove \n
				break;
			}
			//strncpy(Value,line+strlen(Param)+1,strlen(line)-strlen(Param)-1-1/* pour retour chariot*/);
	    	}
	}
	else
		printf("Config file not found \n");
	fclose(fp);

}

/***************************************************************************//**
 * @brief sets the value of Param in PathConfigFile froma program variable
 *        Used to store the configuration in rpidatvconfig.txt
 *
 * @param PatchConfigFile (str) the name of the configuration text file
 * @param Param the string labeling the parameter
 * @param Value the looked-up value of the parameter
 *
 * @return void
*******************************************************************************/

void SetConfigParam(char *PathConfigFile,char *Param,char *Value)
{
	char * line = NULL;
	 size_t len = 0;
	int read;
	char BackupConfigName[255];
	strcpy(BackupConfigName,PathConfigFile);
	strcat(BackupConfigName,".bak");
	//printf("Read %s\n",PathConfigFile);
	 FILE *fp=fopen(PathConfigFile,"r");

	 FILE *fw=fopen(BackupConfigName,"w+");
	if(fp!=0)
	{
		while ((read = getline(&line, &len, fp)) != -1)
		{
		      	//printf("%s", line);
			if(strncmp (line,Param,strlen(Param)) == 0)
			{
				fprintf(fw,"%s=%s\n",Param,Value);
			}
			else
				fprintf(fw,line);
			//strncpy(Value,line+strlen(Param)+1,strlen(line)-strlen(Param)-1-1/* pour retour chariot*/);
	    	}
	}
	else
		printf("Config file not found \n");
	fclose(fp);
	fclose(fw);
	char Command[255];
	sprintf(Command,"cp %s %s",BackupConfigName,PathConfigFile);
	system(Command);
}

int mymillis()
{
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (tv.tv_sec) * 1000 + (tv.tv_usec)/1000;
}

int IsButtonPushed(int NbButton,int x,int y)
{
  int  scaledX, scaledY;

  // scaledx range approx 0 - 700
  // scaledy range approx 0 - 480

  // Adjust registration of touchscreen for Waveshare
  int shiftX, shiftY;
  double factorX, factorY;

  shiftX=30; // move touch sensitive position left (-) or right (+).  Screen is 700 wide
  shiftY=-5; // move touch sensitive positions up (-) or down (+).  Screen is 480 high

  factorX=-0.4;  // expand (+) or contract (-) horizontal button space from RHS. Screen is 5.6875 wide
  factorY=-0.3;  // expand or contract vertical button space.  Screen is 8.53125 high

  // Switch axes for normal and waveshare displays
  if(Inversed==0) //TonTec
  {
    scaledX = x/scaleXvalue;
    scaledY = hscreen-y/scaleYvalue;
  }
  else //Waveshare (inversed)
  {
    scaledX = shiftX+wscreen-y/(scaleXvalue+factorX);
    scaledY = shiftY+hscreen-x/(scaleYvalue+factorY);
  }

  // printf("x=%d y=%d scaledx %d scaledy %d sxv %f syv %f\n",x,y,scaledX,scaledY,scaleXvalue,scaleYvalue);

  int margin=10;  // was 20

  if((scaledX<=(ButtonArray[NbButton].x+ButtonArray[NbButton].w-margin))&&(scaledX>=ButtonArray[NbButton].x+margin) &&
    (scaledY<=(ButtonArray[NbButton].y+ButtonArray[NbButton].h-margin))&&(scaledY>=ButtonArray[NbButton].y+margin)
	/*&&(mymillis()-ButtonArray[NbButton].LastEventTime>TIME_ANTI_BOUNCE)*/)
	{
		ButtonArray[NbButton].LastEventTime=mymillis();
		return 1;
	}
	else
		return 0;

}

int AddButton(int x,int y,int w,int h)
{
	button_t *NewButton=&(ButtonArray[IndexButtonInArray]);
	NewButton->x=x;
	NewButton->y=y;
	NewButton->w=w;
	NewButton->h=h;
	NewButton->NoStatus=0;
	NewButton->IndexStatus=0;
	NewButton->LastEventTime=mymillis();
	return IndexButtonInArray++;
}

int AddButtonStatus(int ButtonIndex,char *Text,color_t *Color)
{
	button_t *Button=&(ButtonArray[ButtonIndex]);
	strcpy(Button->Status[Button->IndexStatus].Text,Text);
	Button->Status[Button->IndexStatus].Color=*Color;
	return Button->IndexStatus++;
}

void DrawButton(int ButtonIndex)
{
	button_t *Button=&(ButtonArray[ButtonIndex]);

	Fill(Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g, Button->Status[Button->NoStatus].Color.b, 1);
	 Roundrect(Button->x,Button->y,Button->w,Button->h, Button->w/10, Button->w/10);
	Fill(255, 255, 255, 1);				   // White text
	TextMid(Button->x+Button->w/2, Button->y+Button->h/2, Button->Status[Button->NoStatus].Text, SerifTypeface, Button->w/strlen(Button->Status[Button->NoStatus].Text)/*25*/);	
}

void SetButtonStatus(int ButtonIndex,int Status)
{
	button_t *Button=&(ButtonArray[ButtonIndex]);
	Button->NoStatus=Status;
}

int GetButtonStatus(int ButtonIndex)
{
	button_t *Button=&(ButtonArray[ButtonIndex]);
	return	Button->NoStatus;

}

void GetNextPicture(char *PictureName)
{

	DIR           *d;
 	struct dirent *dir;

  	d = opendir(ImageFolder);
  	if (d)
  	{
    	while ((dir = readdir(d)) != NULL)
    	{
		if (dir->d_type == DT_REG)
		{
			size_t len = strlen(dir->d_name);
    			if( len > 4 && strcmp(dir->d_name + len - 4, ".jpg") == 0)
			{
    				 printf("%s\n", dir->d_name);

				strncpy(PictureName,dir->d_name,strlen(dir->d_name)-4);
				break;
			}
  		}
      	}

    	closedir(d);
	}
}

int openTouchScreen(int NoDevice)
{
	char sDevice[255];
	sprintf(sDevice,"/dev/input/event%d",NoDevice);
	if(fd!=0) close(fd);
        if ((fd = open(sDevice, O_RDONLY)) > 0)
	 {
                return 1;
        }
	else
		return 0;
}

/*
Input device name: "ADS7846 Touchscreen"
Supported events:
  Event type 0 (Sync)
  Event type 1 (Key)
    Event code 330 (Touch)
  Event type 3 (Absolute)
    Event code 0 (X)
     Value      0
     Min        0
     Max     4095
    Event code 1 (Y)
     Value      0
     Min        0
     Max     4095
    Event code 24 (Pressure)
     Value      0
     Min        0
     Max      255
*/

int getTouchScreenDetails(int *screenXmin,int *screenXmax,int *screenYmin,int *screenYmax)
{
	//unsigned short id[4];
        unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
        char name[256] = "Unknown";
        int abs[6] = {0};

        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        printf("Input device name: \"%s\"\n", name);

        memset(bit, 0, sizeof(bit));
        ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
        printf("Supported events:\n");

        int i,j,k;
	int IsAtouchDevice=0;
        for (i = 0; i < EV_MAX; i++)
                if (test_bit(i, bit[0])) {
                        printf("  Event type %d (%s)\n", i, events[i] ? events[i] : "?");
                        if (!i) continue;
                        ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
                        for (j = 0; j < KEY_MAX; j++){
                                if (test_bit(j, bit[i])) {
                                        printf("    Event code %d (%s)\n", j, names[i] ? (names[i][j] ? names[i][j] : "?") : "?");
	if(j==330) IsAtouchDevice=1;
                                        if (i == EV_ABS) {
                                                ioctl(fd, EVIOCGABS(j), abs);
                                                for (k = 0; k < 5; k++)
                                                        if ((k < 3) || abs[k]){
                                                                printf("     %s %6d\n", absval[k], abs[k]);
                                                                if (j == 0){
                                                                        if (absval[k] == "Min  ") *screenXmin =  abs[k];
                                                                        if (absval[k] == "Max  ") *screenXmax =  abs[k];
                                                                }
                                                                if (j == 1){
                                                                        if (absval[k] == "Min  ") *screenYmin =  abs[k];
                                                                        if (absval[k] == "Max  ") *screenYmax =  abs[k];
                                                                }
                                                        }
                                                }

                                        }
                               }
                        }

return IsAtouchDevice;
}


int getTouchSample(int *rawX, int *rawY, int *rawPressure)
{
	int i;
        /* how many bytes were read */
        size_t rb;
        /* the events (up to 64 at once) */
        struct input_event ev[64];
	//static int Last_event=0; //not used?
	rb=read(fd,ev,sizeof(struct input_event)*64);
	*rawX=-1;*rawY=-1;
	int StartTouch=0;
        for (i = 0;  i <  (rb / sizeof(struct input_event)); i++){
              if (ev[i].type ==  EV_SYN)
		{
                         //printf("Event type is %s%s%s = Start of New Event\n",KYEL,events[ev[i].type],KWHT);
		}
                else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 1)
		{
			StartTouch=1;
                        //printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s1%s = Touch Starting\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,KWHT);
		}
                else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 0)
		{
			//StartTouch=0;
			//printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s0%s = Touch Finished\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,KWHT);
		}
                else if (ev[i].type == EV_ABS && ev[i].code == 0 && ev[i].value > 0){
                        //printf("Event type is %s%s%s & Event code is %sX(0)%s & Event value is %s%d%s\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,ev[i].value,KWHT);
			*rawX = ev[i].value;
		}
                else if (ev[i].type == EV_ABS  && ev[i].code == 1 && ev[i].value > 0){
                        //printf("Event type is %s%s%s & Event code is %sY(1)%s & Event value is %s%d%s\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,ev[i].value,KWHT);
			*rawY = ev[i].value;
		}
                else if (ev[i].type == EV_ABS  && ev[i].code == 24 && ev[i].value > 0){
                        //printf("Event type is %s%s%s & Event code is %sPressure(24)%s & Event value is %s%d%s\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,ev[i].value,KWHT);
			*rawPressure = ev[i].value;
		}
		if((*rawX!=-1)&&(*rawY!=-1)&&(StartTouch==1))
		{
			/*if(Last_event-mymillis()>500)
			{
				Last_event=mymillis();
				return 1;
			}*/
			//StartTouch=0;
			return 1;
		}

	}
	return 0;
}

void UpdateWindow()
{
	int i;
	for(i=0;i<IndexButtonInArray;i++)
		DrawButton(i);
	End();
}

void SelectInGroup(int StartButton,int StopButton,int NoButton,int Status)
{
	int i;
	for(i=StartButton;i<=StopButton;i++)
	{
		if(i==NoButton)
		 	SetButtonStatus(i,Status);
		else
			 SetButtonStatus(i,0);
	}
}

void SelectFreq(int NoButton)  //Frequency
{
	SelectInGroup(0,4,NoButton,1);
	strcpy(freqtxt,TabFreq[NoButton-0]);
	char Param[]="freqoutput";
        printf("************** Set Frequency = %s\n",freqtxt);
	SetConfigParam(PATH_CONFIG,Param,freqtxt);

  // Set the Band (and filter) Switching

  system ("sudo /home/pi/rpidatv/scripts/ctlfilter.sh");

}


void SelectSR(int NoButton)  // Symbol Rate
{
  SelectInGroup(5,9,NoButton,1);
  SR=TabSR[NoButton-5];
  char Param[]="symbolrate";
  char Value[255];
  sprintf(Value,"%d",SR);
  printf("************** Set SR = %s\n",Value);
  SetConfigParam(PATH_CONFIG,Param,Value);

  // Kill express_server in case SR has gone from nb to wb or vice versa
  system("sudo killall express_server >/dev/null 2>/dev/null");
}

void SelectFec(int NoButton)  // FEC
{
	SelectInGroup(10,14,NoButton,1);
	fec=TabFec[NoButton-10];
	char Param[]="fec";
	char Value[255];
	sprintf(Value,"%d",fec);
	printf("************** Set FEC = %s\n",Value);
	SetConfigParam(PATH_CONFIG,Param,Value);
}

void SelectSource(int NoButton,int Status)  //Input mode
{
	SelectInGroup(15,19,NoButton,Status);
	strcpy(ModeInput,TabModeInput[NoButton-15]);
	printf("************** Set Input Mode = %s\n",ModeInput);
	char Param[]="modeinput";
	SetConfigParam(PATH_CONFIG,Param,ModeInput);
}

void SelectPTT(int NoButton,int Status)  // TX/RX
{
	SelectInGroup(20,21,NoButton,Status);
}

void TransmitStart()
{
	printf("Transmit Start\n");

	#define PATH_SCRIPT_A "sudo /home/pi/rpidatv/scripts/a.sh >/dev/null 2>/dev/null"
	if((strcmp(ModeInput,TabModeInput[0])==0)||(strcmp(ModeInput,TabModeInput[1])==0)) //CAM
	{
		printf("DISPLAY OFF \n");
		IsDisplayOn=0;
		finish();

		system("v4l2-ctl --overlay=1 >/dev/null 2>/dev/null");
	}

	system(PATH_SCRIPT_A);

}

void TransmitStop()
{
  printf("Transmit Stop\n");

  // Turn the VCO off
  system("sudo /home/pi/rpidatv/bin/adf4351 off");

  // Stop DATV Express transmitting
  char expressrx[50];
  strcpy( expressrx, "echo \"set ptt rx\" >> /tmp/expctrl" );
  system(expressrx);
  strcpy( expressrx, "echo \"set car off\" >> /tmp/expctrl" );
  system(expressrx);
  system("sudo killall netcat >/dev/null 2>/dev/null");

  // Kill the key processes as nicely as possible
  system("sudo killall rpidatv >/dev/null 2>/dev/null");
  system("sudo killall ffmpeg >/dev/null 2>/dev/null");
  system("sudo killall tcanim >/dev/null 2>/dev/null");
  system("sudo killall avc2ts >/dev/null 2>/dev/null");
  system("v4l2-ctl --overlay=0 >/dev/null 2>/dev/null");

  // Then pause and make sure that avc2ts has really been stopped (needed at high SRs)
  usleep(1000);
  system("sudo killall -9 avc2ts >/dev/null 2>/dev/null");

  // And make sure rpidatv has been stopped (required for brief transmit selections)
  system("sudo killall -9 rpidatv >/dev/null 2>/dev/null");
}

void coordpoint(VGfloat x, VGfloat y, VGfloat size, VGfloat pcolor[4]) {
  setfill(pcolor);
  Circle(x, y, size);
  setfill(pcolor);
}

	fftwf_complex *fftout=NULL;
#define FFT_SIZE 256

int FinishedButton=0;

void *DisplayFFT(void * arg)
{
	FILE * pFileIQ = NULL;
	int fft_size=FFT_SIZE;
	fftwf_complex *fftin;
	fftin = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
	fftout = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
	fftwf_plan plan ;
	plan = fftwf_plan_dft_1d(fft_size, fftin, fftout, FFTW_FORWARD, FFTW_ESTIMATE );

	system("mkfifo fifo.iq >/dev/null 2>/dev/null");
	printf("Entering FFT thread\n");
	pFileIQ = fopen("fifo.iq", "r");

	while(FinishedButton==0)
	{
		int Nbread; // value set later but not used
		//int log2_N=11; //FFT 1024 not used?
		//int ret; // not used?

		Nbread=fread( fftin,sizeof(fftwf_complex),FFT_SIZE,pFileIQ);
		fftwf_execute( plan );

		//printf("NbRead %d %d\n",Nbread,sizeof(struct GPU_FFT_COMPLEX));

		fseek(pFileIQ,(1200000-FFT_SIZE)*sizeof(fftwf_complex),SEEK_CUR);
	}
	fftwf_free(fftin);
	fftwf_free(fftout);
}

void *WaitButtonEvent(void * arg)
{
	int rawX, rawY, rawPressure;

	while(getTouchSample(&rawX, &rawY, &rawPressure)==0);

	FinishedButton=1;
}

void ProcessLeandvb()
{
   #define PATH_SCRIPT_LEAN "sudo /home/pi/rpidatv/scripts/leandvbgui.sh 2>&1"
   char *line=NULL;
   size_t len = 0;
    ssize_t read;

	// int rawX, rawY, rawPressure; //  not used
	FILE *fp;
	// VGfloat px[1000];  // Variable not used
	// VGfloat py[1000];  // Variable not used
	VGfloat shapecolor[4];
	RGBA(255, 255, 128,1, shapecolor);

	printf("Entering LeandProcess\n");
	FinishedButton=0;
// Thread FFT

	pthread_create (&thfft,NULL, &DisplayFFT,NULL);

//END ThreadFFT

// Thread FFT

	pthread_create (&thbutton,NULL, &WaitButtonEvent,NULL);

//END ThreadFFT

	fp=popen(PATH_SCRIPT_LEAN, "r");
	if(fp==NULL) printf("Process error\n");

 while (((read = getline(&line, &len, fp)) != -1)&&(FinishedButton==0))
 {

        char  strTag[20];
	int NbData;
	static int Decim=0;
	sscanf(line,"%s ",strTag);
	char * token;
	static int Lock=0;
	static float SignalStrength=0;
	static float MER=0;
	static float FREQ=0;
	if((strcmp(strTag,"SYMBOLS")==0))
	{

		token = strtok(line," ");
		token = strtok(NULL," ");
		sscanf(token,"%d",&NbData);

		if(Decim%25==0)
		{
			//Start(wscreen,hscreen);
			Fill(255, 255, 255, 1);
			Roundrect(0,0,256,hscreen, 10, 10);
			BackgroundRGB(0,0,0,0);
			//Lock status
			char sLock[100];
			if(Lock==1)
			{
				strcpy(sLock,"Lock");
				Fill(0,255,0, 1);

			}
			else
			{
				strcpy(sLock,"----");
				Fill(255,0,0, 1);
			}
			Roundrect(200,0,100,50, 10, 10);
			Fill(255, 255, 255, 1);				   // White text
			Text(200, 20, sLock, SerifTypeface, 25);

			//Signal Strength
			char sSignalStrength[100];
			sprintf(sSignalStrength,"%3.0f",SignalStrength);

			Fill(255-SignalStrength,SignalStrength,0,1);
			Roundrect(350,0,20+SignalStrength/2,50, 10, 10);
			Fill(255, 255, 255, 1);				   // White text
			Text(350, 20, sSignalStrength, SerifTypeface, 25);

			//MER 2-30
			char sMER[100];
			sprintf(sMER,"%2.1fdB",MER);
			Fill(255-MER*8,(MER*8),0,1);
			Roundrect(500,0,(MER*8),50, 10, 10);
			Fill(255, 255, 255, 1);				   // White text
			Text(500,20, sMER, SerifTypeface, 25);
		}

		if(Decim%25==0)
		{
			static VGfloat PowerFFTx[FFT_SIZE];
			static VGfloat PowerFFTy[FFT_SIZE];
			StrokeWidth(2);

			Stroke(150, 150, 200, 0.8);
			int i;
			if(fftout!=NULL)
			{
			for(i=0;i<FFT_SIZE;i+=2)
			{

				PowerFFTx[i]=(i<FFT_SIZE/2)?(FFT_SIZE+i)/2:i/2;
				PowerFFTy[i]=log10f(sqrt(fftout[i][0]*fftout[i][0]+fftout[i][1]*fftout[i][1])/FFT_SIZE)*100;	
			Line(PowerFFTx[i],0,PowerFFTx[i],PowerFFTy[i]);
			//Polyline(PowerFFTx,PowerFFTy,FFT_SIZE);

			//Line(0, (i<1024/2)?(1024/2+i)/2:(i-1024/2)/2,  (int)sqrt(fftout[i][0]*fftout[i][0]+fftout[i][1]*fftout[i][1])*100/1024,(i<1024/2)?(1024/2+i)/2:(i-1024/2)/2);

			}
			//Polyline(PowerFFTx,PowerFFTy,FFT_SIZE);
			}
			//FREQ
			Stroke(0, 0, 255, 0.8);
			//Line(FFT_SIZE/2+FREQ/2/1024000.0,0,FFT_SIZE/2+FREQ/2/1024000.0,hscreen/2);
			Line(FFT_SIZE/2,0,FFT_SIZE/2,10);
			Stroke(0, 0, 255, 0.8);
			Line(0,hscreen-300,256,hscreen-300);
			StrokeWidth(10);
			Line(128+(FREQ/40000.0)*256.0,hscreen-300-20,128+(FREQ/40000.0)*256.0,hscreen-300+20);

			char sFreq[100];
			sprintf(sFreq,"%2.1fkHz",FREQ/1000.0);
			Text(0,hscreen-300+25, sFreq, SerifTypeface, 20);

		}
		if((Decim%25)==0)
		{
			int x,y;
			Decim++;
			int i;
			StrokeWidth(2);
			Stroke(255, 255, 128, 0.8);
			for(i=0;i<NbData;i++)
			{
				token=strtok(NULL," ");
				sscanf(token,"%d,%d",&x,&y);
				coordpoint(x+128, hscreen-(y+128), 5, shapecolor);

				Stroke(0, 255, 255, 0.8);
				Line(0,hscreen-128,256,hscreen-128);
				Line(128,hscreen,128,hscreen-256);

			}


			End();
			//usleep(40000);

		}
		else
			Decim++;
		/*if(Decim%1000==0)
		{
			char FileSave[255];
			FILE *File;
			sprintf(FileSave,"Snap%d_%dx%d.png",Decim,wscreen,hscreen);
			File=fopen(FileSave,"w");

			dumpscreen(wscreen,hscreen,File);
			fclose(File);
		}*/
		/*if(Decim>200)
		{
			Decim=0;
			Start(wscreen,hscreen);

		}*/

	}

	if((strcmp(strTag,"SS")==0))
	{
		token = strtok(line," ");
		token = strtok(NULL," ");
		sscanf(token,"%f",&SignalStrength);
		//printf("Signal %f\n",SignalStrength);
	}

	if((strcmp(strTag,"MER")==0))
	{

		token = strtok(line," ");
		token = strtok(NULL," ");
		sscanf(token,"%f",&MER);
		//printf("MER %f\n",MER);
	}

	if((strcmp(strTag,"FREQ")==0))
	{

		token = strtok(line," ");
		token = strtok(NULL," ");
		sscanf(token,"%f",&FREQ);
		//printf("FREQ %f\n",FREQ);
	}

	if((strcmp(strTag,"LOCK")==0))
	{

		token = strtok(line," ");
		token = strtok(NULL," ");
		sscanf(token,"%d",&Lock);
	}

	free(line);
	line=NULL;
    }
printf("End Lean - Clean\n");

system("sudo killall fbi");  // kill any previous images
system("sudo fbi -T 1 -noverbose -a /home/pi/rpidatv/scripts/images/BATC_Black.png");  // Add logo image

usleep(5000000); // Time to FFT end reading samples
   pthread_join(thfft, NULL);
	//pclose(fp);
	pthread_join(thbutton, NULL);
	printf("End Lean\n");
}

void ReceiveStart()
{
	//system("sudo SDL_VIDEODRIVER=fbcon SDL_FBDEV=/dev/fb0 mplayer -ao /dev/null -vo sdl  /home/pi/rpidatv/video/mire250.ts &");
	//system(PATH_SCRIPT_LEAN);
	ProcessLeandvb();
}

void ReceiveStop()
{
	system("sudo killall leandvb");
	system("sudo killall hello_video.bin");
	//system("sudo killall mplayer");
}

// wait for a specific character 
void waituntil(int w,int h,int endchar)
{
	// int key; not used?
	int rawX, rawY, rawPressure,i;

	// int Toggle=0; not used

	for (;;)
	{
		//Start(w,h);
		if (getTouchSample(&rawX, &rawY, &rawPressure)==0) continue;
		printf("x=%d y=%d\n",rawX,rawY);
		if(IsDisplayOn==0)
		{
				printf("Display ON\n");
				TransmitStop();
				ReceiveStop();
				init(&wscreen, &hscreen);
				Start(wscreen,hscreen);
				BackgroundRGB(255,255,255,255);
				IsDisplayOn=1;

				SelectPTT(20,0);
				SelectPTT(21,0);
				UpdateWindow();
				//usleep(500000);
				continue;
		}
	for(i=0;i<IndexButtonInArray;i++)
	{
		if(IsButtonPushed(i,rawX,rawY)==1)
		{
			printf("Button Event %d\n",i);
			if((i>=0)&&(i<=4)) //Frequency
			{
				SelectFreq(i);
			}
			if((i>=5)&&(i<=9)) //SR	
			{
				SelectSR(i);
			}
			if((i>=10)&&(i<=14)) //FEC
			{
				SelectFec(i);
			}
			if((i>=15)&&(i<=19)) //Source
			{
				SelectSource(i,1);
			}
			if((i>=20)&&(i<=21)) //PTT
			{

				printf("Status %d\n",GetButtonStatus(i));
				if((i==20)&&(GetButtonStatus(i)==0))
				{
					usleep(500000);
					SelectPTT(i,1);
					UpdateWindow();
					TransmitStart();
					break;
				}
				if((i==20)&&(GetButtonStatus(i)==1))
				{
					TransmitStop();
					usleep(500000);
					SelectPTT(i,0);
					UpdateWindow();
					break;
				}
				if(i==21)
				{
					printf("DISPLAY OFF \n");
					//finish();
					BackgroundRGB(0,0,0,255);
					ReceiveStart();
					BackgroundRGB(255,255,255,255);
					IsDisplayOn=1;

					SelectPTT(20,0);
					SelectPTT(21,0);
					UpdateWindow();
					IsDisplayOn=1;
					//usleep(500000);
				}

			}
			if(IsDisplayOn==1)
			{
				UpdateWindow();
	//			DrawButton(i)

	//		End();
			}
			/*if((i==0)&&(GetButtonStatus(i)==0))
			{
				printf("DISPLAY OFF \n");
				finish();
				IsDisplayOn=0;
			}
			if((i==0)&&(GetButtonStatus(i)==1))
			{
				printf("DISPLAY ON  \n");
				init(&wscreen, &hscreen);
				Start(wscreen,hscreen);
				IsDisplayOn=1;
				UpdateWindow();
			}*/
			//FixMe : Add a Antibounce
		}
	}
	//circleCursor(scaledX,h-scaledY);


//        key = getchar();
  //      if (key == endchar || key == '\n') {
    //        break;
     //   }
    }
}

static void
terminate(int dummy)
{
	printf("Terminate\n");
        char Commnd[255];
        sprintf(Commnd,"stty echo");
        system(Commnd);

	/*restoreterm();
	finish();*/
	exit(1);
}

// main initializes the system and shows the picture. 
// Exit and clean up when you hit [RETURN].
int main(int argc, char **argv) {
	// int n;  // not used?
	// char *progname = argv[0]; // not used?
	int NoDeviceEvent=0;
	saveterm();
	init(&wscreen, &hscreen);
	rawterm();
	int screenXmax, screenXmin;
	int screenYmax, screenYmin;
	int ReceiveDirect=0;
	int i;
        char Param[255];
        char Value[255];
 
// Catch sigaction and call terminate
	for (i = 0; i < 16; i++) {
		struct sigaction sa;

		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = terminate;
		sigaction(i, &sa, NULL);
	}

// Determine if using waveshare screen
// Either by first argument or from rpidatvconfig.txt
	if(argc>1)
		Inversed=atoi(argv[1]);
        strcpy(Param,"display");

        GetConfigParam(PATH_CONFIG,Param,Value);
        if(strcmp(Value,"Waveshare")==0)
        	Inversed=1;

// Set the Band (and filter) Switching

  system ("sudo /home/pi/rpidatv/scripts/ctlfilter.sh");

// Determine if ReceiveDirect 2nd argument 
	if(argc>2)
		ReceiveDirect=atoi(argv[2]);

	if(ReceiveDirect==1)
	{
		getTouchScreenDetails(&screenXmin,&screenXmax,&screenYmin,&screenYmax);
		 ProcessLeandvb(); // For FrMenu and no 
	}

// Check for presence of touchscreen
	for(NoDeviceEvent=0;NoDeviceEvent<5;NoDeviceEvent++)
	{
		if (openTouchScreen(NoDeviceEvent) == 1)
		{
			if(getTouchScreenDetails(&screenXmin,&screenXmax,&screenYmin,&screenYmax)==1) break;
		}
	}
	if(NoDeviceEvent==5) 
	{
		perror("No Touchscreen found");
		exit(1);
	}

// Calculate screen parameters
	scaleXvalue = ((float)screenXmax-screenXmin) / wscreen;
	//printf ("X Scale Factor = %f\n", scaleXvalue);
	scaleYvalue = ((float)screenYmax-screenYmin) / hscreen;
	//printf ("Y Scale Factor = %f\n", scaleYvalue);

// Define button grid
  // -25 keeps right hand side symmetrical with left hand side
	int wbuttonsize=(wscreen-25)/5;
	int hbuttonsize=hscreen/6;

// Frequency

	int button=AddButton(0*wbuttonsize+20,0+hbuttonsize*0+20,wbuttonsize*0.9,hbuttonsize*0.9);
	color_t Col;
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button," 71 MHz ",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button," 71 MHz ",&Col);

	button=AddButton(1*wbuttonsize+20,hbuttonsize*0+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"146.5 MHz",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"146.5 MHz",&Col);

	button=AddButton(2*wbuttonsize+20,hbuttonsize*0+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"437 MHz ",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"437 MHz ",&Col);

	button=AddButton(3*wbuttonsize+20,hbuttonsize*0+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"1249 MHz",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"1249 MHz",&Col);

	button=AddButton(4*wbuttonsize+20,hbuttonsize*0+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"1255 MHz",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"1255 MHz",&Col);

// Symbol Rate

	button=AddButton(0*wbuttonsize+20,0+hbuttonsize*1+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"SR 125",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"SR 125",&Col);

	button=AddButton(1*wbuttonsize+20,hbuttonsize*1+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"SR 333",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"SR 333",&Col);

	button=AddButton(2*wbuttonsize+20,hbuttonsize*1+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"SR1000",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"SR1000",&Col);

	button=AddButton(3*wbuttonsize+20,hbuttonsize*1+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"SR2000",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"SR2000",&Col);

	button=AddButton(4*wbuttonsize+20,hbuttonsize*1+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"SR4000",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"SR4000",&Col);

// FEC

	button=AddButton(0*wbuttonsize+20,hbuttonsize*2+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"FEC 1/2",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"FEC 1/2",&Col);

	button=AddButton(1*wbuttonsize+20,hbuttonsize*2+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"FEC 2/3",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"FEC 2/3",&Col);

	button=AddButton(2*wbuttonsize+20,hbuttonsize*2+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"FEC 3/4",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"FEC 3/4",&Col);

	button=AddButton(3*wbuttonsize+20,hbuttonsize*2+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"FEC 5/6",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"FEC 5/6",&Col);

	button=AddButton(4*wbuttonsize+20,hbuttonsize*2+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"FEC 7/8",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"FEC 7/8",&Col);

//SOURCE

	button=AddButton(0*wbuttonsize+20,hbuttonsize*3+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"CAM MPEG2",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"CAM MPEG2",&Col);

	button=AddButton(1*wbuttonsize+20,hbuttonsize*3+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"CAM H264",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"CAM H264",&Col);

	char PictureName[255];
	//strcpy(PictureName,ImageFolder);
	GetNextPicture(PictureName);

	button=AddButton(2*wbuttonsize+20,hbuttonsize*3+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"Pattern",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"Pattern",&Col);

	button=AddButton(3*wbuttonsize+20,hbuttonsize*3+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"Analog",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"Analog",&Col);

	button=AddButton(4*wbuttonsize+20,hbuttonsize*3+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"Carrier",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"Carrier",&Col);

//TRANSMIT

	button=AddButton(0*wbuttonsize+20,hbuttonsize*4+20,wbuttonsize*1.2,hbuttonsize*1.2);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"TX   ",&Col);
	Col.r=255;Col.g=0;Col.b=0;
	AddButtonStatus(button,"TX ON",&Col);

	button=AddButton(1*wbuttonsize*3+20,hbuttonsize*4+20,wbuttonsize*1.2,hbuttonsize*1.2);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"RX   ",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"RX ON",&Col);

	Start(wscreen,hscreen);
	IsDisplayOn=1;

// Determine button highlights

	// Frequency

 	strcpy(Param,"freqoutput");
	GetConfigParam(PATH_CONFIG,Param,Value);
	strcpy(freqtxt,Value);
	printf("Value=%s %s\n",Value,"Freq");
	if(strcmp(Value,"71")==0)
	{
	     SelectFreq(0);
	}
	if(strcmp(Value,"146.5")==0)
        {
             SelectFreq(1);
        }
        if(strcmp(Value,"437")==0)
        {
             SelectFreq(2);
        }
        if(strcmp(Value,"1249")==0)
        {
             SelectFreq(3);
        }
        if(strcmp(Value,"1255")==0)
        {
             SelectFreq(4);
        }

	// Symbol Rate

	strcpy(Param,"symbolrate");
	GetConfigParam(PATH_CONFIG,Param,Value);
	SR=atoi(Value);
	printf("Value=%s %s\n",Value,"SR");
	switch(SR)
	{
		case 125:SelectSR(5);break;
		case 333:SelectSR(6);break;
		case 1000:SelectSR(7);break;
		case 2000:SelectSR(8);break;
		case 4000:SelectSR(9);break;
	}

	// FEC

	strcpy(Param,"fec");
	strcpy(Value,"");
	GetConfigParam(PATH_CONFIG,Param,Value);
	printf("Value=%s %s\n",Value,"Fec");
	fec=atoi(Value);
	switch(fec)
	{
		case 1:SelectFec(10);break;
		case 2:SelectFec(11);break;
		case 3:SelectFec(12);break;
		case 5:SelectFec(13);break;
		case 7:SelectFec(14);break;
	}

	// Input Mode

	strcpy(Param,"modeinput");
	GetConfigParam(PATH_CONFIG,Param,Value);
	strcpy(ModeInput,Value);
	printf("Value=%s %s\n",Value,"Input Mode");

        if(strcmp(Value,"CAMMPEG-2")==0)
        {
            SelectSource(15,1);
        }
	if(strcmp(Value,"CAMH264")==0)
	{
	    SelectSource(16,1);
	}
	if(strcmp(Value,"PATERNAUDIO")==0)
	{
	    SelectSource(17,1);
	}
        if(strcmp(Value,"ANALOGCAM")==0)
        {
            SelectSource(18,1);
        }
	if(strcmp(Value,"CARRIER")==0)
	{
	    SelectSource(19,1);
	}

	UpdateWindow();

	printf("Update Window\n");

	// RESIZE JPEG TO BE DONE
	/*char PictureName[255];
	strcpy(PictureName,ImageFolder);
	GetNextPicture(PictureName);

	Image(0,0,300,200,PictureName);

	End();
	*/
	//ReceiveStart();
	waituntil(wscreen,hscreen,0x1b);
	restoreterm();
	finish();
	return 0;
}
