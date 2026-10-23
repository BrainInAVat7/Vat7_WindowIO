/* Placeholder for x11 window_io version */

#include <stddef.h>
#include <inttypes.h>
#include <stdlib.h>

#include <xcb/xcb.h>

#include "../include/vat7_window.h"
#include "../include/vat7_input.h"
#include "../include/vat7_render_target.h"

#include "logging/vat7_logger.h"


//temporary placholder
struct vat7_WindowContext
{
	xcb_connection_t *xcb_connection;
	vat7_InputState input_state;
};

/* API Functions */
void vat7_win_io_quit(vat7_WindowContext *wc)
{
	xcb_disconnect(wc->xcb_connection);
	free(wc);
}


vat7_WindowContext *vat7_win_io_init (void)
{
	vat7_WindowContext *wc =
		(vat7_WindowContext *)malloc(sizeof(vat7_WindowContext));

	wc->input_state.buffer_count = 0u;
	wc->input_state.mouse_x = 0u;
	wc->input_state.mouse_y = 0u;

	/* XCB */
	//TODO add error checking and logging
	wc->xcb_connection = xcb_connect(NULL, NULL);

	xcb_screen_t *screen =
		xcb_setup_roots_iterator(xcb_get_setup(wc->xcb_connection)).data;

	VT7_LOG_INFO("Screen width = %"PRIu16"\n", screen->width_in_pixels);
	VT7_LOG_INFO("Screen height = %"PRIu16"\n", screen->height_in_pixels);

	return wc;
}


vat7_InputState *vat7_win_io_get_input_state (vat7_WindowContext *wc)
{
	return &(wc->input_state);
}


vat7_RenderTarget vat7_win_io_get_render_target (vat7_WindowContext *wc)
{
	(void)wc;
	return vat7_NULL_RENDER_TARGET;
}


void vat7_win_io_present (vat7_WindowContext *wc)
{
	(void)wc;
}
