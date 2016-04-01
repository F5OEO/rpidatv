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



#define KWHT  "\x1B[37m"
#define KYEL  "\x1B[33m"

#define PATH_CONFIG "/home/pi/rpidatv/scripts/rpidatvconfig.txt"
char ImageFolder[]="/home/pi/rpidatv/image/";

int fd;
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

#define MAX_BUTTON 20
int IndexButtonInArray=0;
button_t ButtonArray[MAX_BUTTON];
int IsDisplayOn=0;
#define TIME_ANTI_BOUNCE 500
//GLOBAL PARAM
int fec;
int SR;
char ModeInput[255];
int TabSR[5]= {125,250,333,500,1000};
int TabFec[5]={1,2,3,5,7};
char TabModeInput[5][255]={"CAMMPEG-2","CAMH264","PATERNAUDIO","FILETS","CARRIER"};

GetConfigParam(char *PathConfigFile,char *Param, char *Value)
{
	char * line = NULL;
	 size_t len = 0;
	int read;
	//printf("Read %s\n",PathConfigFile);
	 FILE *fp=fopen(PathConfigFile,"r");
	if(fp!=0)
	{
		while ((read = getline(&line, &len, fp)) != -1)
		{
		      	printf("%s", line);
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

SetConfigParam(char *PathConfigFile,char *Param,char *Value)
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
		      	printf("%s", line);
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
	scaledX = x/scaleXvalue;
        scaledY = hscreen-y/scaleYvalue;
	//printf("x=%d y=%d\n",scaledX,scaledY);
	int margin=20;
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

AddButtonStatus(int ButtonIndex,char *Text,color_t *Color)
{
	button_t *Button=&(ButtonArray[ButtonIndex]);
	strcpy(Button->Status[Button->IndexStatus].Text,Text);
	Button->Status[Button->IndexStatus].Color=*Color;
	return Button->IndexStatus++;	

}

DrawButton(int ButtonIndex)
{
	button_t *Button=&(ButtonArray[ButtonIndex]);
	
	Fill(Button->Status[Button->NoStatus].Color.r, Button->Status[Button->NoStatus].Color.g, Button->Status[Button->NoStatus].Color.b, 1);
	 Roundrect(Button->x,Button->y,Button->w,Button->h, Button->w/10, Button->w/10);
	Fill(255, 255, 255, 1);				   // White text
	TextMid(Button->x+Button->w/2, Button->y+Button->h/2, Button->Status[Button->NoStatus].Text, SerifTypeface, Button->w/strlen(Button->Status[Button->NoStatus].Text)/*25*/);	

}

SetButtonStatus(int ButtonIndex,int Status)
{
	button_t *Button=&(ButtonArray[ButtonIndex]);
	Button->NoStatus=Status;
}

int GetButtonStatus(int ButtonIndex)
{
	button_t *Button=&(ButtonArray[ButtonIndex]);
	return	Button->NoStatus;
 
}

GetNextPicture(char *PictureName)
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

int openTouchScreen()
{
        if ((fd = open("/dev/input/event0", O_RDONLY)) < 0)
	 {
                return 1;
        }
	else
		return 0;
}


void getTouchScreenDetails(int *screenXmin,int *screenXmax,int *screenYmin,int *screenYmax)
{
	unsigned short id[4];
        unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
        char name[256] = "Unknown";
        int abs[6] = {0};

        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        printf("Input device name: \"%s\"\n", name);

        memset(bit, 0, sizeof(bit));
        ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
        printf("Supported events:\n");

        int i,j,k;
        for (i = 0; i < EV_MAX; i++)
                if (test_bit(i, bit[0])) {
                        printf("  Event type %d (%s)\n", i, events[i] ? events[i] : "?");
                        if (!i) continue;
                        ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
                        for (j = 0; j < KEY_MAX; j++){
                                if (test_bit(j, bit[i])) {
                                        printf("    Event code %d (%s)\n", j, names[i] ? (names[i][j] ? names[i][j] : "?") : "?");
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
}


int getTouchSample(int *rawX, int *rawY, int *rawPressure)
{
	int i;
        /* how many bytes were read */
        size_t rb;
        /* the events (up to 64 at once) */
        struct input_event ev[64];
	static int Last_event=0;
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

SelectInGroup(int StartButton,int StopButton,int NoButton,int Status)
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

SelectSR(int NoButton)
{
	SelectInGroup(0,4,NoButton,1);
	SR=TabSR[NoButton-0];
	char Param[]="symbolrate";
	char Value[255];
	sprintf(Value,"%d",SR);
	SetConfigParam(PATH_CONFIG,Param,Value);
}

SelectFec(int NoButton)
{
	SelectInGroup(5,9,NoButton,1);
	fec=TabFec[NoButton-5];
	char Param[]="fec";
	char Value[255];
	sprintf(Value,"%d",fec);
	SetConfigParam(PATH_CONFIG,Param,Value);
}

SelectSource(int NoButton,int Status)
{
	SelectInGroup(10,14,NoButton,Status);
	strcpy(ModeInput,TabModeInput[NoButton-10]);
	printf("************** Mode Input = %s\n",ModeInput);
	char Param[]="modeinput";
	SetConfigParam(PATH_CONFIG,Param,ModeInput);
	
}



SelectPTT(int NoButton,int Status)
{
	SelectInGroup(15,16,NoButton,Status);
}

TransmitStart()
{
	printf("Transmit Start\n");
	#define PATH_SCRIPT_A "sudo /home/pi/rpidatv/scripts/a.sh"
	if((strcmp(ModeInput,TabModeInput[0])==0)||(strcmp(ModeInput,TabModeInput[1])==0)) //CAM
	{
		printf("DISPLAY OFF \n");
		IsDisplayOn=0; 
		finish();
		
		system("v4l2-ctl --overlay=1");
	}
	
	system(PATH_SCRIPT_A);
	
}

TransmitStop()
{
	printf("Transmit Stop\n");
	system("sudo killall rpidatv >/dev/null 2>/dev/null");
	system("sudo killall ffmpeg >/dev/null 2>/dev/null");
	
	system("v4l2-ctl --overlay=0");

}

ReceiveStart()
{
		#define PATH_SCRIPT_LEAN "sudo /home/pi/rpidatv/scripts/leandvb2video.sh"
	//system("sudo SDL_VIDEODRIVER=fbcon SDL_FBDEV=/dev/fb0 mplayer -ao /dev/null -vo sdl  /home/pi/rpidatv/video/mire250.ts &");
	system(PATH_SCRIPT_LEAN);
}

ReceiveStop()
{
	system("sudo killall leandvb");
	system("sudo killall mplayer");
}
// wait for a specific character 
void waituntil(int w,int h,int endchar) {
    int key;
int rawX, rawY, rawPressure,i;

	int Toggle=0;
    for (;;) {
	//Start(w,h);
	if (getTouchSample(&rawX, &rawY, &rawPressure)==0) continue;
	printf("x=%x y=%x\n",rawX,rawY);
	if(IsDisplayOn==0)
	{
				
				printf("Display ON\n");
				TransmitStop();
				ReceiveStop();
				init(&wscreen, &hscreen);
				Start(wscreen,hscreen);
				IsDisplayOn=1;			
				
				SelectPTT(15,0);
				SelectPTT(16,0);
				UpdateWindow();
				//usleep(500000);
				continue;
				
	}	
	for(i=0;i<IndexButtonInArray;i++)
	{
		if(IsButtonPushed(i,rawX,rawY)==1)
		{
			
			printf("Button Event %d\n",i);
			if((i>=0)&&(i<=4)) //SR			
			{
				SelectSR(i);
			}
			if((i>=5)&&(i<=9)) //FEC			
			{
				SelectFec(i);
			}
			if((i>=10)&&(i<=14)) //Source			
			{
				SelectSource(i,1);
			}
			if((i>=15)&&(i<=16)) //Source			
			{
				
				printf("Status %d\n",GetButtonStatus(i));
				if((i==15)&&(GetButtonStatus(i)==0))
				{
					
					
					
					usleep(500000);
					SelectPTT(i,1);
					UpdateWindow();
					TransmitStart();
					break;	
				}
				if((i==15)&&(GetButtonStatus(i)==1))
				{
					
						
					TransmitStop();
					usleep(500000);
					SelectPTT(i,0);
					UpdateWindow();	
					break;
				}
				if(i==16)
				{
					printf("DISPLAY OFF \n");
					finish();
					ReceiveStart();
					
					IsDisplayOn=0;
					usleep(500000);
				}

			}
			if(IsDisplayOn==1)
			{
				UpdateWindow();
	//			DrawButton(i);	

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



// main initializes the system and shows the picture. 
// Exit and clean up when you hit [RETURN].
int main(int argc, char **argv) {
	int n;
	char *progname = argv[0];
	saveterm();
	init(&wscreen, &hscreen);
	rawterm();

	

	if (openTouchScreen() == 1)
		perror("error opening touch screen");
	int screenXmax, screenXmin;
	int screenYmax, screenYmin;
	getTouchScreenDetails(&screenXmin,&screenXmax,&screenYmin,&screenYmax);
	scaleXvalue = ((float)screenXmax-screenXmin) / wscreen;
	printf ("X Scale Factor = %f\n", scaleXvalue);
	scaleYvalue = ((float)screenYmax-screenYmin) / hscreen;
	printf ("Y Scale Factor = %f\n", scaleYvalue);

	int wbuttonsize=wscreen/5;	
	int hbuttonsize=hscreen/5;


	int button=AddButton(0*wbuttonsize+20,0+hbuttonsize*0+20,wbuttonsize*0.9,hbuttonsize*0.9);
	color_t Col;
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"SR125",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"SR125",&Col);

	button=AddButton(1*wbuttonsize+20,hbuttonsize*0+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"SR250",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"SR250",&Col);

	button=AddButton(2*wbuttonsize+20,hbuttonsize*0+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"SR333",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"SR333",&Col);

	button=AddButton(3*wbuttonsize+20,hbuttonsize*0+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"SR500",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"SR500",&Col);

	button=AddButton(4*wbuttonsize+20,hbuttonsize*0+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"SR1000",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"SR1000",&Col);
// FEC	
	button=AddButton(0*wbuttonsize+20,hbuttonsize*1+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"FEC 1/2",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"FEC 1/2",&Col);
	
	button=AddButton(1*wbuttonsize+20,hbuttonsize*1+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"FEC 2/3",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"FEC 2/3",&Col);
	
button=AddButton(2*wbuttonsize+20,hbuttonsize*1+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"FEC 3/4",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"FEC 3/4",&Col);
	
button=AddButton(3*wbuttonsize+20,hbuttonsize*1+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"FEC 5/6",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"FEC 5/6",&Col);

button=AddButton(4*wbuttonsize+20,hbuttonsize*1+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"FEC 7/8",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"FEC 7/8",&Col);

//SOURCE
button=AddButton(0*wbuttonsize+20,hbuttonsize*2+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;	
	AddButtonStatus(button,"CAM MPEG2",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"CAM MPEG2",&Col);
	
	button=AddButton(1*wbuttonsize+20,hbuttonsize*2+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;	
	AddButtonStatus(button,"CAM H264",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"CAM H264",&Col);	

char PictureName[255];
	//strcpy(PictureName,ImageFolder);
	GetNextPicture(PictureName);
button=AddButton(2*wbuttonsize+20,hbuttonsize*2+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"Patern",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,PictureName,&Col);
	

button=AddButton(3*wbuttonsize+20,hbuttonsize*2+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"TS File",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"Video Name",&Col);

button=AddButton(4*wbuttonsize+20,hbuttonsize*2+20,wbuttonsize*0.9,hbuttonsize*0.9);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"Carrier",&Col);
	Col.r=0;Col.g=128;Col.b=0;
	AddButtonStatus(button,"Carrier",&Col);

//TRANSMIT

button=AddButton(0*wbuttonsize+20,hbuttonsize*3+20,wbuttonsize*1.2,hbuttonsize*1.2);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"TX   ",&Col);
	Col.r=255;Col.g=0;Col.b=0;
	AddButtonStatus(button,"TX ON",&Col);		

button=AddButton(1*wbuttonsize*3+20,hbuttonsize*3+20,wbuttonsize*1.2,hbuttonsize*1.2);
	Col.r=0;Col.g=0;Col.b=128;
	AddButtonStatus(button,"RX   ",&Col);
	Col.r=0;Col.g=255;Col.b=0;
	AddButtonStatus(button,"RX ON",&Col);		
	
	Start(wscreen,hscreen);
	IsDisplayOn=1;

	
	char Param[]="symbolrate";
	char Value[255];
	GetConfigParam(PATH_CONFIG,Param,Value);
	
	SR=atoi(Value);
	switch(SR)
	{
		case 125:SelectSR(0);break;
		case 250:SelectSR(1);break;
		case 333:SelectSR(2);break;
		case 500:SelectSR(3);break;
		case 1000:SelectSR(4);break;
	}

	strcpy(Param,"fec");
	strcpy(Value,"");
	GetConfigParam(PATH_CONFIG,Param,Value);
	printf("Value=%s %s\n",Value,"Fec");
	fec=atoi(Value);
	switch(fec)
	{
		case 1:SelectFec(5);break;
		case 2:SelectFec(6);break;
		case 3:SelectFec(7);break;
		case 5:SelectFec(8);break;
		case 7:SelectFec(9);break;
	}
	
	strcpy(Param,"modeinput");
	GetConfigParam(PATH_CONFIG,Param,Value);
	strcpy(ModeInput,Value);
	if(strcmp(Value,"CAMH264")==0)
	{
	     SelectSource(11,1);
	     
	}
	if(strcmp(Value,"CAMMPEG-2")==0)
	{
	 	SelectSource(10,1);

	}
	if(strcmp(Value,"PATERNAUDIO")==0)
	{
	 	SelectSource(12,1);

	}
	if(strcmp(Value,"CARRIER")==0)
	{
		SelectSource(13,1);
	}
	UpdateWindow();
	
	// RESIZE JPEG TO BE DONE
	/*char PictureName[255];
	strcpy(PictureName,ImageFolder);
	GetNextPicture(PictureName);
	
	Image(0,0,300,200,PictureName);

	End();
	*/
	waituntil(wscreen,hscreen,0x1b);
	restoreterm();
	finish();
	return 0;
}
