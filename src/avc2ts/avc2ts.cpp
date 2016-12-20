//Original credits to Artem Zuikov 
//clone from https://github.com/4ertus2/rpi-cctv
// Adding other functionnalities F5OEO Evariste - evaristec@gmail.com


#ifndef OMX_SKIP64BIT
#define OMX_SKIP64BIT
#endif

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <string>
#include <vector>
#include <iostream>
#include <inttypes.h>
#include <math.h>


#include "bcm_host.h"

#include <interface/vcos/vcos_semaphore.h>
#include <interface/vmcs_host/vchost.h>


#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Broadcom.h>

extern "C" {
#include "libmpegts/libmpegts.h"
}

#include <arpa/inet.h>
#include <netinet/in.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "webcam.h"
#include "grabdisplay.h"
#include "vncclient.h"

//#include <linux/videodev2.h>

#define PROGRAM_VERSION "1.0.0"

// Problem with delay increasing : https://www.raspberrypi.org/forums/viewtopic.php?f=43&t=133446
// Introductio to IL Component : http://fr.slideshare.net/pchethan/understanding-open-max-il-18376762
// Camera modes : http://picamera.readthedocs.io/en/latest/fov.html
// Understand Low latency : http://www.design-reuse.com/articles/33005/understanding-latency-in-video-compression-systems.html


namespace
{
    static const char * format2str(OMX_VIDEO_CODINGTYPE c)
    {
        switch (c)
        {
            case OMX_VIDEO_CodingUnused:        return "not used";
            case OMX_VIDEO_CodingAutoDetect:    return "autodetect";
            case OMX_VIDEO_CodingMPEG2:         return "MPEG2";
            case OMX_VIDEO_CodingH263:          return "H.263";
            case OMX_VIDEO_CodingMPEG4:         return "MPEG4";
            case OMX_VIDEO_CodingWMV:           return "Windows Media Video";
            case OMX_VIDEO_CodingRV:            return "RealVideo";
            case OMX_VIDEO_CodingAVC:           return "H.264/AVC";
            case OMX_VIDEO_CodingMJPEG:         return "Motion JPEG";
            case OMX_VIDEO_CodingVP6:           return "VP6";
            case OMX_VIDEO_CodingVP7:           return "VP7";
            case OMX_VIDEO_CodingVP8:           return "VP8";
            case OMX_VIDEO_CodingYUV:           return "Raw YUV video";
            case OMX_VIDEO_CodingSorenson:      return "Sorenson";
            case OMX_VIDEO_CodingTheora:        return "OGG Theora";
            case OMX_VIDEO_CodingMVC:           return "H.264/MVC";
            default:
                std::cerr << "unknown OMX_VIDEO_CODINGTYPE: " << c << std::endl;
                return "unknown";
        }
    }

    static const char * format2str(OMX_COLOR_FORMATTYPE c)
    {
        switch (c)
        {
            case OMX_COLOR_FormatUnused:                 return "OMX_COLOR_FormatUnused: not used";
            case OMX_COLOR_FormatMonochrome:             return "OMX_COLOR_FormatMonochrome";
            case OMX_COLOR_Format8bitRGB332:             return "OMX_COLOR_Format8bitRGB332";
            case OMX_COLOR_Format12bitRGB444:            return "OMX_COLOR_Format12bitRGB444";
            case OMX_COLOR_Format16bitARGB4444:          return "OMX_COLOR_Format16bitARGB4444";
            case OMX_COLOR_Format16bitARGB1555:          return "OMX_COLOR_Format16bitARGB1555";
            case OMX_COLOR_Format16bitRGB565:            return "OMX_COLOR_Format16bitRGB565";
            case OMX_COLOR_Format16bitBGR565:            return "OMX_COLOR_Format16bitBGR565";
            case OMX_COLOR_Format18bitRGB666:            return "OMX_COLOR_Format18bitRGB666";
            case OMX_COLOR_Format18bitARGB1665:          return "OMX_COLOR_Format18bitARGB1665";
            case OMX_COLOR_Format19bitARGB1666:          return "OMX_COLOR_Format19bitARGB1666";
            case OMX_COLOR_Format24bitRGB888:            return "OMX_COLOR_Format24bitRGB888";
            case OMX_COLOR_Format24bitBGR888:            return "OMX_COLOR_Format24bitBGR888";
            case OMX_COLOR_Format24bitARGB1887:          return "OMX_COLOR_Format24bitARGB1887";
            case OMX_COLOR_Format25bitARGB1888:          return "OMX_COLOR_Format25bitARGB1888";
            case OMX_COLOR_Format32bitBGRA8888:          return "OMX_COLOR_Format32bitBGRA8888";
            case OMX_COLOR_Format32bitARGB8888:          return "OMX_COLOR_Format32bitARGB8888";
            case OMX_COLOR_FormatYUV411Planar:           return "OMX_COLOR_FormatYUV411Planar";
            case OMX_COLOR_FormatYUV411PackedPlanar:     return "OMX_COLOR_FormatYUV411PackedPlanar: Planes fragmented when a frame is split in multiple buffers";
            case OMX_COLOR_FormatYUV420Planar:           return "OMX_COLOR_FormatYUV420Planar: Planar YUV, 4:2:0 (I420)";
            case OMX_COLOR_FormatYUV420PackedPlanar:     return "OMX_COLOR_FormatYUV420PackedPlanar: Planar YUV, 4:2:0 (I420), planes fragmented when a frame is split in multiple buffers";
            case OMX_COLOR_FormatYUV420SemiPlanar:       return "OMX_COLOR_FormatYUV420SemiPlanar, Planar YUV, 4:2:0 (NV12), U and V planes interleaved with first U value";
            case OMX_COLOR_FormatYUV422Planar:           return "OMX_COLOR_FormatYUV422Planar";
            case OMX_COLOR_FormatYUV422PackedPlanar:     return "OMX_COLOR_FormatYUV422PackedPlanar: Planes fragmented when a frame is split in multiple buffers";
            case OMX_COLOR_FormatYUV422SemiPlanar:       return "OMX_COLOR_FormatYUV422SemiPlanar";
            case OMX_COLOR_FormatYCbYCr:                 return "OMX_COLOR_FormatYCbYCr";
            case OMX_COLOR_FormatYCrYCb:                 return "OMX_COLOR_FormatYCrYCb";
            case OMX_COLOR_FormatCbYCrY:                 return "OMX_COLOR_FormatCbYCrY";
            case OMX_COLOR_FormatCrYCbY:                 return "OMX_COLOR_FormatCrYCbY";
            case OMX_COLOR_FormatYUV444Interleaved:      return "OMX_COLOR_FormatYUV444Interleaved";
            case OMX_COLOR_FormatRawBayer8bit:           return "OMX_COLOR_FormatRawBayer8bit";
            case OMX_COLOR_FormatRawBayer10bit:          return "OMX_COLOR_FormatRawBayer10bit";
            case OMX_COLOR_FormatRawBayer8bitcompressed: return "OMX_COLOR_FormatRawBayer8bitcompressed";
            case OMX_COLOR_FormatL2:                     return "OMX_COLOR_FormatL2";
            case OMX_COLOR_FormatL4:                     return "OMX_COLOR_FormatL4";
            case OMX_COLOR_FormatL8:                     return "OMX_COLOR_FormatL8";
            case OMX_COLOR_FormatL16:                    return "OMX_COLOR_FormatL16";
            case OMX_COLOR_FormatL24:                    return "OMX_COLOR_FormatL24";
            case OMX_COLOR_FormatL32:                    return "OMX_COLOR_FormatL32";
            case OMX_COLOR_FormatYUV420PackedSemiPlanar: return "OMX_COLOR_FormatYUV420PackedSemiPlanar: Planar YUV, 4:2:0 (NV12), planes fragmented when a frame is split in multiple buffers, U and V planes interleaved with first U value";
            case OMX_COLOR_FormatYUV422PackedSemiPlanar: return "OMX_COLOR_FormatYUV422PackedSemiPlanar: Planes fragmented when a frame is split in multiple buffers";
            case OMX_COLOR_Format18BitBGR666:            return "OMX_COLOR_Format18BitBGR666";
            case OMX_COLOR_Format24BitARGB6666:          return "OMX_COLOR_Format24BitARGB6666";
            case OMX_COLOR_Format24BitABGR6666:          return "OMX_COLOR_Format24BitABGR6666";
            case OMX_COLOR_Format32bitABGR8888:          return "OMX_COLOR_Format32bitABGR8888";
            case OMX_COLOR_Format8bitPalette:            return "OMX_COLOR_Format8bitPalette";
            case OMX_COLOR_FormatYUVUV128:               return "OMX_COLOR_FormatYUVUV128";
            case OMX_COLOR_FormatRawBayer12bit:          return "OMX_COLOR_FormatRawBayer12bit";
            case OMX_COLOR_FormatBRCMEGL:                return "OMX_COLOR_FormatBRCMEGL";
            case OMX_COLOR_FormatBRCMOpaque:             return "OMX_COLOR_FormatBRCMOpaque";
            case OMX_COLOR_FormatYVU420PackedPlanar:     return "OMX_COLOR_FormatYVU420PackedPlanar";
            case OMX_COLOR_FormatYVU420PackedSemiPlanar: return "OMX_COLOR_FormatYVU420PackedSemiPlanar";
            default:
                std::cerr << "unknown OMX_COLOR_FORMATTYPE: " << c << std::endl;
                return "unknown";
        }
    }

    static void dump_portdef(OMX_PARAM_PORTDEFINITIONTYPE* portdef)
    {
        fprintf(stderr, "Port %d is %s, %s, buffers wants:%d needs:%d, size:%d, pop:%d, aligned:%d\n",
            portdef->nPortIndex,
            (portdef->eDir ==  OMX_DirInput ? "input" : "output"),
            (portdef->bEnabled == OMX_TRUE ? "enabled" : "disabled"),
            portdef->nBufferCountActual,
            portdef->nBufferCountMin,
            portdef->nBufferSize,
            portdef->bPopulated,
            portdef->nBufferAlignment);

        OMX_VIDEO_PORTDEFINITIONTYPE *viddef = &portdef->format.video;
        OMX_IMAGE_PORTDEFINITIONTYPE *imgdef = &portdef->format.image;

        switch (portdef->eDomain)
        {
            case OMX_PortDomainVideo:
                fprintf(stderr, "Video type:\n"
                    "\tWidth:\t\t%d\n"
                    "\tHeight:\t\t%d\n"
                    "\tStride:\t\t%d\n"
                    "\tSliceHeight:\t%d\n"
                    "\tBitrate:\t%d\n"
                    "\tFramerate:\t%.02f\n"
                    "\tError hiding:\t%s\n"
                    "\tCodec:\t\t%s\n"
                    "\tColor:\t\t%s\n",
                    viddef->nFrameWidth,
                    viddef->nFrameHeight,
                    viddef->nStride,
                    viddef->nSliceHeight,
                    viddef->nBitrate,
                    ((float)viddef->xFramerate / (float)65536),
                    (viddef->bFlagErrorConcealment == OMX_TRUE ? "yes" : "no"),
                    format2str(viddef->eCompressionFormat),
                    format2str(viddef->eColorFormat));
                break;
            case OMX_PortDomainImage:
                fprintf(stderr, "Image type:\n"
                    "\tWidth:\t\t%d\n"
                    "\tHeight:\t\t%d\n"
                    "\tStride:\t\t%d\n"
                    "\tSliceHeight:\t%d\n"
                    "\tError hiding:\t%s\n"
                    "\tCodec:\t\t%s\n"
                    "\tColor:\t\t%s\n",
                    imgdef->nFrameWidth,
                    imgdef->nFrameHeight,
                    imgdef->nStride,
                    imgdef->nSliceHeight,
                    (imgdef->bFlagErrorConcealment == OMX_TRUE ? "yes" : "no"),
                    format2str( (OMX_VIDEO_CODINGTYPE) imgdef->eCompressionFormat ),
                    format2str(imgdef->eColorFormat));
                break;
            default:
                break;
        }
    }

    const char * eventType2Str(OMX_EVENTTYPE eEvent)
    {
        switch (eEvent)
        {
            case OMX_EventCmdComplete:          return "OMX_EventCmdComplete";
            case OMX_EventError:                return "OMX_EventError";
            case OMX_EventMark:                 return "OMX_EventMark";
            case OMX_EventPortSettingsChanged:  return "OMX_EventPortSettingsChanged";
            case OMX_EventBufferFlag:           return "OMX_EventBufferFlag";
            case OMX_EventResourcesAcquired:    return "OMX_EventResourcesAcquired";
            case OMX_EventComponentResumed:     return "OMX_EventComponentResumed";
            case OMX_EventDynamicResourcesAvailable: return "OMX_EventDynamicResourcesAvailable";
            case OMX_EventPortFormatDetected:   return "OMX_EventPortFormatDetected";
            case OMX_EventKhronosExtensions:    return "OMX_EventKhronosExtensions";
            case OMX_EventVendorStartUnused:    return "OMX_EventVendorStartUnused";
            case OMX_EventParamOrConfigChanged: return "OMX_EventParamOrConfigChanged";
            default:
                break;
        };

        return nullptr;
    }

    static void printEvent(const char * compName, OMX_HANDLETYPE hComponent, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2)
    {
        const char * strEvent = eventType2Str(eEvent);
        if (strEvent)
            fprintf(stderr, "%s (%p) %s, data: %d, %d\n", compName, hComponent, strEvent, nData1, nData2);
        else
            fprintf(stderr, "%s (%p) 0x%08x, data: %d, %d\n", compName, hComponent, eEvent, nData1, nData2);
    }
}

namespace broadcom
{
    // TODO: add all

    typedef enum
    {
        VIDEO_SCHEDULER = 10,
        SOURCE = 20,
        RESIZER = 60,
        CAMERA = 70,
        CLOCK = 80,
        VIDEO_RENDER = 90,
        VIDEO_DECODER = 130,
        VIDEO_ENCODER = 200,
        EGL_RENDER = 220,
        NULL_SINK = 240,
        VIDEO_SPLITTER = 250,
	IMAGE_ENCODE = 340
    } ComponentType;

    static const char * componentType2name(ComponentType type)
    {
        switch (type)
        {
            case VIDEO_SCHEDULER:   return "OMX.broadcom.video_scheduler";
            case SOURCE:            return "OMX.broadcom.source";
            case RESIZER:           return "OMX.broadcom.resize";
            case CAMERA:            return "OMX.broadcom.camera";
            case CLOCK:             return "OMX.broadcom.clock";
            case VIDEO_RENDER:      return "OMX.broadcom.video_render";
            case VIDEO_DECODER:     return "OMX.broadcom.video_decode";
            case VIDEO_ENCODER:     return "OMX.broadcom.video_encode";
            case EGL_RENDER:        return "OMX.broadcom.egl_render";
            case NULL_SINK:         return "OMX.broadcom.null_sink";
            case VIDEO_SPLITTER:    return "OMX.broadcom.video_splitter";
	    case IMAGE_ENCODE:	return "OMX.broadcom.image_encode";
        }

        return nullptr;
    }

    static unsigned componentPortsCount(ComponentType type)
    {
        switch (type)
        {
            case VIDEO_SCHEDULER:   return 3;
            case SOURCE:            return 1;
            case RESIZER:           return 2;
            case CAMERA:            return 4;
            case CLOCK:             return 6;
            case VIDEO_RENDER:      return 1;
            case VIDEO_DECODER:     return 2;
            case VIDEO_ENCODER:     return 2;
            case EGL_RENDER:        return 2;
            case NULL_SINK:         return 3;
            case VIDEO_SPLITTER:    return 5;
		 case IMAGE_ENCODE: return 2;
        }

        return 0;
    }

    struct VcosSemaphore
    {
        VcosSemaphore(const char * name)
        {
            if (vcos_semaphore_create(&sem_, name, 1) != VCOS_SUCCESS)
                throw "Failed to create handler lock semaphore";
        }

        ~VcosSemaphore()
        {
            vcos_semaphore_delete(&sem_);
        }

        VCOS_STATUS_T wait() { return vcos_semaphore_wait(&sem_); }
        VCOS_STATUS_T post() { return vcos_semaphore_post(&sem_); }

    private:
        VCOS_SEMAPHORE_T sem_;
    };

    class VcosLock
    {
    public:
        VcosLock(VcosSemaphore * sem)
        :   sem_(sem)
        {
            sem_->wait();
        }

        ~VcosLock()
        {
            sem_->post();
        }

    private:
        VcosSemaphore * sem_;
    };
}

namespace rpi_omx
{
    typedef broadcom::ComponentType ComponentType;
    using broadcom::componentType2name;
    using broadcom::componentPortsCount;

    using broadcom::VcosSemaphore;
    using Lock = broadcom::VcosLock;

    VcosSemaphore * pSemaphore;

    //

    static OMX_ERRORTYPE callback_EventHandler(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_EVENTTYPE eEvent,
        OMX_U32 nData1,
        OMX_U32 nData2,
        OMX_PTR pEventData);

    static OMX_ERRORTYPE callback_EmptyBufferDone(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_BUFFERHEADERTYPE * pBuffer);

    static OMX_ERRORTYPE callback_FillBufferDone(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_BUFFERHEADERTYPE * pBuffer);

    OMX_CALLBACKTYPE cbsEvents = {
        .EventHandler = callback_EventHandler,
        .EmptyBufferDone = callback_EmptyBufferDone,
        .FillBufferDone = callback_FillBufferDone
    };

    //

    ///
    class OMXExeption
    {
    public:
        static const unsigned MAX_LEN = 512;

        OMXExeption(OMX_ERRORTYPE errCode, const char * file, unsigned line, const char * msg = nullptr)
        :   errCode_(errCode)
        {
            if (msg && msg[0])
                snprintf(msg_, MAX_LEN, "%s:%d OpenMAX IL error: 0x%08x. %s", file, line, errCode, msg);
            else
                snprintf(msg_, MAX_LEN, "%s:%d OpenMAX IL error: 0x%08x", file, line, errCode);
        }

        OMX_ERRORTYPE code() const { return errCode_; }
        const char * what() const { return msg_; }

        static void die(OMX_ERRORTYPE error, const char * str)
        {
            const char * errStr = omxErr2str(error);
            fprintf(stderr, "OMX error: %s: 0x%08x %s\n", str, error, errStr);
            exit(1);
        }

    private:
        OMX_ERRORTYPE errCode_;
        char msg_[MAX_LEN];

        static const char * omxErr2str(OMX_ERRORTYPE error)
        {
            switch (error)
            {
                case OMX_ErrorNone:                     return "OMX_ErrorNone";
                case OMX_ErrorBadParameter:             return "OMX_ErrorBadParameter";
                case OMX_ErrorIncorrectStateOperation:  return "OMX_ErrorIncorrectStateOperation";
                case OMX_ErrorIncorrectStateTransition: return "OMX_ErrorIncorrectStateTransition";
                case OMX_ErrorInsufficientResources:    return "OMX_ErrorInsufficientResources";
                case OMX_ErrorBadPortIndex:             return "OMX_ErrorBadPortIndex";
                case OMX_ErrorHardware:                 return "OMX_ErrorHardware";
                // ...

                default:
                    break;
            }

            return "";
        }
    };

#define ERR_OMX(err, msg) if ((err) != OMX_ErrorNone) throw OMXExeption(err, __FILE__, __LINE__, msg)

    ///
    struct VideoFromat
    {
        typedef enum
        {
            RATIO_4x3,
            RATIO_16x9
        } Ratio;

        unsigned width;
        unsigned height;
        unsigned framerate;
        Ratio ratio;
        bool fov; //Field of view
    };

    
// CAMERA NATIVE MODE V1
//#	Resolution	Aspect Ratio	Framerates	Video	Image	FoV	Binning
//1	1920x1080	16:9	1-30fps	x	 	Partial	None
//2	2592x1944	4:3	1-15fps	x	x	Full	None
//3	2592x1944	4:3	0.1666-1fps	x	x	Full	None
//4	1296x972	4:3	1-42fps	x	 	Full	2x2
//5	1296x730	16:9	1-49fps	x	 	Full	2x2
//6	640x480	4:3	42.1-60fps	x	 	Full	4x4
//7	640x480	4:3	60.1-90fps	x	 	Full	4x4

// CAMERA NATIVE MODE V2
//#	Resolution	Aspect Ratio	Framerates	Video	Image	FoV	Binning
//1	1920x1080	16:9	0.1-30fps	x	 	Partial	None
//2	3280x2464	4:3	0.1-15fps	x	x	Full	None
//3	3280x2464	4:3	0.1-15fps	x	x	Full	None
//4	1640x1232	4:3	0.1-40fps	x	 	Full	2x2
//5	1640x922	16:9	0.1-40fps	x	 	Full	2x2
//6	1280x720	16:9	40-90fps	x	 	Partial	2x2
//7	640x480		4:3	40-90fps	x	 	Partial	2x2

    static const VideoFromat VF_1920x1080 = { 1920, 1080, 25, VideoFromat::RATIO_16x9, false };
    //static const VideoFromat VF_2560x1920 = { 2560, 1920, 0, VideoFromat::RATIO_4x3, true };
    static const VideoFromat VF_1280x960 = { 1280, 960, 25, VideoFromat::RATIO_4x3, true };
    static const VideoFromat VF_1280x720 = { 1280, 720, 25, VideoFromat::RATIO_16x9, true };
    static const VideoFromat VF_640x480 = { 640, 480, 25, VideoFromat::RATIO_4x3, true };

    static const VideoFromat VF_RESIZED_352x288 = { 352, 288, 25, VideoFromat::RATIO_4x3, true };
    static const VideoFromat VF_RESIZED_640x480 = VF_640x480;
    static const VideoFromat VF_RESIZED_320x240 = { 320, 240, 15, VideoFromat::RATIO_4x3, true };
    static const VideoFromat VF_RESIZED_256x192 = { 256, 192, 25, VideoFromat::RATIO_4x3, true };
    static const VideoFromat VF_RESIZED_160x120 = { 160, 120, 25, VideoFromat::RATIO_4x3, true };
    static const VideoFromat VF_RESIZED_128x96 = { 128, 96, 25, VideoFromat::RATIO_4x3, true };

    static const VideoFromat VF_RESIZED_960x540 = { 960, 540, 25, VideoFromat::RATIO_16x9, false };
    static const VideoFromat VF_RESIZED_640x360 = { 640, 360, 25, VideoFromat::RATIO_16x9, false };
    static const VideoFromat VF_RESIZED_480x270 = { 480, 270, 25, VideoFromat::RATIO_16x9, false };
    static const VideoFromat VF_RESIZED_384x216 = { 384, 216, 25, VideoFromat::RATIO_16x9, false };
    static const VideoFromat VF_RESIZED_320x180 = { 320, 180, 25, VideoFromat::RATIO_16x9, false };
    static const VideoFromat VF_RESIZED_240x135 = { 240, 135, 25, VideoFromat::RATIO_16x9, false };

    ///
    template <typename T>
    class Parameter
    {
    public:
        Parameter()
        {
            init();
        }

        void init()
        {
            memset(&param_, 0, sizeof(param_));
            param_.nSize = sizeof(param_);
            param_.nVersion.nVersion = OMX_VERSION;
            param_.nVersion.s.nVersionMajor = OMX_VERSION_MAJOR;
            param_.nVersion.s.nVersionMinor = OMX_VERSION_MINOR;
            param_.nVersion.s.nRevision = OMX_VERSION_REVISION;
            param_.nVersion.s.nStep = OMX_VERSION_STEP;
        }

        T& operator * () { return param_; }
        T* operator & () { return &param_; }
        T* operator -> () { return &param_; }

        const T& operator * () const { return param_; }
        const T* operator & () const { return &param_; }
        const T* operator -> () const { return &param_; }

    private:
        T param_;
    };

    ///
    class OMXInit
    {
    public:
        OMXInit()
        {
            ERR_OMX( OMX_Init(), "OMX initalization failed" );
        }

        ~OMXInit()
        {
            try
            {
                ERR_OMX( OMX_Deinit(), "OMX de-initalization failed" );
            }
            catch (const OMXExeption& )
            {
                // TODO
            }
        }
    };

    ///
    class Buffer
    {
    public:
        Buffer()
        :   ppBuffer_(nullptr),
            fillDone_(false)
        {}

        bool filled() const { return fillDone_; }

        void setFilled(bool val = true)
        {
            Lock lock(pSemaphore); // LOCK

            fillDone_ = val;
        }

	bool setDatasize(OMX_U32 Datasize)
	{
		if(Datasize<=allocSize())
		{
			ppBuffer_->nOffset=0;
			ppBuffer_->nFilledLen=Datasize;
			return true;
		}
		return false;	
	}

        OMX_BUFFERHEADERTYPE ** pHeader() { return &ppBuffer_; }
        OMX_BUFFERHEADERTYPE * header() { return ppBuffer_; }
        OMX_U32 flags() const { return ppBuffer_->nFlags; }
        OMX_U32& flags() { return ppBuffer_->nFlags; }

        OMX_U8 * data() { return  ppBuffer_->pBuffer + ppBuffer_->nOffset; }
        OMX_U32 dataSize() const { return ppBuffer_->nFilledLen; }
	
        OMX_U32 allocSize() const { return ppBuffer_->nAllocLen; }
	OMX_U32 TimeStamp() {return ppBuffer_->nTickCount;}

    private:
        OMX_BUFFERHEADERTYPE * ppBuffer_;
        bool fillDone_;
    };

    ///
    class Component
    {
    public:
        OMX_HANDLETYPE& component() { return component_; }
        ComponentType type() const { return type_; }
        const char * name() const { return componentType2name(type_); }
        unsigned numPorts() const { return componentPortsCount(type_); }

        void dumpPort(OMX_U32 nPortIndex, OMX_BOOL dumpFormats = OMX_FALSE)
        {
            Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portdef;
            getPortDefinition(nPortIndex, portdef);

            dump_portdef(&portdef);

            if (dumpFormats)
            {
                Parameter<OMX_VIDEO_PARAM_PORTFORMATTYPE> portformat;
                portformat->nPortIndex = nPortIndex;
                portformat->nIndex = 0;

                std::cerr << "Port " << nPortIndex << " supports these video formats:" << std::endl;

                for (;; portformat->nIndex++)
                {
                    OMX_ERRORTYPE err = OMX_GetParameter(component_, OMX_IndexParamVideoPortFormat, &portformat);
                    if (err != OMX_ErrorNone)
                        break;

                    std::cerr << "\t" << format2str(portformat->eColorFormat)
                        << ", compression: " << format2str(portformat->eCompressionFormat) << std::endl;
                }
            }
        }

        OMX_STATETYPE state()
        {
            OMX_STATETYPE state;
            ERR_OMX( OMX_GetState(component_, &state), "OMX_GetState" );
            return state;
        }

        void switchState(OMX_STATETYPE newState)
        {
            unsigned value = eventState_;
            ERR_OMX( OMX_SendCommand(component_, OMX_CommandStateSet, newState, NULL), "switch state");

            if (! waitValue(&eventState_, value + 1))
                std::cerr << name() << " lost state changed event" << std::endl;
#if 0
            if (! waitStateChanged(newState))
                std::cerr << name() << " state wanted: " << newState << " observed: " << state() << std::endl;
#endif
        }

        unsigned waitCount(OMX_U32 nPortIndex) const { return (nPortIndex == OMX_ALL) ? numPorts() : 1; }

        void enablePort(OMX_U32 nPortIndex = OMX_ALL)
        {
            unsigned value = eventEnabled_;
            ERR_OMX( OMX_SendCommand(component_, OMX_CommandPortEnable, nPortIndex, NULL), "enable port");

            if (! waitValue(&eventEnabled_, value + waitCount(nPortIndex)))
                std::cerr << name() << " port " << nPortIndex << " lost enable port event(s)" << std::endl;
        }

        void disablePort(OMX_U32 nPortIndex = OMX_ALL)
        {
            unsigned value = eventDisabled_;
            ERR_OMX( OMX_SendCommand(component_, OMX_CommandPortDisable, nPortIndex, NULL), "disable port");

            if (! waitValue(&eventDisabled_, value + waitCount(nPortIndex)))
                std::cerr << name() << " port " << nPortIndex << " lost disable port event(s)" << std::endl;
        }

        void flushPort(OMX_U32 nPortIndex = OMX_ALL)
        {
            unsigned value = eventFlushed_;
            ERR_OMX( OMX_SendCommand(component_, OMX_CommandFlush, nPortIndex, NULL), "flush buffers");

            if (! waitValue(&eventFlushed_, value + waitCount(nPortIndex)))
                std::cerr << name() << " port " << nPortIndex << " lost flush port event(s)" << std::endl;
        }

        void getPortDefinition(OMX_U32 nPortIndex, Parameter<OMX_PARAM_PORTDEFINITIONTYPE>& portDef)
        {
            portDef->nPortIndex = nPortIndex;
            ERR_OMX( OMX_GetParameter(component_, OMX_IndexParamPortDefinition, &portDef), "get port definition");
        }

        void setPortDefinition(OMX_U32 nPortIndex, Parameter<OMX_PARAM_PORTDEFINITIONTYPE>& portDef)
        {
            portDef->nPortIndex = nPortIndex;
            ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamPortDefinition, &portDef), "set port definition");
        }

        void allocBuffers(OMX_U32 nPortIndex, Buffer& buffer)
        {
            Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portDef;
            getPortDefinition(nPortIndex, portDef);

            ERR_OMX( OMX_AllocateBuffer(component_, buffer.pHeader(), nPortIndex, NULL, portDef->nBufferSize), "OMX_AllocateBuffer");
        }

        void freeBuffers(OMX_U32 nPortIndex, Buffer& buffer)
        {
            ERR_OMX( OMX_FreeBuffer(component_, nPortIndex, buffer.header()), "OMX_FreeBuffer");
        }

        void callFillThisBuffer(Buffer& buffer)
        {
            ERR_OMX( OMX_FillThisBuffer(component_, buffer.header()), "OMX_FillThisBuffer");
        }

	 void callEmptyThisBuffer(Buffer& buffer)
        {
            ERR_OMX( OMX_EmptyThisBuffer(component_, buffer.header()), "OMX_EmptyThisBuffer");
        }

        void eventCmdComplete(OMX_U32 cmd, OMX_U32 /*nPortIndex*/)
        {
            Lock lock(pSemaphore);  // LOCK

            switch (cmd)
            {
                case OMX_CommandStateSet:
                    ++eventState_;
                    break;

                case OMX_CommandFlush:
                    ++eventFlushed_;
                    break;

                case OMX_CommandPortDisable:
                    ++eventDisabled_;
                    break;

                case OMX_CommandPortEnable:
                    ++eventEnabled_;
                    break;

                case OMX_CommandMarkBuffer:
                default:
                    break;
            }
        }

        void eventPortSettingsChanged(OMX_U32 nPortIndex)
        {
            Lock lock(pSemaphore);  // LOCK

            ++changedPorts_[n2idx(nPortIndex)];
        }

    protected:
        OMX_HANDLETYPE component_;
        ComponentType type_;

        Component(ComponentType type, OMX_PTR pAppData, OMX_CALLBACKTYPE * callbacks)
        :   type_(type),
            eventState_(0),
            eventFlushed_(0),
            eventDisabled_(0),
            eventEnabled_(0)
        {
            changedPorts_.resize(numPorts());

            OMX_STRING xName = const_cast<OMX_STRING>(name());
            ERR_OMX( OMX_GetHandle(&component_, xName, pAppData, callbacks), "OMX_GetHandle");

            disablePort();
        }

        ~Component()
        {
            try
            {
                ERR_OMX( OMX_FreeHandle(component_), "OMX_FreeHandle" );
            }
            catch (const OMXExeption& )
            {
                // TODO
            }
        }

        // type_ equals to first port number
        unsigned n2idx(OMX_U32 nPortIndex) const { return nPortIndex - type_; }
        unsigned idx2n(unsigned idx) const { return type_ + idx; }

    private:
        static const unsigned WAIT_CHANGES_US = 1000;
        static const unsigned MAX_WAIT_COUNT = 200;

        unsigned eventState_;
        unsigned eventFlushed_;
        unsigned eventDisabled_;
        unsigned eventEnabled_;
        std::vector<unsigned> changedPorts_;

        // TODO: wait for specific port changes
        bool waitValue(unsigned * pValue, unsigned wantedValue)
        {
            for (unsigned i = 0; i < MAX_WAIT_COUNT; ++i)
            {
                if (*pValue == wantedValue)
                    return true;

                usleep(WAIT_CHANGES_US);
            }

            return false;
        }
#if 0
        bool waitStateChanged(OMX_STATETYPE wantedState)
        {
            for (unsigned i=0; i < MAX_WAIT_COUNT; ++i)
            {
                if (state() == wantedState)
                    return true;

                usleep(WAIT_CHANGES_US);
            }

            return false;
        }
#endif
    };

    /// Raspberry Pi Camera Module
    class Camera : public Component
    {
    public:
        static const ComponentType cType = broadcom::CAMERA;

        static const unsigned OPORT_PREVIEW = 70;
        static const unsigned OPORT_VIDEO = 71;
        static const unsigned OPORT_STILL = 72;
        static const unsigned IPORT = 73;

        static const unsigned CAM_DEVICE_NUMBER = 0;

        static int32_t align(unsigned x, unsigned y)
        {
            return (x + y - 1) & (~(y - 1));
        }

        // The recommended initialisation sequence:
        // 1. Create component.
        // 2. Use OMX_IndexConfigRequestCallback to request callbacks on OMX_IndexParamCameraDeviceNumber.
        // 3. Set OMX_IndexParamISPTunerName.
        // 4. Set OMX_IndexParamCameraFlashType.
        // 5. Set OMX_IndexParamCameraDeviceNumber.
        // 6. Wait for the callback that OMX_IndexParamCameraDeviceNumber has changed.
        //      At this point, all the drivers have been loaded. Other settings can be applied whilst waiting for this event.
        // 7. Query for OMX_IndexConfigCameraSensorModes as required.
        // 8. Change state to IDLE, and proceed as required.
        Camera()
        :   Component(cType, (OMX_PTR) this, &cbsEvents),
            ready_(false)
        {
            requestCallback();
            setDeviceNumber(CAM_DEVICE_NUMBER);
        }

        void requestCallback()
        {
            Parameter<OMX_CONFIG_REQUESTCALLBACKTYPE> cbtype;
            cbtype->nPortIndex = OMX_ALL;
            cbtype->nIndex     = OMX_IndexParamCameraDeviceNumber;
            cbtype->bEnable    = OMX_TRUE;

            ERR_OMX( OMX_SetConfig(component_, OMX_IndexConfigRequestCallback, &cbtype), "request callbacks");
        }

        void setDeviceNumber(unsigned camNumber)
        {
            Parameter<OMX_PARAM_U32TYPE> device;
            device->nPortIndex = OMX_ALL;
            device->nU32 = camNumber;

            ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamCameraDeviceNumber, &device), "set camera device number");
        }

        void setVideoFromat(const VideoFromat& videoFormat)
        {
            Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portDef;
            getPortDefinition(OPORT_VIDEO, portDef);

            portDef->format.video.nFrameWidth  = videoFormat.width;
            portDef->format.video.nFrameHeight = videoFormat.height;
            portDef->format.video.xFramerate   = videoFormat.framerate << 16;
            portDef->format.video.nStride      = align(portDef->format.video.nFrameWidth, portDef->nBufferAlignment);
            portDef->format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;

            setPortDefinition(OPORT_VIDEO, portDef);

	    getPortDefinition(OPORT_PREVIEW, portDef);
	portDef->format.video.nFrameWidth  = videoFormat.width;
            portDef->format.video.nFrameHeight = videoFormat.height;
            portDef->format.video.xFramerate   = videoFormat.framerate << 16;
            portDef->format.video.nStride      = align(portDef->format.video.nFrameWidth, portDef->nBufferAlignment);
            portDef->format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
	    setPortDefinition(OPORT_PREVIEW, portDef);
            //setFramerate(videoFormat.framerate);
        }

        void setFramerate(unsigned fps)
        {
            Parameter<OMX_CONFIG_FRAMERATETYPE> framerate;
            framerate->xEncodeFramerate = fps << 16;
            framerate->nPortIndex = OPORT_VIDEO;

            ERR_OMX( OMX_SetConfig(component_, OMX_IndexConfigVideoFramerate, &framerate), "set framerate");
        }

        // -100 .. 100
        void setSharpness(OMX_U32 nPortIndex = OMX_ALL, OMX_S32 nSharpness = 0)
        {
            Parameter<OMX_CONFIG_SHARPNESSTYPE> sharpness;
            sharpness->nPortIndex = nPortIndex;
            sharpness->nSharpness = nSharpness;

            ERR_OMX( OMX_SetConfig(component_, OMX_IndexConfigCommonSharpness, &sharpness), "set camera sharpness");
        }

        // -100 .. 100
        void setContrast(OMX_U32 nPortIndex = OMX_ALL, OMX_S32 nContrast = 0)
        {
            Parameter<OMX_CONFIG_CONTRASTTYPE> contrast;
            contrast->nPortIndex = nPortIndex;
            contrast->nContrast = nContrast;

            ERR_OMX( OMX_SetConfig(component_, OMX_IndexConfigCommonContrast, &contrast), "set camera contrast");
        }

        // -100 .. 100
        void setSaturation(OMX_U32 nPortIndex = OMX_ALL, OMX_S32 nSaturation = 0)
        {
            Parameter<OMX_CONFIG_SATURATIONTYPE> saturation;
            saturation->nPortIndex = nPortIndex;
            saturation->nSaturation = nSaturation;

            ERR_OMX( OMX_SetConfig(component_, OMX_IndexConfigCommonSaturation, &saturation), "set camera saturation");
        }

        // 0 .. 100
        void setBrightness(OMX_U32 nPortIndex = OMX_ALL, OMX_U32 nBrightness = 50)
        {
            Parameter<OMX_CONFIG_BRIGHTNESSTYPE> brightness;
            brightness->nPortIndex = nPortIndex;
            brightness->nBrightness = nBrightness;

            ERR_OMX( OMX_SetConfig(component_, OMX_IndexConfigCommonBrightness, &brightness), "set camera brightness");
        }

        void setExposureValue(OMX_U32 nPortIndex = OMX_ALL,
                OMX_S32 xEVCompensation = 0, OMX_U32 nSensitivity = 100, OMX_BOOL bAutoSensitivity = OMX_TRUE)
        {
            Parameter<OMX_CONFIG_EXPOSUREVALUETYPE> exposure_value;
            exposure_value->nPortIndex = nPortIndex;
            exposure_value->xEVCompensation = xEVCompensation;
            exposure_value->nSensitivity = nSensitivity;
            exposure_value->bAutoSensitivity = bAutoSensitivity;
	    //exposure_value->bAutoShutterSpeed=OMX_TRUE;
	exposure_value->bAutoShutterSpeed=OMX_FALSE;
	    exposure_value->nShutterSpeedMsec=10000;

            ERR_OMX( OMX_SetConfig(component_, OMX_IndexConfigCommonExposureValue, &exposure_value), "set camera exposure value");
        }

        void setFrameStabilisation(OMX_U32 nPortIndex = OMX_ALL, OMX_BOOL bStab = OMX_TRUE)
        {
            Parameter<OMX_CONFIG_FRAMESTABTYPE> frame_stabilisation_control;
            frame_stabilisation_control->nPortIndex = nPortIndex;
            frame_stabilisation_control->bStab = bStab;

            ERR_OMX( OMX_SetConfig(component_, OMX_IndexConfigCommonFrameStabilisation, &frame_stabilisation_control),
                     "set camera frame stabilisation");
        }

        void setWhiteBalanceControl(OMX_U32 nPortIndex = OMX_ALL, OMX_WHITEBALCONTROLTYPE eWhiteBalControl = OMX_WhiteBalControlAuto)
        {
            Parameter<OMX_CONFIG_WHITEBALCONTROLTYPE> white_balance_control;
            white_balance_control->nPortIndex = nPortIndex;
            white_balance_control->eWhiteBalControl = eWhiteBalControl;

            ERR_OMX( OMX_SetConfig(component_, OMX_IndexConfigCommonWhiteBalance, &white_balance_control),
                     "set camera frame white balance");
        }

        void setImageFilter(OMX_U32 nPortIndex = OMX_ALL, OMX_IMAGEFILTERTYPE eImageFilter = OMX_ImageFilterNone)
        {
            Parameter<OMX_CONFIG_IMAGEFILTERTYPE> image_filter;
            image_filter->nPortIndex = nPortIndex;
            image_filter->eImageFilter = eImageFilter;

            ERR_OMX( OMX_SetConfig(component_, OMX_IndexConfigCommonImageFilter, &image_filter), "set camera image filter");
        }

        // OMX_MirrorHorizontal | OMX_MirrorVertical | OMX_MirrorBoth
        void setMirror(OMX_U32 nPortIndex, OMX_MIRRORTYPE eMirror = OMX_MirrorNone)
        {
            Parameter<OMX_CONFIG_MIRRORTYPE> mirror;
            mirror->nPortIndex = nPortIndex;
            mirror->eMirror = eMirror;

            ERR_OMX( OMX_SetConfig(component_, OMX_IndexConfigCommonMirror, &mirror), "set cammera mirror");
        }

        void setImageDefaults()
        {
            setSharpness();
            setContrast();
            setSaturation();
            setBrightness();
            setExposureValue();
            setFrameStabilisation();
            setWhiteBalanceControl();
            setImageFilter();
            //setMirror();
        }

        void capture(OMX_U32 nPortIndex, OMX_BOOL bEnabled)
        {
            Parameter<OMX_CONFIG_PORTBOOLEANTYPE> capture;
            capture->nPortIndex = nPortIndex;
            capture->bEnabled = bEnabled;

            ERR_OMX( OMX_SetParameter(component_, OMX_IndexConfigPortCapturing, &capture), "switch capture on port");
        }

        void allocBuffers()
        {
            Component::allocBuffers(IPORT, bufferIn_);
        }

        void freeBuffers()
        {
            Component::freeBuffers(IPORT, bufferIn_);
        }

        bool ready() const { return ready_; }

        void eventReady()
        {
            Lock lock(pSemaphore); // LOCK

            ready_ = true;
        }

    private:
        Buffer bufferIn_;
        bool ready_;
    };

    ///
    class Encoder : public Component
    {
    public:
        static const ComponentType cType = broadcom::VIDEO_ENCODER;

        static const unsigned IPORT = 200;
        static const unsigned OPORT = 201;

        Encoder()
        :   Component(cType, (OMX_PTR) this, &cbsEvents)
        {
        }

        void setupOutputPortFromCamera(const Parameter<OMX_PARAM_PORTDEFINITIONTYPE>& cameraPortDef, unsigned bitrate, unsigned framerate = 0) // Framerate 0 means get it from Camera when tunelled
        {
            Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portDef;
            getPortDefinition(OPORT, portDef);

            portDef->format.video.nFrameWidth  = cameraPortDef->format.video.nFrameWidth;
            portDef->format.video.nFrameHeight = cameraPortDef->format.video.nFrameHeight;
            portDef->format.video.xFramerate   = cameraPortDef->format.video.xFramerate;
            portDef->format.video.nStride      = cameraPortDef->format.video.nStride;
            portDef->format.video.nBitrate     = bitrate;
	   //printf("FPS=%x\n",cameraPortDef->format.video.xFramerate);	
            if (framerate)
                portDef->format.video.xFramerate = framerate<<16;

            setPortDefinition(OPORT, portDef);
        }

	void setupOutputPort(const VideoFromat Videoformat, unsigned bitrate, unsigned framerate = 25)
	{
		// Input Definition
		Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portDefI;
            getPortDefinition(IPORT, portDefI);
		portDefI->format.video.nFrameWidth  = Videoformat.width;
            portDefI->format.video.nFrameHeight = Videoformat.height;
            portDefI->format.video.xFramerate   = framerate<<16;
            portDefI->format.video.nStride      = Videoformat.width;//(portDefI->format.video.nFrameWidth + portDefI->nBufferAlignment - 1) & (~(portDefI->nBufferAlignment - 1));
	portDefI->format.video.nSliceHeight=Videoformat.height;
		portDefI->format.video.eColorFormat=OMX_COLOR_FormatYUV420PackedPlanar;
	 setPortDefinition(IPORT, portDefI);	
	
	// Output definition : copy from input
		Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portDef;
		getPortDefinition(OPORT, portDef);
            //portDefI->nPortIndex     = OPORT;
		//portDef->format.video.eColorFormat=OMX_COLOR_FormatUnused;
		portDef->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
      		portDef->format.video.nBitrate=bitrate;
		portDef->format.video.nFrameWidth  = Videoformat.width;
		portDef->format.video.nFrameHeight = Videoformat.height;
		portDef->format.video.xFramerate= framerate<<16;

	        setPortDefinition(OPORT, portDef);
	}

        void setCodec(OMX_VIDEO_CODINGTYPE codec)
        {
            Parameter<OMX_VIDEO_PARAM_PORTFORMATTYPE> format;
            format->nPortIndex = OPORT;
            format->eCompressionFormat = codec;

            ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamVideoPortFormat, &format), "set video format");
        }

	void setProfileLevel(int Profile=OMX_VIDEO_AVCProfileMain,int Level=OMX_VIDEO_AVCLevel32)
        {
	    //OMX_VIDEO_AVCProfileBaseline,OMX_VIDEO_AVCProfileMain,OMX_VIDEO_AVCProfileExtended, OMX_VIDEO_AVCProfileHigh
	    //OMX_VIDEO_AVCLevel3
            Parameter<OMX_VIDEO_PARAM_PROFILELEVELTYPE> ProfileLevel;
            ProfileLevel->nPortIndex = OPORT;
            ProfileLevel->eProfile = Profile;
            ProfileLevel->eLevel = Level;

            ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamVideoProfileLevelCurrent, &ProfileLevel), "set Profile/Level");
        }
	

        void setBitrate(OMX_U32 bitrate, OMX_VIDEO_CONTROLRATETYPE type = OMX_Video_ControlRateVariable)
        {
	    // https://github.com/raspberrypi/firmware/issues/329#issuecomment-61696016

            Parameter<OMX_VIDEO_PARAM_BITRATETYPE> brate;
            brate->nPortIndex = OPORT;
            brate->eControlRate = type;
            brate->nTargetBitrate = bitrate;

            ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamVideoBitrate, &brate), "set bitrate");
        }

	void setIDR(OMX_U32  idr_period/*, OMX_U32  nPFrames*/)
	{
		Parameter<OMX_VIDEO_CONFIG_AVCINTRAPERIOD> idr_st;
		idr_st->nPortIndex= OPORT;
	 	idr_st->nIDRPeriod = idr_period;
		printf("idr %d p%d\n",idr_st->nIDRPeriod,idr_st->nPFrames);
  		//idr_st->nPFrames=nPFrames;
		ERR_OMX( OMX_GetParameter(component_, OMX_IndexConfigVideoAVCIntraPeriod, &idr_st)," Get idr");
		idr_st->nPFrames=idr_period-1;
		idr_st->nIDRPeriod = idr_period;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexConfigVideoAVCIntraPeriod, &idr_st), "set idr");


		

	}

	void setVectorMotion()
	{
		Parameter<OMX_CONFIG_PORTBOOLEANTYPE> motionvector;
		motionvector->nPortIndex= OPORT;
  		motionvector->bEnabled=OMX_TRUE;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamBrcmVideoAVCInlineVectorsEnable, &motionvector), "set vector motion");
	}
	
	// Measure Quality of coding
	void setEED()
	{
		Parameter<OMX_VIDEO_EEDE_ENABLE> EED;
		EED->nPortIndex= OPORT;
		EED->enable=OMX_TRUE;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamBrcmEEDEEnable, &EED), "EED");

		Parameter<OMX_VIDEO_EEDE_LOSSRATE> EEDRate;
		EEDRate->nPortIndex= OPORT;
		EEDRate->loss_rate= 1; // Set to no packet lost on transmission
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamBrcmEEDELossRate, &EEDRate), "EED");
		
	}

	void setSEIMessage()
	{
		Parameter<OMX_CONFIG_PORTBOOLEANTYPE> InlineHeader; //SPS/PPS
		InlineHeader->nPortIndex= OPORT;
  		InlineHeader->bEnabled=OMX_TRUE;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamBrcmVideoAVCInlineHeaderEnable, &InlineHeader), "InlineHeader");
	
		Parameter<OMX_CONFIG_PORTBOOLEANTYPE> SPSTiming; //SPS Timing Enable
		SPSTiming->nPortIndex= OPORT;
  		SPSTiming->bEnabled=OMX_TRUE;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamBrcmVideoAVCSPSTimingEnable, &SPSTiming), "SPS Timing Enable");
		
		Parameter<OMX_CONFIG_PORTBOOLEANTYPE> SEIMessage;
		SEIMessage->nPortIndex= OPORT;
  		SEIMessage->bEnabled=OMX_TRUE;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamBrcmVideoAVCSEIEnable, &SEIMessage), "SEIMessage");

		
		Parameter<OMX_CONFIG_PORTBOOLEANTYPE> VCLHRD; //HRD ENABLE IN HEADER
		VCLHRD->nPortIndex= OPORT;
  		VCLHRD->bEnabled=OMX_TRUE;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamBrcmVideoAVC_VCLHRDEnable, &VCLHRD), "VLCHRD");

		
		

	}


	void setLowLatency()
	{

		/* the encoder for I420 doesn’t emit any output buffers if the MMAL_PARAMETER_VIDEO_ENCODE_H264_LOW_LATENCY parameter is set. It works with opaque. No errors are generated.

[6by9 Raspi Engineer:]
LOW_LATENCY mode is not a mode intended for general use. There was a specific use case for it where the source could feed the image in a stripe at a time, and the encoder would take the data as it was available. There were a large number of limitations to using it, but it fulfilled the purpose. This is the downside of having released the full MMAL headers without sanitising first – people see interesting looking parameters and tweak. At that point it is user beware!
		*/
		/*Parameter<OMX_CONFIG_PORTBOOLEANTYPE> LowLatency; //HRD Low delay FLAG
		LowLatency->nPortIndex= OPORT;
  		LowLatency->bEnabled=OMX_TRUE;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexConfigBrcmVideoH264LowLatency, &LowLatency), " Low delay");*/

		Parameter<OMX_CONFIG_PORTBOOLEANTYPE> LowHRD; //HRD Low delay FLAG
		LowHRD->nPortIndex= OPORT;
  		LowHRD->bEnabled=OMX_TRUE;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamBrcmVideoAVC_LowDelayHRDEnable, &LowHRD), "HRD Low Delay");
	}

	void setSeparateNAL()
	{
		Parameter<OMX_CONFIG_PORTBOOLEANTYPE> SepNAL;
		SepNAL->nPortIndex= OPORT;
  		SepNAL->bEnabled=OMX_TRUE;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamBrcmNALSSeparate, &SepNAL), "Separate NAL");
	}

	void setMinizeFragmentation()
	{
		Parameter<OMX_CONFIG_PORTBOOLEANTYPE> MinizeFragmentation;
		MinizeFragmentation->nPortIndex= OPORT;
  		MinizeFragmentation->bEnabled=OMX_TRUE;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexConfigMinimiseFragmentation, &MinizeFragmentation), "Minimize Fragmentation");
	}

	void setMaxFrameLimits(int FrameLimitInBits)
	{
		// Seems that if above this limit, encoder do not output frame !!
		Parameter<OMX_PARAM_U32TYPE> MaxFrameLimit;
		MaxFrameLimit->nPortIndex= OPORT;
  		MaxFrameLimit->nU32=FrameLimitInBits;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamBrcmVideoFrameLimitBits, &MaxFrameLimit), "MaxFrameLimit");
	}
	
	void setPeakRate(int PeakRateinKbit)
	{
		Parameter<OMX_PARAM_U32TYPE> PeakRate;
		PeakRate->nPortIndex= OPORT;
  		
		ERR_OMX( OMX_GetParameter(component_, OMX_IndexParamBrcmVideoPeakRate, &PeakRate)," GetPeakRate");
		printf("\nPEAK GET = %d\n",PeakRate->nU32); 
		PeakRate->nU32=PeakRateinKbit;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexParamBrcmVideoPeakRate, &PeakRate), "PeakRateinKbit");
		ERR_OMX( OMX_GetParameter(component_, OMX_IndexParamBrcmVideoPeakRate, &PeakRate)," GetPeakRate");
		printf("\nPEAK GET 2 = %d\n",PeakRate->nU32); 
           // Peak video bitrate in bits per second. Must be larger or equal to the average video bitrate. It is ignored if the  video bitrate mode is set to constant bitrate.
                    
	}
	
	void setDynamicBitrate(int VideoBitrate)
	{
		Parameter<OMX_VIDEO_CONFIG_BITRATETYPE> DynamicBitrate;
		DynamicBitrate->nPortIndex= OPORT;
		DynamicBitrate->nEncodeBitrate = VideoBitrate;
		ERR_OMX( OMX_SetParameter(component_, OMX_IndexConfigVideoBitrate, &DynamicBitrate), "DynamicVideoBitRate");

	}
	
//Set QP restrict QP : means if encoder choose a QP which is not in this range, Frame is dropped
	void setQPLimits(int QMin=10,int QMax=50)
	{
		Parameter<OMX_PARAM_U32TYPE> QPMin;
		QPMin->nPortIndex=OPORT;
		QPMin->nU32=QMin;
		ERR_OMX( OMX_SetParameter(component_,OMX_IndexParamBrcmVideoEncodeMinQuant, &QPMin)," QPMin");

		Parameter<OMX_PARAM_U32TYPE> QPMax;
		QPMax->nPortIndex=OPORT;
		QPMax->nU32=QMax;
		ERR_OMX( OMX_SetParameter(component_,OMX_IndexParamBrcmVideoEncodeMaxQuant, &QPMax)," QPMax");
		
	} 

	void setQFromBitrate(int Bitrate,int fps,int Width,int Height,int MotionType=0)
	{
		int Coeff=2;
		//int QPCalculation=10+Width*Height*fps*Coeff/((Bitrate));
		//For 720*576, 25fps  : QP=280birate⁻(0,345)
		int QPCalculation=281*pow(Bitrate*(25/fps)*(720/Width)*(576/Height)/1000.0,-0.345)+10;	
		printf("QP=%d\n",QPCalculation);
		if(QPCalculation>48) QPCalculation=48; //Fixme
		if(QPCalculation<10) QPCalculation=10; //Fixme
		
		setQPLimits(QPCalculation,QPCalculation);
	} 

// ONLY IF RATECONTROL is not CBR/VBR
 void setQP(int QPi,int QPp)
{
	Parameter<OMX_VIDEO_PARAM_QUANTIZATIONTYPE> QP;
	QP->nPortIndex=OPORT;
	QP->nQpI=QPi;
	QP->nQpP=QPp;
	QP->nQpB=0; // No B Frame, only zero is allowed

	ERR_OMX( OMX_SetParameter(component_,OMX_IndexParamBrcmVideoEncodeQpP, &QP)," QP");


}
	void setMultiSlice(int SliceSize)
	{

/*typedef enum OMX_VIDEO_AVCSLICEMODETYPE {
    OMX_VIDEO_SLICEMODE_AVCDefault = 0,
    OMX_VIDEO_SLICEMODE_AVCMBSlice,
    OMX_VIDEO_SLICEMODE_AVCByteSlice,
    OMX_VIDEO_SLICEMODE_AVCKhronosExtensions = 0x6F000000, 
    OMX_VIDEO_SLICEMODE_AVCVendorStartUnused = 0x7F000000, 
    OMX_VIDEO_SLICEMODE_AVCLevelMax = 0x7FFFFFFF
} OMX_VIDEO_AVCSLICEMODETYPE;*/
/* https://e2e.ti.com/support/embedded/android/f/509/t/234521 */
		/*Parameter<OMX_VIDEO_PARAM_AVCSLICEFMO> MultiSliceMode; //NOT SUPPORTED !!!!!!!!
		MultiSliceMode->nPortIndex=OPORT;
		ERR_OMX( OMX_GetParameter(component_, OMX_IndexParamVideoSliceFMO, &MultiSliceMode)," Get SliceMode");
		printf("Slice Mode %d %d %d \n",MultiSliceMode->eSliceMode,MultiSliceMode->nNumSliceGroups,MultiSliceMode->nSliceGroupMapType);
		//MultiSliceMode->eSliceMode = OMX_VIDEO_SLICEMODE_AVCByteSlice;
  		//MultiSliceMode->nNumSliceGroups = 0;
  		//MultiSliceMode->nSliceGroupMapType = 0;
		//ERR_OMX( OMX_SetParameter(component_,OMX_IndexParamVideoSliceFMO, &MultiSliceMode)," MultisliceMode");
		*/  

		/* Run with more than one slice per frame. It reduces compression efficiently slightly, but as it can't send out only one of the two slices then it has to send both. OMX_IndexConfigBrcmVideoEncoderMBRowsPerSlice to (height/16)/2 for 2 slices per frame. Don't increase it excessively.*/

		// For MultiRows : OK but penalty on all Frames !!!
		/*Parameter<OMX_PARAM_U32TYPE> MultiSliceRow;
		MultiSliceRow->nPortIndex=OPORT;
		MultiSliceRow->nU32=SliceSize;
		ERR_OMX( OMX_SetParameter(component_,OMX_IndexConfigBrcmVideoEncoderMBRowsPerSlice, &MultiSliceRow)," MultisliceRow");
		*/

		Parameter<OMX_VIDEO_PARAM_INTRAREFRESHTYPE> IntraRefreshType;
		IntraRefreshType->nPortIndex=OPORT;
		ERR_OMX( OMX_GetParameter(component_,  OMX_IndexParamVideoIntraRefresh, &IntraRefreshType)," IntraRefreshMode");
		IntraRefreshType->eRefreshMode=OMX_VIDEO_IntraRefreshCyclicMrows;
		
		IntraRefreshType->nCirMBs=SliceSize;
		ERR_OMX( OMX_SetParameter(component_,  OMX_IndexParamVideoIntraRefresh, &IntraRefreshType)," IntraRefreshMode");

// SHOULD HAVE INSPECT WITH OMX_VIDEO_INTRAREFRESHTYPE
		/*Curiously there are IL settings for OMX_VIDEO_IntraRefreshPseudoRand and OMX_VIDEO_IntraRefreshCyclicMrows.
...
Having just spoken to the codecs guys, he recalls CyclicMrows to actually be the one actively used for a previous product to split the I-frame into about 5. I'll add pseudo random and cyclic mrows to MMAL, but you're on your own working out useful settings, and may not achieve 1080P30 if pushed too far.
So the advice was for MMAL_VIDEO_INTRA_REFRESH_CYCLIC_MROWS and cir_mbs set probably to 5 (at a guess).*/


		/*Parameter<OMX_PARAM_U32TYPE> MultiSlice;
		MultiSlice->nPortIndex=OPORT;
		MultiSlice->nU32=SliceSize;
		ERR_OMX( OMX_SetParameter(component_,OMX_IndexConfigBrcmVideoEncodedSliceSize, &MultiSlice)," Multislice");*/
  		
  
	}

	void getEncoderStat(int Flags)
	{
		char debug[255];
		sprintf(debug,"");
		if(Flags&OMX_BUFFERFLAG_ENDOFFRAME) strcat(debug,"ENDOFFRAME ");
		if(Flags&OMX_BUFFERFLAG_SYNCFRAME) strcat(debug,"SYNCFRAME ");
		if(Flags&OMX_BUFFERFLAG_CODECCONFIG) strcat(debug,"CODECCONFIG ");
		if(Flags&OMX_BUFFERFLAG_ENDOFNAL) strcat(debug,"ENDOFNAL ");

		
		static struct timespec tbefore;
		static int Count=0;
	Parameter<OMX_CONFIG_BRCMPORTSTATSTYPE> VideoStat;
		VideoStat->nPortIndex= OPORT;
		ERR_OMX( OMX_GetParameter(component_, OMX_IndexConfigBrcmPortStats, &VideoStat)," Get VideoStat");
		struct timespec t;
         clock_gettime(CLOCK_REALTIME, &t);
		printf("VideoStat : %s ByteCount %d Buffer %d - Frame %d = %d Skip %d Discard %d Max Delta%d:%d TIME %li\n",/*VideoStat->nByteCount.nLowPart*8*25/VideoStat->nFrameCount,*/debug,VideoStat->nByteCount.nLowPart,VideoStat->nBufferCount,VideoStat->nFrameCount,VideoStat->nBufferCount-VideoStat->nFrameCount*2,VideoStat->nFrameSkips,VideoStat->nDiscards,VideoStat->nMaxTimeDelta.nHighPart,VideoStat->nMaxTimeDelta.nLowPart,( t.tv_sec -tbefore.tv_sec  )*1000ul + ( t.tv_nsec - tbefore.tv_nsec)/1000000);
	tbefore=t;
	Count++;
	}

        void allocBuffers(bool WithBuffIn=false)
        {
		Component::allocBuffers(OPORT, bufferOut_);
		HaveABufferIn=WithBuffIn;
	     	if(HaveABufferIn) 
		{
			
			Component::allocBuffers(IPORT, bufferIn_);
		}
			
        }

        void freeBuffers()
        {
            Component::freeBuffers(OPORT, bufferOut_);
		if(HaveABufferIn)
		Component::freeBuffers(IPORT, bufferIn_);
        }

        void callFillThisBuffer()
        {
            Component::callFillThisBuffer(bufferOut_);
        }

	void callEmptyThisBuffer()
        {
            Component::callEmptyThisBuffer(bufferIn_);
        }

        Buffer& outBuffer() { return bufferOut_; }
	Buffer& inBuffer() { return bufferIn_; }
    private:
        Parameter<OMX_PARAM_PORTDEFINITIONTYPE> encoderPortDef_;
        Buffer bufferOut_;
	Buffer bufferIn_;
	bool HaveABufferIn=false;
    };

    ///
    class NullSink : public Component
    {
    public:
        static const ComponentType cType = broadcom::NULL_SINK;

        static const unsigned IPORT = 240;

        NullSink()
        :   Component(cType, (OMX_PTR) this, &cbsEvents)
        {
        }
    };

    ///
    class VideoRenderer : public Component
    {
    public:
        static const ComponentType cType = broadcom::VIDEO_RENDER;

        static const unsigned IPORT = 90;

        VideoRenderer()
        :   Component(cType, (OMX_PTR) this, &cbsEvents)
        {
        }

	void setupPortFromCamera(const Parameter<OMX_PARAM_PORTDEFINITIONTYPE>& cameraPortDef)
	{
	
	Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portDef;
            getPortDefinition(IPORT, portDef);

          /*  portDef->format.video.nFrameWidth  = cameraPortDef->format.video.nFrameWidth;
            portDef->format.video.nFrameHeight = cameraPortDef->format.video.nFrameHeight;
            portDef->format.video.xFramerate   = cameraPortDef->format.video.xFramerate;
            portDef->format.video.nStride      = cameraPortDef->format.video.nStride;
	*/
		portDef=cameraPortDef;
           // setPortDefinition(IPORT, portDef);
	}
	void SetDestRect(int x,int y,int width,int height)
	{
	#define DISPLAY_DEVICE 0
	 Parameter<OMX_CONFIG_DISPLAYREGIONTYPE> display_region;
    	display_region->nPortIndex=IPORT;
	    display_region->set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_NUM | OMX_DISPLAY_SET_FULLSCREEN | OMX_DISPLAY_SET_MODE | OMX_DISPLAY_SET_DEST_RECT);
	    display_region->num = DISPLAY_DEVICE;
	    display_region->fullscreen = OMX_FALSE;
	    display_region->mode = OMX_DISPLAY_MODE_FILL;
	    display_region->dest_rect.width = width;
	    display_region->dest_rect.height = height;
	    display_region->dest_rect.x_offset = x;
	    display_region->dest_rect.y_offset = y;
	ERR_OMX( OMX_SetParameter(component_,  OMX_IndexConfigDisplayRegion, &display_region)," Display rect");
	}

	void allocBuffers()
        {
		
			
			Component::allocBuffers(IPORT, bufferIn_);
			
        }

        void freeBuffers()
        {
            
		Component::freeBuffers(IPORT, bufferIn_);
        }
	private:
	Buffer bufferIn_;
    };

class VideoSplitter : public Component
    {
    public:
        static const ComponentType cType = broadcom::VIDEO_SPLITTER;

        static const unsigned IPORT = 250;
        static const unsigned OPORT_1 = 251;
        static const unsigned OPORT_2 = 252;
        static const unsigned OPORT_3 = 253;
        static const unsigned OPORT_4 = 254;

        VideoSplitter()
        :   Component(cType, (OMX_PTR) this, &cbsEvents)
        {
        }
    };

    ///
    class Resizer : public Component
    {
    public:
        static const ComponentType cType = broadcom::RESIZER;

        static const unsigned IPORT = 60;
        static const unsigned OPORT = 61;

        Resizer()
        :   Component(cType, (OMX_PTR) this, &cbsEvents)
        {
        }

        void setupOutputPort(int SrcImageWidth,int SrcImageHeight,const VideoFromat Videoformat,OMX_COLOR_FORMATTYPE colortype)
        {
//http://www.jvcref.com/files/PI/documentation/ilcomponents/resize.html
	Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portDefI;
            getPortDefinition(IPORT, portDefI);
	
		portDefI->format.image.nFrameWidth  = SrcImageWidth;
            portDefI->format.image.nFrameHeight = SrcImageHeight;
            portDefI->format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
	//portDefI->format.image.bFlagErrorConcealment = OMX_TRUE;
	if(colortype==OMX_COLOR_FormatYUV420PackedPlanar)
	{
		
		portDefI->format.image.nStride      = SrcImageWidth;
		portDefI->format.image.nSliceHeight=SrcImageHeight;
		printf("%d * %d\n",portDefI->format.image.nStride,portDefI->format.image.nSliceHeight);
	}
	else
	{           portDefI->format.image.nStride      =0 ;
		portDefI->format.image.nSliceHeight=0;
	}

	portDefI->format.image.eColorFormat=colortype;
	
	 setPortDefinition(IPORT, portDefI);
	
// Output definition 
	Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portDefO;
	getPortDefinition(OPORT, portDefO);

	portDefO->format.image.nFrameWidth  = Videoformat.width;
         portDefO->format.image.nFrameHeight = Videoformat.height;
        portDefO->format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
	portDefO->format.image.nStride      = Videoformat.width;
	portDefO->format.image.nSliceHeight=Videoformat.height;
	portDefO->format.image.bFlagErrorConcealment = OMX_FALSE;
	portDefO->format.image.eColorFormat=OMX_COLOR_FormatYUV420PackedPlanar;//For encoder
            	
	       setPortDefinition(OPORT, portDefO);
            
           
        }

	 void allocBuffers()
        {
		
			
			Component::allocBuffers(IPORT, bufferIn_);
		
			
        }

        void freeBuffers()
        {
            
		Component::freeBuffers(IPORT, bufferIn_);
        }

        void callFillThisBuffer()
        {
            Component::callFillThisBuffer(bufferOut_);
        }

	void callEmptyThisBuffer()
        {
            Component::callEmptyThisBuffer(bufferIn_);
        }

        Buffer& outBuffer() { return bufferOut_; }
	Buffer& inBuffer() { return bufferIn_; }
    private:
        //Parameter<OMX_PARAM_PORTDEFINITIONTYPE> encoderPortDef_;
        Buffer bufferOut_;
	Buffer bufferIn_;
	

    };


 class ImageEncode : public Component
    {
    public:
        static const ComponentType cType = broadcom::IMAGE_ENCODE;

        static const unsigned IPORT = 340;
        static const unsigned OPORT = 341;

        ImageEncode()
        :   Component(cType, (OMX_PTR) this, &cbsEvents)
        {
        }

	
	void setupPort(int SrcImageWidth,int SrcImageHeight,OMX_COLOR_FORMATTYPE colortype)
        {
//http://www.jvcref.com/files/PI/documentation/ilcomponents/resize.html
	Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portDefI;
            getPortDefinition(IPORT, portDefI);
	
		portDefI->format.image.nFrameWidth  = SrcImageWidth;
            portDefI->format.image.nFrameHeight = SrcImageHeight;
            portDefI->format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
	//portDefI->format.image.bFlagErrorConcealment = OMX_TRUE;
	if(colortype==OMX_COLOR_FormatYUV420PackedPlanar)
	{
		
		portDefI->format.image.nStride      = SrcImageWidth;
		portDefI->format.image.nSliceHeight=SrcImageHeight;
		//printf("%d * %d\n",portDefI->format.image.nStride,portDefI->format.image.nSliceHeight);
	}
	else
	{           portDefI->format.image.nStride      =SrcImageWidth ;
		portDefI->format.image.nSliceHeight=SrcImageHeight;
	}

	portDefI->format.image.eColorFormat=colortype;
	
	 setPortDefinition(IPORT, portDefI);
	
// Output definition 
	Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portDefO;
	getPortDefinition(OPORT, portDefO);

	portDefO->format.image.nFrameWidth  = SrcImageWidth;
         portDefO->format.image.nFrameHeight = SrcImageHeight;
        portDefO->format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
	portDefO->format.image.nStride      =SrcImageWidth;
	portDefO->format.image.nSliceHeight=SrcImageHeight;
	//portDefO->format.image.bFlagErrorConcealment = OMX_FALSE;
	portDefO->format.image.eColorFormat=OMX_COLOR_FormatYUV420PackedPlanar;//For encoder
         
   	
	setPortDefinition(OPORT, portDefO);
            
           
        }

	void allocBuffers()
        {
		
			
			Component::allocBuffers(IPORT, bufferIn_);
			
        }

        void freeBuffers()
        {
            
		Component::freeBuffers(IPORT, bufferIn_);
        }

        void callFillThisBuffer()
        {
            Component::callFillThisBuffer(bufferOut_);
        }

	void callEmptyThisBuffer()
        {
            Component::callEmptyThisBuffer(bufferIn_);
        }

        Buffer& outBuffer() { return bufferOut_; }
	Buffer& inBuffer() { return bufferIn_; }
    private:
        //Parameter<OMX_PARAM_PORTDEFINITIONTYPE> encoderPortDef_;
        Buffer bufferOut_;
	Buffer bufferIn_;
	
};
    //

    static OMX_ERRORTYPE callback_EventHandler(
            OMX_HANDLETYPE hComponent,
            OMX_PTR pAppData,
            OMX_EVENTTYPE eEvent,
            OMX_U32 nData1,
            OMX_U32 nData2,
            OMX_PTR pEventData)
    {
        Component * component = static_cast<Component *>(pAppData);

        printEvent(component->name(), hComponent, eEvent, nData1, nData2);

        switch (eEvent)
        {
            case OMX_EventCmdComplete:
            {
                switch (nData1)
                {
                    case OMX_CommandFlush:
                    case OMX_CommandPortDisable:
                    case OMX_CommandPortEnable:
                    case OMX_CommandMarkBuffer:
                        // nData2 is port index
                        component->eventCmdComplete(nData1, nData2);
                        break;

                    case OMX_CommandStateSet:
                        // nData2 is state
                        component->eventCmdComplete(nData1, nData2);
                        break;

                    default:
                        break;
                }

                break;
            }

            case OMX_EventPortSettingsChanged:
            {
                // nData1 is port index
                component->eventPortSettingsChanged(nData1);
                break;
            }

            // vendor specific
            case OMX_EventParamOrConfigChanged:
            {
                if (nData2 == OMX_IndexParamCameraDeviceNumber)
                {
                    Camera * camera = static_cast<Camera *>(pAppData);
                    camera->eventReady();
                }

                break;
            }

            case OMX_EventError:
                OMXExeption::die( (OMX_ERRORTYPE) nData1, "OMX_EventError received");
                break;

            case OMX_EventMark:
            case OMX_EventResourcesAcquired:
            case OMX_EventBufferFlag:
            default:
                break;
        }

        return OMX_ErrorNone;
    }

    static OMX_ERRORTYPE callback_EmptyBufferDone(OMX_HANDLETYPE, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBuffer)
    {
	Component * component = static_cast<Component *>(pAppData);

        if (component->type() == Encoder::cType)
        {
	    //printf("Filled %d Timestamp %li\n",pBuffer->nFilledLen,pBuffer->nTickCount);
            Encoder * encoder = static_cast<Encoder *>(pAppData);
            encoder->inBuffer().setFilled();
        }
	if (component->type() == Resizer::cType)
        {

            Resizer * resizer = static_cast<Resizer *>(pAppData);
            resizer->inBuffer().setFilled();
        }

	if (component->type() == ImageEncode::cType)
        {

            ImageEncode * imageencode = static_cast<ImageEncode *>(pAppData);
            imageencode->inBuffer().setFilled();
        }
        return OMX_ErrorNone;
    }

    static OMX_ERRORTYPE callback_FillBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBuffer)
    {
        Component * component = static_cast<Component *>(pAppData);

        if (component->type() == Encoder::cType)
        {
	    //printf("Filled %d Timestamp %li\n",pBuffer->nFilledLen,pBuffer->nTickCount);
            Encoder * encoder = static_cast<Encoder *>(pAppData);
            encoder->outBuffer().setFilled();
        }

        return OMX_ErrorNone;
    }
}


// Global variable used by the signal handler and capture/encoding loop
static int want_quit = 0;

#if 1
// Global signal handler for trapping SIGINT, SIGTERM, and SIGQUIT
static void signal_handler(int signal)
{
    want_quit = 1;
}
#endif




class TSEncaspulator
{
	private:
	ts_writer_t *writer = NULL;

	ts_main_t tsmain;
	ts_program_t program[1];
	ts_stream_t ts_stream[1];
	FILE *vout=NULL;

	int64_t vdts = 0;
	int64_t vpts = 0;

	 int64_t *pcr_list = NULL;
	 uint8_t *out = NULL;
         size_t fn = 0;
	#define MAX_SIZE_PICTURE 128000
	uint8_t InternalBuffer[MAX_SIZE_PICTURE];
	int InternalBufferSize=0;
	int VideoPid;
	int Videofps;
	int VideoBitrate;
	int FrameDuration;
	//int key_frame=1;
	uint8_t TsUdpBuffer[1316];
	char *OutputFilename;
	char *UdpOutput;
	//int FirstFrame=true;
	//struct timespec *TimeFirstFrame;
	 int        m_sock;
	 struct     sockaddr_in m_client;
	
	public:
	 TSEncaspulator(){};
	void SetOutput(char *FileName,char *Udp)
	{
		OutputFilename=FileName;
		 writer = ts_create_writer();
		UdpOutput=Udp;
		if(UdpOutput) udp_init();
	};


	void ConstructTsTree(int VideoBit,int TsBitrate,int PMTPid,char *sdt,int fps=25)
	{
		

		VideoPid=PMTPid+1;
		Videofps=fps;
		VideoBitrate=VideoBit;
		FrameDuration=1000/Videofps;
		tsmain.network_id = 1;
		tsmain.muxrate=TsBitrate;
		tsmain.cbr = 1;
		tsmain.ts_type = TS_TYPE_DVB;
		tsmain.pcr_period = 35;
		tsmain.pat_period = 400;
	        tsmain.sdt_period = 400;
        	tsmain.nit_period = 450;
        	tsmain.tdt_period = 1950;
        	tsmain.tot_period = 1950;
		tsmain.num_programs = 1;
		tsmain.programs=program;

		program[0].pmt_pid = PMTPid;
		program[0].program_num = 1;
		program[0].pcr_pid = VideoPid;
		program[0].num_streams = 1;
		program[0].streams=ts_stream;
		program[0].sdt = (sdt_program_ctx_t){
                    .service_type = DVB_SERVICE_TYPE_DIGITAL_TELEVISION,
                    .service_name = "Rpidatv",
                    .provider_name = sdt,
                };

		

		ts_stream[0].pid = VideoPid;
                ts_stream[0].stream_format = LIBMPEGTS_VIDEO_AVC;
                ts_stream[0].stream_id = LIBMPEGTS_STREAM_ID_MPEGVIDEO;
                ts_stream[0].dvb_au = 1;
                ts_stream[0].dvb_au_frame_rate = LIBMPEGTS_DVB_AU_25_FPS;//To be fixed : using framerate


		ts_setup_transport_stream(writer, &tsmain);
   		ts_setup_sdt(writer);
   		ts_setup_mpegvideo_stream(writer, VideoPid,
				     32,//3.2
                                     AVC_BASELINE, //Fixme should pass Profile and Level
                                     VideoBitrate,
                                     40000,//Fix Me : should have to be calculated
                                     Videofps);	
		if(OutputFilename)
			 vout = fopen(OutputFilename, "w+");
                    
	}

	void AddFrame(uint8_t *buffer,int size,int OmxFlags,uint64_t key_frame,int DelayPTS=200,struct timespec *Time=NULL)
	{
		 //unsigned char buffer[100];
		ts_frame_t tsframe;
		static int TimeToTransmitFrameUs=0;
		static int TotalFrameSize=0;
		int ret;
		int len;	
		/*if((FirstFrame==true)&&(Time!=NULL))
		{
			TimeFirstFrame.tv_sec=Time->tv_sec;
			TimeFirstFrame.tv_nsec=Time->tv_nsec;
			FirstFrame=false;	
			
		}*/
		if(OmxFlags&OMX_BUFFERFLAG_CODECCONFIG)
		{
			memcpy(InternalBuffer,buffer,size);
			InternalBufferSize+=size;
		}
		else
		if(OmxFlags&OMX_BUFFERFLAG_ENDOFFRAME)
		//if(OmxFlags&OMX_BUFFERFLAG_ENDOFNAL)
		{
			if((OmxFlags&OMX_BUFFERFLAG_ENDOFFRAME)&&!(OmxFlags&OMX_BUFFERFLAG_CODECCONFIG))
			{
				 //key_frame++;
				TotalFrameSize=0;
				TimeToTransmitFrameUs=0;
				//printf("-----\n");
			}
			
			if(OmxFlags&OMX_BUFFERFLAG_SYNCFRAME)
				tsframe.frame_type=LIBMPEGTS_CODING_TYPE_SLICE_IDR|LIBMPEGTS_CODING_TYPE_SLICE_I;
			else
				tsframe.frame_type=LIBMPEGTS_CODING_TYPE_SLICE_P;

			if(InternalBufferSize==0)
			{	
				tsframe.data=buffer;
				tsframe.size=size;
			}
	       		else
			{
				memcpy(InternalBuffer+InternalBufferSize,buffer,size);
				InternalBufferSize+=size;
				tsframe.data=InternalBuffer;
				tsframe.size=InternalBufferSize;
				InternalBufferSize=0;
			}
			
			tsframe.pid=VideoPid;
			int MaxVideoBitrate=tsmain.muxrate-10000; //MINUS SI/PSI
			TotalFrameSize+=tsframe.size;
			TimeToTransmitFrameUs= (TotalFrameSize*8.0*1000000.0*1.1/(float)MaxVideoBitrate);
			//if(OmxFlags&OMX_BUFFERFLAG_SYNCFRAME)
			if(Time==NULL)//Frame base calculation
			{
				//printf("IDR Image=%d TotalSize=%d Temps=%d\n",tsframe.size,TotalFrameSize,TimeToTransmitFrameUs);
				vdts=(key_frame*FrameDuration)*90L ; //TimeToTransmitFrameUs*90L/1000;
				vpts=(key_frame*FrameDuration)*90L; 	
				
				//tsframe.cpb_initial_arrival_time = vdts*300L -  DelayPTS*90*300L ;
	                	//tsframe.cpb_final_arrival_time = vdts*300L -  DelayPTS*90*300L ;
				tsframe.cpb_initial_arrival_time = vdts*300L - TimeToTransmitFrameUs*2.7- DelayPTS*90*300L ;
	                	tsframe.cpb_final_arrival_time = vdts*300L - TimeToTransmitFrameUs*2.7- DelayPTS*90*300L ;
				

			}
			else
			{
				//printf("%d:%d %lld\n",Time->tv_sec,Time->tv_nsec/(int64_t)1E6L,key_frame);
				vdts=(Time->tv_sec*1000+Time->tv_nsec/1000000.0)*90L ; //TimeToTransmitFrameUs*90L/1000;
				vpts=(Time->tv_sec*1000+Time->tv_nsec/1000000.0)*90L; 	
				
				//tsframe.cpb_initial_arrival_time = vdts*300L -  DelayPTS*90*300L ;
	                	//tsframe.cpb_final_arrival_time = vdts*300L -  DelayPTS*90*300L ;
				tsframe.cpb_initial_arrival_time = vdts*300L - TimeToTransmitFrameUs*2.7- DelayPTS*90*300L ;
	                	tsframe.cpb_final_arrival_time = vdts*300L - TimeToTransmitFrameUs*2.7- DelayPTS*90*300L ;
				
			}
	                tsframe.dts = vdts;
	                tsframe.pts = vpts;
	                tsframe.random_access = key_frame;
	                tsframe.priority = key_frame;
			tsframe.ref_pic_idc = 0; //Fixme (frame->pict_type == AV_PICTURE_TYPE_B) ? 1 : 0
			if(key_frame>1) //Skip first frame		
				ret = ts_write_frames(writer, &tsframe, 1, &out, &len, &pcr_list);
			else 
				len=0;
			if (len)
			{
				/*if(len>10000)
				{
					printf("TimeToTransmitFrameUs=%d %d bitrate=%d\n",TimeToTransmitFrameUs,len,len*8*Videofps);
					fprintf(stderr, "Muxed VIDEO len: %d %d\n", len, ret);
				}*/	
				static struct timespec gettime_now,gettime_first;
				long time_difference;
				clock_gettime(CLOCK_REALTIME, &gettime_now);
				time_difference = gettime_now.tv_nsec - gettime_first.tv_nsec;
				if(time_difference<0) time_difference+=1E9L;
				
				clock_gettime(CLOCK_REALTIME, &gettime_first);
	
				if(vout)
				{
					int n,ret;
					ret=ioctl(fileno(vout), FIONREAD, &n);
					if(n>40000) 
						printf("Overflow outpipe %ld Pipe %d\n",time_difference,n);
			
					 fwrite(out, 1, len, vout);
				}
				if(UdpOutput) udp_send(out,len);
				clock_gettime(CLOCK_REALTIME, &gettime_now);
				time_difference = gettime_now.tv_nsec - gettime_first.tv_nsec;
				if(time_difference<0) time_difference+=1E9;
				//if(time_difference>5000000) printf("Overflow ! timetowrite=%ld\n",time_difference);
			}
			else
			{
				fprintf(stderr, "tswrite frame Len=0 Ret=%d tsframe.size=%d originalsize=%d\n",ret,tsframe.size,size);
			}	
		}
	}
	     
	void udp_send( u_int8_t *b, int len )
	{
		#define BUFF_MAX_SIZE (7*188)
		static u_int8_t Buffer[BUFF_MAX_SIZE];
		static int Size=0;
		while(len>0)
		{
			if(Size+len>=BUFF_MAX_SIZE)
			{
				 memcpy(Buffer+Size,b,BUFF_MAX_SIZE-Size);
				b+=(BUFF_MAX_SIZE-Size);
				len-=(BUFF_MAX_SIZE-Size);
				if(sendto(m_sock, Buffer, BUFF_MAX_SIZE, 0,(struct sockaddr *) &m_client, sizeof(m_client))<0)
				{
	        			printf("UDP send failed\n");
			    	}
				Size=0;
			}
			else
			{
				memcpy(Buffer+Size,b,len);
				b+=len;
				Size+=len;
				len=0;
			
			}
		}
/*
    	if(sendto(m_sock, b, len, 0,(struct sockaddr *) &m_client, sizeof(m_client))<0){
        printf("UDP send failed\n");
    	}*/
	}
void udp_set_ip(const char *ip )
{
    char text[40];
    char *add[2];
    u_int16_t sock;

    strcpy(text,ip);
    add[0] = strtok(text,":");
    add[1] = strtok(NULL,":");
    if(strlen(add[1])==0)
        sock = 1314;
    else
        sock = atoi(add[1]);
    // Construct the client sockaddr_in structure
    memset(&m_client, 0, sizeof(m_client));      // Clear struct
    m_client.sin_family      = AF_INET;          // Internet/IP
    m_client.sin_addr.s_addr = inet_addr(add[0]);  // IP address
    m_client.sin_port        = htons(sock);      // server socket
}
void udp_init(void)
{
    // Create a socket for transmitting UDP TS packets
    if ((m_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        printf("Failed to create socket\n");
        return;
    }
    udp_set_ip(UdpOutput);
}
};

using namespace rpi_omx;

class CameraTots
{
private:
	VideoFromat VideoFormat;
	Camera camera;
	 Encoder encoder;
	TSEncaspulator tsencoder;
	VideoRenderer videorender;
	int EncVideoBitrate;
	bool FirstTime=true;
	uint64_t key_frame=10;
	VideoFromat CurrentVideoFormat;
	int DelayPTS;
	struct timespec InitTime;
public:
	void Init(VideoFromat &VideoFormat,char *FileName,char *Udp,int VideoBitrate,int TsBitrate,int SetDelayPts,int PMTPid,char *sdt,int fps=25,int IDRPeriod=100,int RowBySlice=0,int EnableMotionVectors=0)
	{	
		CurrentVideoFormat=VideoFormat;
		DelayPTS=SetDelayPts;
		 // configuring camera
		camera.setVideoFromat(VideoFormat);
		camera.setImageDefaults();
            camera.setImageFilter(OMX_ALL, OMX_ImageFilterNoise);

            while (!camera.ready())
            {
                std::cerr << "waiting for camera..." << std::endl;
                usleep(10000);
            } 
	
	 // configuring encoders
		{
		    VideoFromat vfResized = VideoFormat;
		    
		    Parameter<OMX_PARAM_PORTDEFINITIONTYPE> portDef;
		    camera.getPortDefinition(Camera::OPORT_VIDEO, portDef);

		    videorender.setupPortFromCamera(portDef);
		videorender.SetDestRect(0,0,640,480);
		    
		    portDef->format.video.nFrameWidth = vfResized.width;
		    portDef->format.video.nFrameHeight = vfResized.height;
	
		    encoder.setupOutputPortFromCamera(portDef, VideoBitrate*2);
		    encoder.setBitrate(VideoBitrate*2,OMX_Video_ControlRateVariable/*OMX_Video_ControlRateConstant*/);
		    encoder.setCodec(OMX_VIDEO_CodingAVC);
		    encoder.setIDR(IDRPeriod);	
		    encoder.setSEIMessage();
		    if(EnableMotionVectors) encoder.setVectorMotion();
	
			encoder.setQFromBitrate(VideoBitrate,fps,CurrentVideoFormat.width,CurrentVideoFormat.height);

			encoder.setLowLatency();
			encoder.setSeparateNAL();
			if(RowBySlice)
				encoder.setMultiSlice(RowBySlice);
			else
				encoder.setMinizeFragmentation();
		    //encoder.setEED();

	/*OMX_VIDEO_AVCProfileBaseline = 0x01,   //< Baseline profile 
	    OMX_VIDEO_AVCProfileMain     = 0x02,   //< Main profile 
	    OMX_VIDEO_AVCProfileExtended = 0x04,   //< Extended profile 
	    OMX_VIDEO_AVCProfileHigh     = 0x08,   //< High profile 
		OMX_VIDEO_AVCProfileConstrainedBaseline
	*/
		    encoder.setProfileLevel(OMX_VIDEO_AVCProfileHigh);

			// With Main Profile : have more skipped frame
			tsencoder.SetOutput(FileName,Udp);
		   tsencoder.ConstructTsTree(VideoBitrate,TsBitrate,PMTPid,sdt,fps); 	
		 EncVideoBitrate=VideoBitrate;
	
		    //encoder.setPeakRate(VIDEO_BITRATE_LOW/1000);
		    //encoder.setMaxFrameLimits(10000*8);
		}
ERR_OMX( OMX_SetupTunnel(camera.component(), Camera::OPORT_VIDEO, encoder.component(), Encoder::IPORT),
                "tunnel camera.video -> encoder.input");
ERR_OMX( OMX_SetupTunnel(camera.component(), Camera::OPORT_PREVIEW, videorender.component(), VideoRenderer::IPORT),
                "tunnel camera.video -> renderer");
		// switch components to idle state
		{
		    camera.switchState(OMX_StateIdle);

		    encoder.switchState(OMX_StateIdle);
		videorender.switchState(OMX_StateIdle);
			 
		}

		// enable ports
		{
		    camera.enablePort(Camera::IPORT);
		    camera.enablePort(Camera::OPORT_VIDEO);
			camera.enablePort(Camera::OPORT_PREVIEW);
		   videorender.enablePort(VideoRenderer::IPORT);
		    
		encoder.enablePort();    // all
		}

		// allocate buffers
		{
		    camera.allocBuffers();
		    //videorender.allocBuffers();
		    encoder.allocBuffers();
		}

		// switch state of the components prior to starting
		{
			videorender.switchState(OMX_StateExecuting);
		    camera.switchState(OMX_StateExecuting);
		    encoder.switchState(OMX_StateExecuting);
		}

		// start capturing video with the camera
		{
		    camera.capture(Camera::OPORT_VIDEO, OMX_TRUE);
		}
	
	}

	void Run(bool want_quit)
	{
		Buffer& encBuffer = encoder.outBuffer();
		if(FirstTime) 
		{
			clock_gettime(CLOCK_REALTIME, &InitTime);
			FirstTime=false;
			encoder.callFillThisBuffer();
		}
		if (!want_quit&&encBuffer.filled())
		 {
			       //encoder.getEncoderStat(encBuffer.flags());
	      			
				//encoder.setDynamicBitrate(EncVideoBitrate);
				//encoder.setQP(20,20);
				//printf("Len = %"\n",encBufferLow
				if(encBuffer.flags() & OMX_BUFFERFLAG_CODECSIDEINFO)
				{
					printf("CODEC CONFIG>\n");
					int LenVector=encBuffer.dataSize();
					 
					for(int j=0;j<CurrentVideoFormat.height/16;j++)
					{
						for(int i=0;i<CurrentVideoFormat.width/16;i++)
						{
							int Motionx=encBuffer.data()[(CurrentVideoFormat.width/16*j+i)*4];
							int Motiony=encBuffer.data()[(CurrentVideoFormat.width/16*j+i)*4+1];
							int MotionAmplitude=sqrt((double)((Motionx * Motionx) + (Motiony * Motiony)));
							printf("%d ",MotionAmplitude);
						}
						printf("\n");
					}
					encBuffer.setFilled(false);
					encoder.callFillThisBuffer();
					return;
				}
						
				unsigned toWrite = (encBuffer.dataSize()) ;
				
		 		
				if (toWrite)
				{
			       
					int OmxFlags=encBuffer.flags();
					if((OmxFlags&OMX_BUFFERFLAG_ENDOFFRAME)&&!(OmxFlags&OMX_BUFFERFLAG_CODECCONFIG))	key_frame++;
					struct timespec gettime_now;
				
							clock_gettime(CLOCK_REALTIME, &gettime_now);
							gettime_now.tv_sec=(int)difftime(gettime_now.tv_sec,InitTime.tv_sec);
	//			tsencoder.AddFrame(encBuffer.data(),encBuffer.dataSize(),OmxFlags,key_frame,DelayPTS,&gettime_now);

					tsencoder.AddFrame(encBuffer.data(),encBuffer.dataSize(),OmxFlags,key_frame,DelayPTS);
			
	
		
				}
				else
				{
					key_frame++; //Skipped Frame, key_frame++ to allow correct timing for next valid frames
					printf("!");
				}
				// Buffer flushed, request a new buffer to be filled by the encoder component
				encBuffer.setFilled(false);
				encoder.callFillThisBuffer();
		  }
			else
				usleep(1000);	
			   
	}


	void Terminate()
	{ 
		// stop capturing video with the camera
		{
		    camera.capture(Camera::OPORT_VIDEO, OMX_FALSE);
		}

		// return the last full buffer back to the encoder component
		{
		    encoder.outBuffer().flags() &= OMX_BUFFERFLAG_EOS;

		 
		    //encoder.callFillThisBuffer();
		}

		// flush the buffers on each component
		{
		    camera.flushPort();
		videorender.flushPort();
		    encoder.flushPort();
		}

		// disable all the ports
		{
		    camera.disablePort();
			videorender.disablePort();
		    encoder.disablePort();
		}

		// free all the buffers
		{
		    camera.freeBuffers();
		//videorender.freeBuffers();
		    encoder.freeBuffers();
		}

		// transition all the components to idle states
		{
		    camera.switchState(OMX_StateIdle);
		videorender.switchState(OMX_StateIdle);
		    encoder.switchState(OMX_StateIdle);
		}

		// transition all the components to loaded states
		{
		    camera.switchState(OMX_StateLoaded);
		videorender.switchState(OMX_StateLoaded);
		    encoder.switchState(OMX_StateLoaded);
		}
	}
};

class PictureTots
{
private:
	
	
	 Encoder encoder;
	TSEncaspulator tsencoder;
	Resizer resizer;
	//ImageEncode colorconverter; 
	int EncVideoBitrate;
	bool FirstTime=true;
	uint key_frame=0;
	VideoFromat CurrentVideoFormat;
	int DelayPTS;
	int Videofps;
	struct timespec last_time;
	int Mode=Mode_PATTERN;
	Webcam *pwebcam;
	GrabDisplay *pgrabdisplay;
	VncClient *pvncclient;
	struct timespec InitTime;


public:
static const int Mode_PATTERN=0;
		static const int Mode_V4L2=1;
static const int Mode_GRABDISPLAY=2;
static const int Mode_VNCCLIENT=3;
public:
	void Init(VideoFromat &VideoFormat,char *FileName,char *Udp,int VideoBitrate,int TsBitrate,int SetDelayPts,int PMTPid,char* sdt,int fps=25,int IDRPeriod=100,int RowBySlice=0,int EnableMotionVectors=0,int ModeInput=Mode_PATTERN,char *Extra=NULL)
	{
		last_time.tv_sec=0;
		last_time.tv_nsec=0;
		CurrentVideoFormat=VideoFormat;
		Videofps=fps;
		DelayPTS=SetDelayPts;
		EncVideoBitrate=VideoBitrate;
		Mode=ModeInput;
		
		if(Mode==Mode_V4L2)
		{
			pwebcam=new Webcam(Extra);
			int CamWidth,CamHeight;
			pwebcam->GetCameraSize(CamWidth,CamHeight);
			printf("Resizer input = %d x %d\n",CamWidth,CamHeight);
			
			resizer.setupOutputPort(CamWidth,CamHeight,VideoFormat,OMX_COLOR_FormatYUV420PackedPlanar);
		}
		if(Mode==Mode_PATTERN)
		{
		 resizer.setupOutputPort(VideoFormat.width,VideoFormat.height,VideoFormat,OMX_COLOR_FormatYUV420PackedPlanar);
		}
		if(Mode==Mode_GRABDISPLAY)
		{
			pgrabdisplay=new GrabDisplay(0);
			int DisplayWidth,DisplayHeight,Rotate;

			pgrabdisplay->GetDisplaySize(DisplayWidth,DisplayHeight,Rotate);
			printf("Resizer input = %d x %d\n",DisplayWidth,DisplayHeight);
			resizer.setupOutputPort(DisplayWidth,DisplayHeight,VideoFormat, OMX_COLOR_Format32bitABGR8888);
		}
		if(Mode==Mode_VNCCLIENT)
		{
			printf("Connecting to VNCSERVER %s...\n",Extra);
			pvncclient=new VncClient(Extra,"datv");
			int DisplayWidth,DisplayHeight,Rotate;

			pvncclient->GetDisplaySize(DisplayWidth,DisplayHeight,Rotate);
			printf("Resizer input = %d x %d\n",DisplayWidth,DisplayHeight);
			resizer.setupOutputPort(DisplayWidth,DisplayHeight,VideoFormat, OMX_COLOR_Format32bitABGR8888);
		}
		//resizer.setupOutputPort(VideoFormat,OMX_COLOR_Format32bitABGR8888);//OK
	
			// configuring encoders
		
		    VideoFromat vfResized = VideoFormat;
		    
		   
		 encoder.setupOutputPort(VideoFormat,VideoBitrate*2,fps);
		    //OMX_Video_ControlRateDisable seems not supported !!!
			 encoder.setCodec(OMX_VIDEO_CodingAVC);
		    encoder.setBitrate(VideoBitrate*2,OMX_Video_ControlRateVariable/*OMX_Video_ControlRateConstant*/);
		   
		    encoder.setIDR(IDRPeriod);	
		    encoder.setSEIMessage();
		    if(EnableMotionVectors) encoder.setVectorMotion();
			encoder.setQFromBitrate(VideoBitrate,Videofps,CurrentVideoFormat.width,CurrentVideoFormat.height);
			//encoder.setQPLimits(30,30);
			//encoder.setQP(24,24);
			encoder.setLowLatency();
			encoder.setSeparateNAL();
			if(RowBySlice)
				encoder.setMultiSlice(RowBySlice);
			else
				encoder.setMinizeFragmentation();
		    //encoder.setEED();

	/*OMX_VIDEO_AVCProfileBaseline = 0x01,   //< Baseline profile 
	    OMX_VIDEO_AVCProfileMain     = 0x02,   //< Main profile 
	    OMX_VIDEO_AVCProfileExtended = 0x04,   //< Extended profile 
	    OMX_VIDEO_AVCProfileHigh     = 0x08,   //< High profile 
		OMX_VIDEO_AVCProfileConstrainedBaseline
	*/
		    encoder.setProfileLevel(OMX_VIDEO_AVCProfileHigh);

			// With Main Profile : have more skipped frame
			tsencoder.SetOutput(FileName,Udp);
		   tsencoder.ConstructTsTree(VideoBitrate,TsBitrate,PMTPid,sdt,fps); 	
		 printf("Ts bitrate = %d\n",TsBitrate);
		


		ERR_OMX( OMX_SetupTunnel(resizer.component(), Resizer::OPORT, encoder.component(), Encoder::IPORT),         "tunnel resizer.output -> encoder.input (low)");
	

		// switch components to idle state
		{
		
			resizer.switchState(OMX_StateIdle);
		      encoder.switchState(OMX_StateIdle);
			 
		}

		// enable ports
		{
			
		  	resizer.enablePort();  		    
			encoder.enablePort();    // all
		}

		// allocate buffers
		
		  
		    resizer.allocBuffers();
		  
		    if(Mode==Mode_V4L2)
		    {
				 pwebcam->SetOmxBuffer((unsigned char*)resizer.inBuffer().data());
		    }
			if(Mode==Mode_GRABDISPLAY)
			{
				pgrabdisplay->SetOmxBuffer((unsigned char*)resizer.inBuffer().data());
			}
		if(Mode==Mode_VNCCLIENT)
		{	
			pvncclient->SetOmxBuffer((unsigned char*)resizer.inBuffer().data());
			}
		    printf("Allocsize= %d\n",resizer.inBuffer().allocSize());
		    encoder.allocBuffers(false);//Only  Bufout
		

		// switch state of the components prior to starting
		{
			
		  resizer.switchState(OMX_StateExecuting);
		    encoder.switchState(OMX_StateExecuting);
		}

	}

void usleep_exactly(long MuToSleep )
{
	#define KERNEL_GRANULARITY 1000000
	#define MARGIN 500
	struct timespec gettime_now;
	long time_difference;
	if(last_time.tv_sec==0) clock_gettime(CLOCK_REALTIME, &last_time);
	clock_gettime(CLOCK_REALTIME, &gettime_now);
	time_difference = gettime_now.tv_nsec - last_time.tv_nsec;
	if(time_difference<0) time_difference+=1E9;

	long BigToSleepns=(MuToSleep*1000L-time_difference-KERNEL_GRANULARITY);
	//printf("ToSleep %ld\n",BigToSleepns/1000);
	if(BigToSleepns<KERNEL_GRANULARITY)
	{
	  last_time=gettime_now;
	  return;
	}
	
	usleep(BigToSleepns/1000);
	do
	{
		clock_gettime(CLOCK_REALTIME, &gettime_now);
		time_difference = gettime_now.tv_nsec - last_time.tv_nsec;
		if(time_difference<0) time_difference+=1E9;
		//printf("#");
	}
	while(time_difference<(MuToSleep*1000L-MARGIN));	
	//printf("TimeDiff =%ld\n",time_difference);
	last_time=gettime_now;
			
}

// generate an animated test card in YUV format
 int generate_test_card(OMX_U8 *buf, OMX_U32 * filledLen, int frame)
{
   int i, j;
	frame=frame%256;
   OMX_U8 *y = buf, *u = y + CurrentVideoFormat.width * CurrentVideoFormat.height, *v =
      u + (CurrentVideoFormat.width >> 1) * (CurrentVideoFormat.height >> 1);

   for (j = 0; j < CurrentVideoFormat.height / 2; j++) {
      OMX_U8 *py = y + 2 * j * CurrentVideoFormat.width;
      OMX_U8 *pu = u + j * (CurrentVideoFormat.width >> 1);
      OMX_U8 *pv = v + j * (CurrentVideoFormat.width >> 1);
      for (i = 0; i < CurrentVideoFormat.width / 2; i++) {
	 int z = (((i + frame) >> 4) ^ ((j + frame) >> 4)) & 15;
	 py[0] = py[1] = py[CurrentVideoFormat.width] = py[CurrentVideoFormat.width + 1] = 0x80 + frame * 0x8;
	 pu[0] = frame;//0x00 + z * 0x10;
	 pv[0] = frame;//0x80 + z * 0x30;
	 py += 2;
	 pu++;
	 pv++;
      }
   }

   *filledLen = ((CurrentVideoFormat.width * CurrentVideoFormat.height * 3)/2);
  
   return 1;
}

 int generate_test_rgbcard(OMX_U8 *buf, OMX_U32 * filledLen, int frame)
{
	OMX_U8 *current=buf;
   for(int j=0;j<CurrentVideoFormat.height;j++)
	for(int i=0;i<CurrentVideoFormat.width;i++)
	{	
		*current++=255;		
		*current++=frame%256;
		*current++=i%128;
		*current++=frame%256;
		
	}		
   *filledLen = ((CurrentVideoFormat.width * CurrentVideoFormat.height * 4));
  
   return 1;
}

/*
int ConvertColor(OMX_U8 *out,OMX_U8 *in,int Size)
{
	OMX_U8 *inprocess=in;
	int Width=(fmt.fmt.pix.width>>5)<<5;
	int WidthMissing=fmt.fmt.pix.width-((fmt.fmt.pix.width>>5)<<5);
	int Height=(fmt.fmt.pix.height>>4)<<4;
	OMX_U8 *PlanY=out;
	OMX_U8 *PlanU=out+Width*Height;
	OMX_U8 *PlanV=PlanU+(Width*Height)/4;
	
	//printf("WidthMissin %d\n",WidthMissing);
	int count=0;
	for(int j=0;j<Height;j++)
	{
		for(int i=0;i<Width/2;i++)
		{
			
			*(PlanU)=*(inprocess++);
			if((j%2==0)) PlanU++;
			*(PlanY++)=*(inprocess++);
			*(PlanV)=*(inprocess++);
			if((j%2==0)) PlanV++;
			*(PlanY++)=*(inprocess++);
			
		}
		count+=4*Width/2;
		if(count>Size) return 0;
		inprocess+=WidthMissing*2;
		count+=WidthMissing*2;
	}
	//printf("Count =%d\n",count);
}
*/


void Run(bool want_quit)
	{
		Buffer& encBuffer = encoder.outBuffer();
		Buffer& PictureBuffer = resizer.inBuffer();
		static int QP=45;
		
		if(!want_quit&&(FirstTime||PictureBuffer.filled()))
		{
			
			OMX_U32 filledLen;
//			generate_test_card(PictureBuffer.data(),&filledLen,key_frame);
			if(Mode==Mode_PATTERN)
			{
			generate_test_card(PictureBuffer.data(),&filledLen,key_frame);
		
			usleep_exactly(1e6/Videofps);
			}
			if(Mode==Mode_V4L2)
			{
				
				auto frame = pwebcam->frame(2);
				
				filledLen=frame.size;
				
				//V4L2_read_frame(PictureBuffer.data(),&filledLen);
				//usleep_exactly(1e6/(Videofps*2));
			}
			if(Mode==Mode_GRABDISPLAY)
			{
				
				pgrabdisplay->GetPicture();
				int DisplayWidth,DisplayHeight,Rotate;

				pgrabdisplay->GetDisplaySize(DisplayWidth,DisplayHeight,Rotate);
				filledLen=PictureBuffer.allocSize();//DisplayWidth*DisplayHeight*4;
				//printf("%d filled\n",filledLen);
				usleep_exactly(1e6/Videofps);
			}
			
			if(Mode==Mode_VNCCLIENT)
			{
				
				int FrameDiff=pvncclient->GetPicture(Videofps);
				
				filledLen=PictureBuffer.allocSize();
				//printf("%d filled\n",filledLen);
				if(FrameDiff==0)
					usleep_exactly(1e6/Videofps);
				else
				{
					//usleep_exactly((FrameDiff+1)*1e6/Videofps);
					key_frame+=FrameDiff;
					//printf("DiffFrame %d\n",FrameDiff);
				}
			}
			PictureBuffer.setDatasize(filledLen);
			PictureBuffer.setFilled(false);
			resizer.callEmptyThisBuffer();
			
			
			if(FirstTime)
			{
				clock_gettime(CLOCK_REALTIME, &InitTime);
				FirstTime=false;
				encoder.callFillThisBuffer();
				
			}
			
		}
		if (!want_quit&&encBuffer.filled())
		 {
			       
	      			//encoder.getEncoderStat(encBuffer.flags());
				//encoder.setDynamicBitrate(EncVideoBitrate);
				//printf("Len = %"\n",encBufferLow
				/*if(key_frame%250==0)
				{
					QP--;	
					encoder.setQPLimits(QP,QP);
					printf("------ QP =%d\n",QP);
				}*/
				if(encBuffer.flags() & OMX_BUFFERFLAG_CODECSIDEINFO)
				{
					printf("CODEC CONFIG>\n");
					int LenVector=encBuffer.dataSize();
					 
					for(int j=0;j<CurrentVideoFormat.height/16;j++)
					{
						for(int i=0;i<CurrentVideoFormat.width/16;i++)
						{
							int Motionx=encBuffer.data()[(CurrentVideoFormat.width/16*j+i)*4];
							int Motiony=encBuffer.data()[(CurrentVideoFormat.width/16*j+i)*4+1];
							int MotionAmplitude=sqrt((double)((Motionx * Motionx) + (Motiony * Motiony)));
							printf("%d ",MotionAmplitude);
						}
						printf("\n");
					}
					encBuffer.setFilled(false);
					encoder.callFillThisBuffer();
					return;
				}
						
				unsigned toWrite = (encBuffer.dataSize()) ;
				
		 		
				if (toWrite)
				{
			       
					int OmxFlags=encBuffer.flags();
					if((OmxFlags&OMX_BUFFERFLAG_ENDOFFRAME)&&!(OmxFlags&OMX_BUFFERFLAG_CODECCONFIG))	key_frame++;
						
							struct timespec gettime_now;
				
							clock_gettime(CLOCK_REALTIME, &gettime_now);
//printf("Avnt %ld:%ld - %ld:%ld \n",gettime_now.tv_sec,gettime_now.tv_nsec,InitTime.tv_sec,InitTime.tv_nsec);
							//gettime_now.tv_sec=(int)difftime(gettime_now.tv_sec,InitTime.tv_sec);
gettime_now.tv_sec=gettime_now.tv_sec-InitTime.tv_sec;
if(gettime_now.tv_nsec<InitTime.tv_nsec)
{	
	
	gettime_now.tv_nsec=((int64_t)1E9L+(int64_t)gettime_now.tv_nsec)-(int64_t)InitTime.tv_nsec;
	  gettime_now.tv_sec-=1;
}
else
{
	gettime_now.tv_nsec=gettime_now.tv_nsec-(int64_t)InitTime.tv_nsec;
}

				tsencoder.AddFrame(encBuffer.data(),encBuffer.dataSize(),OmxFlags,key_frame,DelayPTS,&gettime_now);
						
						
						//tsencoder.AddFrame(encBuffer.data(),encBuffer.dataSize(),OmxFlags,key_frame,DelayPTS);
						
					
			
	
		
				}
				else
				{
					//usleep_exactly(1e6/(2*Videofps));
					key_frame++; //Skipped Frame, key_frame++ to allow correct timing for next valid frames
					printf("!%ld\n",key_frame);
				}
				
				// Buffer flushed, request a new buffer to be filled by the encoder component
				encBuffer.setFilled(false);
				//PictureBuffer.setFilled(true);
				encoder.callFillThisBuffer();
				
				
		  }
		else usleep(1000);
			   
	}


	void Terminate()
	{ 
		
		if(Mode==Mode_V4L2)
		{
			free(pwebcam);
		}
		if(Mode==Mode_GRABDISPLAY)
		{
			free(pgrabdisplay);
		}
		if(Mode==Mode_VNCCLIENT)
		{
			free(pvncclient);
		}	
		// return the last full buffer back to the encoder component
		{
		    encoder.outBuffer().flags() &= OMX_BUFFERFLAG_EOS;

		 
		    //encoder.callFillThisBuffer();
		}

		// flush the buffers on each component
		{
		   
		    resizer.flushPort();
		    encoder.flushPort();
		}

		// disable all the ports
		{
		  
		    resizer.disablePort();
		    encoder.disablePort();
		}

		// free all the buffers
		{
		  
		   resizer.freeBuffers();	
		    encoder.freeBuffers();
		}

		// transition all the components to idle states
		{
		
		    resizer.switchState(OMX_StateIdle);  
		    encoder.switchState(OMX_StateIdle);
		}

		// transition all the components to loaded states
		{
		  
			resizer.switchState(OMX_StateLoaded);
		    encoder.switchState(OMX_StateLoaded);
		}
	}
};
	


void print_usage()
{

fprintf(stderr,\
"\navc2ts -%s\n\
Usage:\nrpi-avc2ts  -o OutputFile -b BitrateVideo -m BitrateMux -x VideoWidth  -y VideoHeight -f Framerate -n MulticastGroup [-d PTS/PCR][-v][-h] \n\
-o            path to Transport File Output \n\
-b            VideoBitrate in bit/s \n\
-m            Multiplex Bitrate (should be around 1.4 VideoBitrate)\n\
-x            VideoWidth (should be 16 pixel aligned)\n\
-y 	      VideoHeight (should be 16 pixel aligned)\n\
-f            Framerate (25 for example)\n\
-n 	      Multicast group (optionnal) example 230.0.0.1:10000\n\
-d 	      Delay PTS/PCR in ms\n\
-v	      Enable Motion vectors\n\
-i	      IDR Period\n\
-t		TypeInput {0=Picamera,1=InternalPatern,2=USB Camera,3=Rpi Display,4=VNC}\n\
-e 		Extra Arg:\n\
			- For usb camera name of device (/dev/video0)\n\
			- For VNC : IP address of VNC Server. Password must be datv\n\
-p 		Set the PidStart: Set PMT=PIDStart,Pidvideo=PidStart+1,PidAudio=PidStart+2\n\
-s 		Set Servicename : Typically CALL\n\
-h            help (print this help).\n\
Example : ./rpi-avc2ts -o result.ts -b 1000000 -m 1400000 -x 640 -y 480 -f 25 -n 230.0.0.1:1000\n\
\n",\
PROGRAM_VERSION);

} /* end function print_usage */



int main(int argc, char **argv)
{

	int a;
	int anyargs = 0;
	char *OutputFileName=NULL;//"out.ts"
	int VideoBitrate=300000;
	int MuxBitrate=400000;
	int VideoWidth=352;
	int VideoHeight=288;
	int VideoFramerate=25;
	int IDRPeriod=100;
	int DelayPTS=200;
	int RowBySlice=0;
	char *NetworkOutput=NULL;//"230.0.0.1:10000";
	int EnableMotionVectors=0;
	char *ExtraArg=NULL;
	char *sdt="F5OEO";
	int pidpmt=255,pidvideo=256,pidaudio=257;
	#define CAMERA 0
	#define PATTERN 1
	#define USB_CAMERA 2
	#define DISPLAY 3
	#define VNC 4		
	int TypeInput=CAMERA;

	while(1)
	{
	a = getopt(argc, argv, "o:b:m:hx:y:f:n:d:i:r:vt:e:p:s:");
	
	if(a == -1) 
	{
		if(anyargs) break;
		else a='h'; //print usage and exit
	}
	anyargs = 1;	

	switch(a)
		{
		case 'o': // Outputfile
			OutputFileName=optarg;
			
			break;
		case 'b': // BitrateVideo
			VideoBitrate = atoi(optarg);
			break;
		case 'm': // Mux
			MuxBitrate=atoi(optarg);
			
			break;
		case 'h': // help
			print_usage();
			exit(1);
			break;
		case 'x': // Width
			VideoWidth=atoi(optarg);
			VideoWidth=((VideoWidth>>5)<<5);
			break;
		case 'y': // Height
			VideoHeight=atoi(optarg);
			VideoHeight=((VideoHeight>>4)<<4);
			break;
		case 'f': // Framerate
			VideoFramerate=atoi(optarg);
			break;
		case 'n': // Network
			NetworkOutput=optarg;
			break;
		case 'd': // PTS/PCR
			DelayPTS=atoi(optarg);
			break;
		case 'i': // IDR PERIOD
			IDRPeriod=atoi(optarg);
			break;
		case 'v': // Motion Vectors
			EnableMotionVectors=1;
			break;
		case 'r': // Rows by slice
			RowBySlice=atoi(optarg);
			break;
		case 't': //Type input
			TypeInput=atoi(optarg);
			break;
		case 'e': //Type input extra arg
			ExtraArg=optarg;
			break;
		case 'p': //Pid Start
			pidpmt=atoi(optarg);
			pidvideo=atoi(optarg)+1;
			pidaudio=atoi(optarg)+2;
			break;
		case 's': //Service sname : sdt
			sdt=optarg;
			break;
		case -1:
        	break;
		case '?':
			if (isprint(optopt) )
 				{
 				fprintf(stderr, "avc2ts: unknown option `-%c'.\n", optopt);
 				}
			else
				{
				fprintf(stderr, "avc2ts: unknown option character `\\x%x'.\n", optopt);
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

/*struct VideoFromat
    {
        typedef enum
        {
            RATIO_4x3,
            RATIO_16x9
        } Ratio;

        unsigned width;
        unsigned height;
        unsigned framerate;
        Ratio ratio;
        bool fov;
    };
*/
 VideoFromat CurrentVideoFormat;
CurrentVideoFormat.width=VideoWidth;
CurrentVideoFormat.height=VideoHeight;
CurrentVideoFormat.framerate=VideoFramerate;
CurrentVideoFormat.ratio=VideoFromat::RATIO_16x9;
CurrentVideoFormat.fov=false; // To check
 
bcm_host_init();
	try
    {
        OMXInit omx;
        VcosSemaphore sem("common semaphore");
        pSemaphore = &sem;

        CameraTots *cameratots;
	PictureTots *picturetots;
if(TypeInput==0)
{
	cameratots=new CameraTots; 
	cameratots->Init(CurrentVideoFormat,OutputFileName,NetworkOutput,VideoBitrate,MuxBitrate,DelayPTS,pidpmt,sdt,VideoFramerate,IDRPeriod,RowBySlice,false);
}
else
{
	int PictureMode=PictureTots::Mode_PATTERN;
	switch(TypeInput)
	{
	case PATTERN:PictureMode=PictureTots::Mode_PATTERN;break;
	case USB_CAMERA:PictureMode=PictureTots::Mode_V4L2;
	if(ExtraArg==NULL) ExtraArg="/dev/video0";break;
	case DISPLAY:PictureMode=PictureTots::Mode_GRABDISPLAY;break;
	case VNC:PictureMode=PictureTots::Mode_VNCCLIENT;
	if(ExtraArg==NULL) {printf("IP of VNCServer should be set with -e option\n");exit(0);}
	
	}
	picturetots = new PictureTots;
	picturetots->Init(CurrentVideoFormat,OutputFileName,NetworkOutput,VideoBitrate,MuxBitrate,DelayPTS,pidpmt,sdt,VideoFramerate,IDRPeriod,RowBySlice,false,PictureMode,ExtraArg);
}	
#if 1
        signal(SIGINT,  signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGQUIT, signal_handler);
	signal(SIGKILL, signal_handler);
	signal(SIGPIPE, signal_handler);
#endif



        std::cerr << "Enter capture and encode loop, press Ctrl-C to quit..." << std::endl;

        unsigned highCount = 0;
        unsigned lowCount = 0;
        unsigned noDataCount = 0;
       
       

	     
	
        while (1)
        {
            
	   if(TypeInput==0)
		cameratots->Run(want_quit);
	else
          	 picturetots->Run(want_quit);
	
	   

                if (want_quit /*&& (encBufferLow.flags() & OMX_BUFFERFLAG_SYNCFRAME)*/)
                {
                    std::cerr << "Clean Exiting avc2ts" << std::endl;
                    break;
                }


              
            
        }

        std::cerr << "high: " << highCount << " low: " << lowCount << std::endl;

#if 1
        // Restore signal handlers
        signal(SIGINT,  SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
	signal(SIGKILL, SIG_DFL);
#endif
	if(TypeInput==0)
	{
		cameratots->Terminate();
		delete(cameratots);
	}
	else
	{
          	picturetots->Terminate();
		delete(picturetots);
	}
		
       
    }
    catch (const OMXExeption& e)
    {
        OMXExeption::die(e.code(), e.what());
    }
    catch (const char * msg)
    {
        std::cerr << msg;
        return 1;
    }

    return 0;
}
