

#include <getopt.h>
#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vncclient.h"

char* MyPassword;

//http://svn.openscenegraph.org/osg/OpenSceneGraph/trunk/src/osgPlugins/vnc/ReaderWriterVNC.cpp
rfbBool VncClient::resize(rfbClient* client) {
	

	/* VncClient::DisplayWidth = (client->width>>5)<<5;
	 VncClient::DisplayHeight = (client->height>>4)<<4;
	*/
	return TRUE;
}


void VncClient::update(rfbClient* client,int x,int y,int w,int h)
 {
	
}


char* VncClient::GetPassword(rfbClient* client)
{
	char* Password=(char*)malloc(255);
	strcpy(Password,MyPassword);
	return Password;
}

VncClient::VncClient(char *IP,char* Password)
{
	client = rfbGetClient(8,3,4);
	DisplayWidth=0;
	DisplayHeight=0;
	 //client->MallocFrameBuffer=&VncClient::resize;
	 client->GotFrameBufferUpdate=update;
	
	client->GetPassword=GetPassword;
	MyPassword=Password;
	char *argv[2];
	argv[0]="rpidatv";
	argv[1]=IP;
	int argc=2;
	if(!rfbInitClient(client,&argc,argv))
	{
		printf("InitClient Failed\n");
	};

}

VncClient::~VncClient()
{
	printf("VNC Clean todo\n");	
	//rfbClientCleanup(client);
}

void VncClient::GetDisplaySize(int& Width,int& Height,int& Rotate)
{
	//printf("Enter DisplaySize\n");	
	while(DisplayWidth==0)
	{	sleep(1);
		if(!WaitForMessage(client,50000)) continue;
		if(!HandleRFBServerMessage(client)) printf("Handle Error");
		
		if((client->width!=0)&&(client->height!=0))
		{
			DisplayWidth = ((client->width>>5))<<5;
			DisplayHeight = ((client->height>>4)+1)<<4;
			client->width=DisplayWidth;
			client->height=DisplayHeight;
		}
	}
	printf("Origin FB = %d * %d\n",client->width,client->height);
	Width=DisplayWidth;
	Height=DisplayHeight;
}  

void VncClient::SetOmxBuffer(unsigned char* Buffer)
{
	client->frameBuffer=Buffer;
}

int VncClient::GetPicture(int fps=25)
{
	int result;
	struct timespec gettime_now,last_time;
	long time_difference;

	

	 clock_gettime(CLOCK_REALTIME, &last_time);
	

	result=WaitForMessage(client,500000);
	if(result<0) {printf("Message Failed\n");return 0;}
	if(result==0) 
	{
		return 0; 	
		
	}
	else
	{
		if(!HandleRFBServerMessage(client)) printf("VNC KAPUT\n");
	}

	clock_gettime(CLOCK_REALTIME, &gettime_now);
	time_difference = gettime_now.tv_nsec - last_time.tv_nsec;
	if(time_difference<0) time_difference+=1E9;
	int FrameDiff=(int)((time_difference/1000)/(1000000/fps));
	//printf("Diff=%ld us Frame %d\n",time_difference/1000,FrameDiff);
	//int FrameDiff=0;
	return FrameDiff;
	
}
