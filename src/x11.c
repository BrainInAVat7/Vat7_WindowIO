/* Placeholder for x11 window_io version */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <xcb/xcb.h>

#include "../include/vat7_window.h"
#include "../include/vat7_input.h"
#include "../include/vat7_render_target.h"

#include "logging/vat7_logger.h"


/* Window Context */
struct vat7_WindowContext
{
	xcb_connection_t *xcb_connection;
	xcb_screen_t *xcb_screen;
	xcb_window_t xcb_window;
	vat7_InputState input_state;
};


/* X11 data structures */

/* MOTIF hints for x11. This is a mess of historical baggage that took
 * some research to sort out. It's all stupid boilerplate, but it's
 * necessary to control window attributes through the window manager
 * on linux desktop environments.
 *
 * Basically need to declare this manually to avoid importing decades old
 * headers from a windows manager no longer in use but which defined
 * data structures used by modern windows managers.
 */
typedef struct
{
	uint32_t flags;
	uint32_t functions;
	uint32_t decorations;
	int32_t input_mode;
	uint32_t status;
} MotifWindowManagerHints;


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
	/* Disable Window Decorations */
	/* 15 in 3rd parameter is strlen("_MOTIF_WM_HINTS"), but I'm not
	 * including string.h just for that */
	xcb_intern_atom_cookie_t xcb_cookie =
		xcb_intern_atom(wc->xcb_connection, 0, 15, "_MOTIF_WM_HINTS");
	/* This call allocates and reply must be freed */
	xcb_intern_atom_reply_t *reply =
		xcb_intern_atom_reply(wc->xcb_connection, xcb_cookie, NULL);
	MotifWindowManagerHints motif_hints;
	/* flags tells windows manager which fields in the motif hints struct
	 * are active. 1LL << 1 (i.e., 2) tells it to use the decorations
	 * field. I want to disable decorations to make behavior in
	 * parity with wayland version, which does not implement
	 * desktop window decorations (like the "X" to close).
	 */
	motif_hints.flags = 2;
	motif_hints.functions = 0;
	/* decorations = 0 disables all decorations */
	motif_hints.decorations = 0;
	motif_hints.input_mode = 0;
	motif_hints.status = 0;
	xcb_change_property(wc->xcb_connection,
			XCB_PROP_MODE_REPLACE,
			wc->xcb_window,
			reply->atom,
			reply->atom,
			32, //length of data in motif_hints struct
			5, //number of fields in motif_hints struct,
			&motif_hints);
	free(reply);

	xcb_map_window(wc->xcb_connection, wc->xcb_window);
	xcb_flush(wc->xcb_connection);
	//TODO handle desktop scaling, and make window position consistent
	//accross machines


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
