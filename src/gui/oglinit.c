#include <EGL/egl.h>
#include "eglstate.h"
#include <bcm_host.h>
#include <assert.h>

// setWindowParams sets the window's position, adjusting if need be to
// prevent it from going fully off screen. Also sets the dispman rects
// for displaying.
static void setWindowParams(STATE_T * state, int x, int y, VC_RECT_T * src_rect, VC_RECT_T * dst_rect) {
	uint32_t dx, dy, w, h, sx, sy;

	// Set source & destination rectangles so that the image is
	// clipped if it goes off screen (else dispman won't show it properly)
	if (x < (1 - (int)state->window_width)) {	   // Too far off left
		x = 1 - (int)state->window_width;
		dx = 0;
		sx = state->window_width - 1;
		w = 1;
	} else if (x < 0) {				   // Part of left is off
		dx = 0;
		sx = -x;
		w = state->window_width - sx;
	} else if (x < (state->screen_width - state->window_width)) {	// On
		dx = x;
		sx = 0;
		w = state->window_width;
	} else if (x < state->screen_width) {		   // Part of right is off
		dx = x;
		sx = 0;
		w = state->screen_width - x;
	} else {					   // Too far off right
		x = state->screen_width - 1;
		dx = state->screen_width - 1;
		sx = 0;
		w = 1;
	}

	if (y < (1 - (int)state->window_height)) {	   // Too far off top
		y = 1 - (int)state->window_height;
		dy = 0;
		sy = state->window_height - 1;
		h = 1;
	} else if (y < 0) {				   // Part of top is off
		dy = 0;
		sy = -y;
		h = state->window_height - sy;
	} else if (y < (state->screen_height - state->window_height)) {	// On
		dy = y;
		sy = 0;
		h = state->window_height;
	} else if (y < state->screen_height) {		   // Part of bottom is off
		dy = y;
		sy = 0;
		h = state->screen_height - y;
	} else {					   // Wholly off bottom
		y = state->screen_height - 1;
		dy = state->screen_height - 1;
		sy = 0;
		h = 1;
	}

	state->window_x = x;
	state->window_y = y;

	vc_dispmanx_rect_set(dst_rect, dx, dy, w, h);
	vc_dispmanx_rect_set(src_rect, sx << 16, sy << 16, w << 16, h << 16);
}

// oglinit sets the display, OpenVGL context and screen information
// state holds the display information
void oglinit(STATE_T * state) {
	int32_t success = 0;
	EGLBoolean result;
	EGLint num_config;

	static EGL_DISPMANX_WINDOW_T nativewindow;

	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
	static VC_DISPMANX_ALPHA_T alpha = {
		DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,
		255, 0
	};

	static const EGLint attribute_list[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	EGLConfig config;

	// get an EGL display connection
	state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(state->display != EGL_NO_DISPLAY);

	// initialize the EGL display connection
	result = eglInitialize(state->display, NULL, NULL);
	assert(EGL_FALSE != result);

	// bind OpenVG API
	eglBindAPI(EGL_OPENVG_API);

	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);

	// create an EGL rendering context
	state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, NULL);
	assert(state->context != EGL_NO_CONTEXT);

	// create an EGL window surface
	success = graphics_get_display_size(0 /* LCD */ , &state->screen_width,
					    &state->screen_height);
	assert(success >= 0);

	if ((state->window_width == 0) || (state->window_width > state->screen_width))
		state->window_width = state->screen_width;
	if ((state->window_height == 0) || (state->window_height > state->screen_height))
		state->window_height = state->screen_height;

	// adjust position so that at least one pixel is on screen and
	// set up the dispman rects
	setWindowParams(state, state->window_x, state->window_y, &src_rect, &dst_rect);

	dispman_display = vc_dispmanx_display_open(0 /* LCD */ );
	dispman_update = vc_dispmanx_update_start(0);

	dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display, 0 /*layer */ , &dst_rect, 0 /*src */ ,
						  &src_rect, DISPMANX_PROTECTION_NONE, &alpha, 0 /*clamp */ ,
						  0 /*transform */ );

	state->element = dispman_element;
	nativewindow.element = dispman_element;
	nativewindow.width = state->window_width;
	nativewindow.height = state->window_height;
	vc_dispmanx_update_submit_sync(dispman_update);

	state->surface = eglCreateWindowSurface(state->display, config, &nativewindow, NULL);
	assert(state->surface != EGL_NO_SURFACE);

	// preserve the buffers on swap
	result = eglSurfaceAttrib(state->display, state->surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED);
	assert(EGL_FALSE != result);

	// connect the context to the surface
	result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
	assert(EGL_FALSE != result);
}

// dispmanMoveWindow repositions the openVG window to given coords
// -ve coords are allowed upto (1-width,1-height),
// max (screen_width-1,screen_height-1). i.e. at least one pixel must be
// on the screen.
void dispmanMoveWindow(STATE_T * state, int x, int y) {
	VC_RECT_T src_rect, dst_rect;
	DISPMANX_UPDATE_HANDLE_T dispman_update;

	setWindowParams(state, x, y, &src_rect, &dst_rect);
	dispman_update = vc_dispmanx_update_start(0);
	vc_dispmanx_element_change_attributes(dispman_update, state->element, 0, 0, 0, &dst_rect, &src_rect, 0, DISPMANX_NO_ROTATE);
	vc_dispmanx_update_submit_sync(dispman_update);
}

// dispmanChangeWindowOpacity changes the window's opacity
// 0 = transparent, 255 = opaque
void dispmanChangeWindowOpacity(STATE_T * state, uint32_t alpha) {
	DISPMANX_UPDATE_HANDLE_T dispman_update;

	if (alpha > 255)
		alpha = 235;

	dispman_update = vc_dispmanx_update_start(0);
	// The 1<<1 below means update the alpha value
	vc_dispmanx_element_change_attributes(dispman_update, state->element, 1 << 1, 0, alpha, 0, 0, 0, DISPMANX_NO_ROTATE);
	vc_dispmanx_update_submit_sync(dispman_update);
}
