

#include <getopt.h>
#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "grabdisplay.h"

GrabDisplay::GrabDisplay(int displayNumber)
{
	//bcm_host_init();
	displayHandle=vc_dispmanx_display_open(displayNumber);

	if (displayHandle == 0)
        {
        fprintf(stderr,
                " unable to open display %d\n",
                
                displayNumber);

        exit(EXIT_FAILURE);
    	}

	int result = 0;
	DISPMANX_MODEINFO_T modeInfo;
	result = vc_dispmanx_display_get_info(displayHandle, &modeInfo);

	if (result != 0)
   	 {
     	   fprintf(stderr, " unable to get display information\n");
     	   exit(EXIT_FAILURE);
 	  }
	 DisplayWidth = (modeInfo.width>>5)<<5;
	 DisplayHeight = (modeInfo.height>>4)<<4;
	dmxPitch=DisplayWidth*4; //4 BYTES IN RGBA //2 BYTE PAR PIXEL IN RGB565
	    resourceHandle = vc_dispmanx_resource_create(imageType,
                                                 DisplayWidth,
                                                 DisplayHeight,
                                                 &vcImagePtr);
}
GrabDisplay::~GrabDisplay()
{
	vc_dispmanx_resource_delete(resourceHandle);
    vc_dispmanx_display_close(displayHandle);
}

void GrabDisplay::GetDisplaySize(int& Width,int& Height,int& Rotate)
{	
	Width=DisplayWidth;
	Height=DisplayHeight;
	Rotate=0;
}  

void GrabDisplay::SetOmxBuffer(unsigned char* Buffer)
{
	OmxBuffer=Buffer;
}

void GrabDisplay::GetPicture()
{
// FOR YUV420
//https://www.raspberrypi.org/forums/viewtopic.php?t=45711&p=481480
//https://github.com/raspberrypi/firmware/issues/235
	 int result = vc_dispmanx_snapshot(displayHandle,
                                  resourceHandle,
                                  DISPMANX_NO_ROTATE);
	 if (result != 0)
   	 {
  	      vc_dispmanx_resource_delete(resourceHandle);
  	      vc_dispmanx_display_close(displayHandle);
	
   	     fprintf(stderr, " vc_dispmanx_snapshot() failed\n");
  	      exit(EXIT_FAILURE);
 	  }
	
	VC_RECT_T rect;
    	result = vc_dispmanx_rect_set(&rect, 0, 0, DisplayWidth, DisplayHeight);

    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        fprintf(stderr, " vc_dispmanx_rect_set() failed\n");
        exit(EXIT_FAILURE);
    }

    result = vc_dispmanx_resource_read_data(resourceHandle,
                                            &rect,
                                            OmxBuffer,
                                            dmxPitch);





	
}
