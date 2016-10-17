extern "C"
{
#include <rfb/rfbclient.h>
}

class VncClient {

public:
    VncClient(char *IP,char* Password);

    ~VncClient();
    void GetDisplaySize(int& Width,int& Height,int& Rotate);  
    void SetOmxBuffer(unsigned char* Buffer);
   int GetPicture(int fps);
 

private:
 int32_t DisplayWidth=0;
 int32_t DisplayHeight=0;

static rfbBool  resize(rfbClient* client);
static void  update(rfbClient* client,int x,int y,int w,int h) ;
static char* GetPassword(rfbClient* client);
rfbClient* client;
unsigned char *OmxBuffer=0;
};

