typedef struct {
	// Screen dimentions
	uint32_t screen_width;
	uint32_t screen_height;
	// Window dimentions
	int32_t window_x;
	int32_t window_y;
	uint32_t window_width;
	uint32_t window_height;
	// dispman window 
	DISPMANX_ELEMENT_HANDLE_T element;

	// EGL data
	EGLDisplay display;

	EGLSurface surface;
	EGLContext context;
} STATE_T;

extern void oglinit(STATE_T *);
extern void dispmanMoveWindow(STATE_T *, int, int);
extern void dispmanChangeWindowOpacity(STATE_T *, unsigned int);
