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
	xcb_screen_t *xcb_screen;
	xcb_window_t xcb_window;
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
	wc->xcb_screen = xcb_setup_roots_iterator(
			xcb_get_setup(wc->xcb_connection)).data;
	wc->xcb_window = xcb_generate_id(wc->xcb_connection);
	xcb_create_window(
			wc->xcb_connection,
			XCB_COPY_FROM_PARENT,
			wc->xcb_window,
			wc->xcb_screen->root,
			0, 0,			// x, y
			960, 512,		// width, height
			0,			// border width
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			wc->xcb_screen->root_visual,
			0, NULL
			);
	xcb_map_window(wc->xcb_connection, wc->xcb_window);
	xcb_flush(wc->xcb_connection);
	//TODO handle desktop scaling and remove window decorations


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
