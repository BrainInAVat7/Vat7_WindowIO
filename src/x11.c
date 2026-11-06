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
	xcb_gcontext_t xcb_graphics_context;
	vat7_InputState input_state;
	uint32_t pixel_buffer_len;
	uint8_t *pixel_buffer;
	uint8_t pixel_size;
	uint16_t width;
	uint16_t height;
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
	free(wc->pixel_buffer);
	free(wc);
}


vat7_WindowContext *vat7_win_io_init (void)
{
	vat7_WindowContext *wc =
		(vat7_WindowContext *)malloc(sizeof(vat7_WindowContext));
	if (wc == NULL) return NULL;

	/* Input State Initialization */
	wc->input_state.buffer_count = 0u;
	wc->input_state.mouse_x = 0u;
	wc->input_state.mouse_y = 0u;

	/* Create Pixel Buffer */
	// TODO use mmap here instead
	wc->width = 960;
	wc->height = 512;
	wc->pixel_size = 4;
	uint32_t pixel_buffer_len = (uint32_t)wc->width
				* (uint32_t)wc->height
				* (uint32_t)wc->pixel_size;
	wc->pixel_buffer =
		(uint8_t *)malloc((size_t)pixel_buffer_len * sizeof(uint8_t));
	if (wc->pixel_buffer == NULL) return NULL;
	wc->pixel_buffer_len = pixel_buffer_len;
	for (size_t i = 0; i < pixel_buffer_len; i++)
	{
		wc->pixel_buffer[i] = 0;
	}

	/* XCB */
	//TODO add error checking and logging
	wc->xcb_connection = xcb_connect(NULL, NULL);
	wc->xcb_screen = xcb_setup_roots_iterator(
			xcb_get_setup(wc->xcb_connection)).data;
	wc->xcb_window = xcb_generate_id(wc->xcb_connection);
	uint32_t mask = XCB_CW_EVENT_MASK;
	uint32_t values[1];
	values[0] = XCB_EVENT_MASK_EXPOSURE
		| XCB_EVENT_MASK_KEY_PRESS;
	xcb_create_window(wc->xcb_connection,
			XCB_COPY_FROM_PARENT,
			wc->xcb_window,
			wc->xcb_screen->root,
			0, 0,			// x, y on screen
			(uint16_t)wc->width, (uint16_t)wc->height,
			0,			// border width
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			wc->xcb_screen->root_visual,
			mask,
			values);
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
	/* Create XCB graphics Context */
	wc->xcb_graphics_context = xcb_generate_id(wc->xcb_connection);
	uint32_t xcb_gc_mask = XCB_GC_FOREGROUND;
	uint32_t xcb_gc_value_list[1];
	xcb_gc_value_list[0] = wc->xcb_screen->black_pixel;
	xcb_create_gc(wc->xcb_connection,
			wc->xcb_graphics_context,
			wc->xcb_window,
			xcb_gc_mask,
			xcb_gc_value_list);

	xcb_map_window(wc->xcb_connection, wc->xcb_window);
	xcb_flush(wc->xcb_connection);

	xcb_generic_event_t *e;
	while((e = xcb_wait_for_event(wc->xcb_connection)))
	{
		if ((e->response_type & ~0x80) == XCB_EXPOSE)
		{
			xcb_put_image(wc->xcb_connection,
					XCB_IMAGE_FORMAT_Z_PIXMAP,
					wc->xcb_window,
					wc->xcb_graphics_context,
					wc->width,
					wc->height,
					0,
					0,
					0,
					wc->xcb_screen->root_depth,
					wc->pixel_buffer_len * sizeof(uint8_t),
					wc->pixel_buffer);
			xcb_flush(wc->xcb_connection);
			break;
		}
	}
	//TODO handle desktop scaling

	return wc;
}


vat7_InputState *vat7_win_io_get_input_state (vat7_WindowContext *wc)
{
	xcb_generic_event_t *e;
	while((e = xcb_poll_for_event(wc->xcb_connection)))
	{
		if (e == NULL) break;
		switch ((e->response_type & ~0x80))
		{
			case XCB_KEY_PRESS:
				{
					vat7_InputEvent ie;
					ie.type = vat7_QUIT;
					ie.key = vat7_NULL_KEY;
					uint16_t bc =
						wc->input_state.buffer_count++;
					wc->input_state.buffer[bc] = ie;
					break;
				}
			default: break;
		}
	}
	free(e);
	return &(wc->input_state);
}


vat7_RenderTarget vat7_win_io_get_render_target (vat7_WindowContext *wc)
{
	vat7_RenderTarget rt;
	rt.width = wc->width;
	rt.height = wc->height;
	rt.pixel_bytes = wc->pixel_size;
	rt.pixel_buffer = wc->pixel_buffer;
	rt.flags = 0;
	return rt;
}


void vat7_win_io_present (vat7_WindowContext *wc)
{
	xcb_put_image(wc->xcb_connection,
			XCB_IMAGE_FORMAT_Z_PIXMAP,
			wc->xcb_window,
			wc->xcb_graphics_context,
			wc->width,
			wc->height,
			0,
			0,
			0,
			wc->xcb_screen->root_depth,
			wc->pixel_buffer_len * sizeof(uint8_t),
			wc->pixel_buffer);
	xcb_flush(wc->xcb_connection);
}
