/** Small C++ wrapper around V4L example code to access the webcam
**/

#include <string>
#include <memory> // unique_ptr
#include <linux/videodev2.h>
struct buffer {
      void   *data;
      size_t  size;
};

struct YUV420Image {
      unsigned char   *data; // YUV420
      size_t          width;
      size_t          height;
      size_t          size; // width * height * 3/2
};


class Webcam {

public:
    Webcam(const std::string& device = "/dev/video0");

    ~Webcam();

    void GetCameraSize(int& Width,int& Height);
	int ConvertColor(unsigned char *out,unsigned char *in);
    void SetOmxBuffer(unsigned char* Buffer);
    /** Captures and returns a frame from the webcam.
     *
     * The returned object contains a field 'data' with the image data in YUV420
     * format 
     * This call blocks until a frame is available or until the provided
     * timeout (in seconds). 
     *
     * Throws a runtime_error if the timeout is reached.
     */
    const YUV420Image& frame(int timeout = 1);

private:
    void init_mmap();

    void open_device();
    void close_device();

    void init_device();
    void uninit_device();

    void start_capturing();
    void stop_capturing();

    bool read_frame();

    std::string device;
    int fd;

    YUV420Image yuv420frame;
    struct buffer          *buffers;
    unsigned int     n_buffers;

    size_t xres, yres;
    size_t stride;
    struct v4l2_format fmt;
    bool force_format = false;
    bool StatusCapturing=false;
};




