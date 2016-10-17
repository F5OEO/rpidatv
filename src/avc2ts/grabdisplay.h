
#include "bcm_host.h"

class GrabDisplay {

public:
    GrabDisplay(int displayNumber);

    ~GrabDisplay();
    void GetDisplaySize(int& Width,int& Height,int& Rotate);  
    void SetOmxBuffer(unsigned char* Buffer);
   void GetPicture();

private:
VC_IMAGE_TYPE_T imageType = VC_IMAGE_RGBA32;//VC_IMAGE_YUV420;//VC_IMAGE_RGBA32;//VC_IMAGE_RGB565;
DISPMANX_DISPLAY_HANDLE_T displayHandle;
 DISPMANX_RESOURCE_HANDLE_T resourceHandle;
uint32_t vcImagePtr = 0;
int32_t DisplayWidth;
  int32_t DisplayHeight ;
int32_t dmxPitch;
unsigned char *OmxBuffer=0;
};

