/* Placeholder for x11 window_io version */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <linux/input-event-codes.h>

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
	xcb_atom_t wm_protocols_atom;
	xcb_atom_t wm_delete_window_atom;
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


/* Helper Functions */


/* Convert xcb mouse button codes for a mouse click event into an
 * engine constant.
 */
static uint8_t get_engine_mouse_button_value (uint32_t mouse_button)
{
	uint8_t eng_key;
	switch (mouse_button)
	{
		case XCB_BUTTON_INDEX_1:
			eng_key = vat7_MSBTN_LEFT;
			break;
		case XCB_BUTTON_INDEX_2:
			eng_key = vat7_MSBTN_MIDDLE;
			break;
		case XCB_BUTTON_INDEX_3:
			eng_key = vat7_MSBTN_RIGHT;
			break;
		default:
			eng_key = vat7_NULL_KEY;
			break;
	}
	return eng_key;
}


/* Convert linux keycodes for a keyboard event into an engine constant. */
static uint8_t get_engine_key_value (uint8_t xcb_key)
{
	uint8_t eng_key;
	// X11 offsets keycodes because 0 to 7 are reserved.
	switch (xcb_key - 8)
	{
		case KEY_ENTER:
			eng_key = vat7_K_ENTER;
			break;
		case KEY_ESC:
			eng_key = vat7_K_ESC;
			break;
		case KEY_BACKSPACE:
			eng_key = vat7_K_BSPACE;
			break;
		case KEY_TAB:
			eng_key = vat7_K_TAB;
			break;
		case KEY_SPACE:
			eng_key = vat7_K_SPACE;
			break;
		case KEY_EQUAL:
			eng_key = vat7_K_EQUAL;
			break;
		case KEY_COMMA:
			eng_key = vat7_K_COMMA;
			break;
		case KEY_MINUS:
			eng_key = vat7_K_HYPHEN;
			break;
		case KEY_DOT:
			eng_key = vat7_K_PERIOD;
			break;
		case KEY_SLASH:
			eng_key = vat7_K_FSLASH;
			break;
		case KEY_LEFTBRACE:
			eng_key = vat7_K_LBRACK;
			break;
		case KEY_RIGHTBRACE:
			eng_key = vat7_K_RBRACK;
			break;
		case KEY_SEMICOLON:
			eng_key = vat7_K_SEMCLN;
			break;
		case KEY_BACKSLASH:
			eng_key = vat7_K_BSLASH;
			break;
		case KEY_GRAVE:
			eng_key = vat7_K_GRAVE;
			break;
		case KEY_APOSTROPHE:
			eng_key = vat7_K_APOSTR;
			break;
		case KEY_LEFTCTRL:
			eng_key = vat7_K_LCTRL;
			break;
		case KEY_LEFTSHIFT:
			eng_key = vat7_K_LSHIFT;
			break;
		case KEY_LEFTALT:
			eng_key = vat7_K_LALT;
			break;
		case KEY_RIGHTSHIFT:
			eng_key = vat7_K_RSHIFT;
			break;
		case KEY_RIGHTCTRL:
			eng_key = vat7_K_RCTRL;
			break;
		case KEY_RIGHTALT:
			eng_key = vat7_K_RALT;
			break;
		case KEY_CAPSLOCK:
			eng_key = vat7_K_CAPSLK;
			break;
		case KEY_RIGHT:
			eng_key = vat7_K_RIGHT;
			break;
		case KEY_DOWN:
			eng_key = vat7_K_DOWN;
			break;
		case KEY_LEFT:
			eng_key = vat7_K_LEFT;
			break;
		case KEY_UP:
			eng_key = vat7_K_UP;
			break;
		case KEY_HOME:
			eng_key = vat7_K_HOME;
			break;
		case KEY_INSERT:
			eng_key = vat7_K_INSERT;
			break;
		case KEY_END:
			eng_key = vat7_K_END;
			break;
		case KEY_DELETE:
			eng_key = vat7_K_DELETE;
			break;
		case KEY_PAGEUP:
			eng_key = vat7_K_PGUP;
			break;
		case KEY_PAGEDOWN:
			eng_key = vat7_K_PGDOWN;
			break;
		case KEY_F1:
			eng_key = vat7_K_F1;
			break;
		case KEY_F2:
			eng_key = vat7_K_F2;
			break;
		case KEY_F3:
			eng_key = vat7_K_F3;
			break;
		case KEY_F4:
			eng_key = vat7_K_F4;
			break;
		case KEY_F5:
			eng_key = vat7_K_F5;
			break;
		case KEY_F6:
			eng_key = vat7_K_F6;
			break;
		case KEY_F7:
			eng_key = vat7_K_F7;
			break;
		case KEY_F8:
			eng_key = vat7_K_F8;
			break;
		case KEY_F9:
			eng_key = vat7_K_F9;
			break;
		case KEY_F10:
			eng_key = vat7_K_F10;
			break;
		case KEY_F11:
			eng_key = vat7_K_F11;
			break;
		case KEY_F12:
			eng_key = vat7_K_F12;
			break;
		case KEY_0:
			eng_key = vat7_K_0;
			break;
		case KEY_1:
			eng_key = vat7_K_1;
			break;
		case KEY_2:
			eng_key = vat7_K_2;
			break;
		case KEY_3:
			eng_key = vat7_K_3;
			break;
		case KEY_4:
			eng_key = vat7_K_4;
			break;
		case KEY_5:
			eng_key = vat7_K_5;
			break;
		case KEY_6:
			eng_key = vat7_K_6;
			break;
		case KEY_7:
			eng_key = vat7_K_7;
			break;
		case KEY_8:
			eng_key = vat7_K_8;
			break;
		case KEY_9:
			eng_key = vat7_K_9;
			break;
		case KEY_A:
			eng_key = vat7_K_a;
			break;
		case KEY_B:
			eng_key = vat7_K_b;
			break;
		case KEY_C:
			eng_key = vat7_K_c;
			break;
		case KEY_D:
			eng_key = vat7_K_d;
			break;
		case KEY_E:
			eng_key = vat7_K_e;
			break;
		case KEY_F:
			eng_key = vat7_K_f;
			break;
		case KEY_G:
			eng_key = vat7_K_g;
			break;
		case KEY_H:
			eng_key = vat7_K_h;
			break;
		case KEY_I:
			eng_key = vat7_K_i;
			break;
		case KEY_J:
			eng_key = vat7_K_j;
			break;
		case KEY_K:
			eng_key = vat7_K_k;
			break;
		case KEY_L:
			eng_key = vat7_K_l;
			break;
		case KEY_M:
			eng_key = vat7_K_m;
			break;
		case KEY_N:
			eng_key = vat7_K_n;
			break;
		case KEY_O:
			eng_key = vat7_K_o;
			break;
		case KEY_P:
			eng_key = vat7_K_p;
			break;
		case KEY_Q:
			eng_key = vat7_K_q;
			break;
		case KEY_R:
			eng_key = vat7_K_r;
			break;
		case KEY_S:
			eng_key = vat7_K_s;
			break;
		case KEY_T:
			eng_key = vat7_K_t;
			break;
		case KEY_U:
			eng_key = vat7_K_u;
			break;
		case KEY_V:
			eng_key = vat7_K_v;
			break;
		case KEY_W:
			eng_key = vat7_K_w;
			break;
		case KEY_X:
			eng_key = vat7_K_x;
			break;
		case KEY_Y:
			eng_key = vat7_K_y;
			break;
		case KEY_Z:
			eng_key = vat7_K_z;
			break;
		case KEY_KPSLASH:
			eng_key = vat7_K_NP_DIV;
			break;
		case KEY_KPASTERISK:
			eng_key = vat7_K_NP_MUL;
			break;
		case KEY_KPMINUS:
			eng_key = vat7_K_NP_MIN;
			break;
		case KEY_KPPLUS:
			eng_key = vat7_K_NP_PLS;
			break;
		case KEY_KPENTER:
			eng_key = vat7_K_NP_ENT;
			break;
		case KEY_KPDOT:
			eng_key = vat7_K_NP_PER;
			break;
		case KEY_KP0:
			eng_key = vat7_K_NP_0;
			break;
		case KEY_KP1:
			eng_key = vat7_K_NP_1;
			break;
		case KEY_KP2:
			eng_key = vat7_K_NP_2;
			break;
		case KEY_KP3:
			eng_key = vat7_K_NP_3;
			break;
		case KEY_KP4:
			eng_key = vat7_K_NP_4;
			break;
		case KEY_KP5:
			eng_key = vat7_K_NP_5;
			break;
		case KEY_KP6:
			eng_key = vat7_K_NP_6;
			break;
		case KEY_KP7:
			eng_key = vat7_K_NP_7;
			break;
		case KEY_KP8:
			eng_key = vat7_K_NP_8;
			break;
		case KEY_KP9:
			eng_key = vat7_K_NP_9;
			break;
		default:
			eng_key = vat7_NULL_KEY;
			break;
	}
	return eng_key;
}


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

	/* Create Window */
	//TODO add error checking and logging
	wc->xcb_connection = xcb_connect(NULL, NULL);
	wc->xcb_screen = xcb_setup_roots_iterator(
			xcb_get_setup(wc->xcb_connection)).data;
	wc->xcb_window = xcb_generate_id(wc->xcb_connection);
	uint32_t mask = XCB_CW_EVENT_MASK;
	uint32_t values[1];
	values[0] = XCB_EVENT_MASK_EXPOSURE
		| XCB_EVENT_MASK_KEY_PRESS
		| XCB_EVENT_MASK_KEY_RELEASE
		| XCB_EVENT_MASK_BUTTON_PRESS
		| XCB_EVENT_MASK_BUTTON_RELEASE
		| XCB_EVENT_MASK_POINTER_MOTION;
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

	/* Register for close window events (WTF X11?) */
	xcb_intern_atom_cookie_t xcb_cookie_2 =
		xcb_intern_atom(wc->xcb_connection, 0, 12, "WM_PROTOCOLS");
	/* This call allocates and reply must be freed */
	xcb_intern_atom_reply_t *reply_2 =
		xcb_intern_atom_reply(wc->xcb_connection, xcb_cookie_2, NULL);
	wc->wm_protocols_atom = reply_2->atom;
	free(reply_2);
	xcb_intern_atom_cookie_t xcb_cookie_3 =
		xcb_intern_atom(wc->xcb_connection, 0, 16, "WM_DELETE_WINDOW");
	/* This call allocates and reply must be freed */
	xcb_intern_atom_reply_t *reply_3 =
		xcb_intern_atom_reply(wc->xcb_connection, xcb_cookie_3, NULL);
	wc->wm_delete_window_atom = reply_3->atom;
	free(reply_3);
	xcb_change_property(wc->xcb_connection,
			XCB_PROP_MODE_REPLACE,
			wc->xcb_window,
			wc->wm_protocols_atom,
			XCB_ATOM_ATOM,
			32,
			1,
			&(wc->wm_delete_window_atom));

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
		vat7_InputEvent ie;
		switch ((e->response_type & ~0x80))
		{
			case XCB_CLIENT_MESSAGE:
			{
				xcb_client_message_event_t *cm =
					(xcb_client_message_event_t *)e;
				if (cm->type == wc->wm_protocols_atom
					&& cm->format == 32
					&& cm->data.data32[0]
						== wc->wm_delete_window_atom)
				{
					ie.type = vat7_QUIT;
					ie.key = vat7_NULL_KEY;
				}
				break;
			}
			case XCB_KEY_PRESS:
			{
				xcb_key_press_event_t *ke =
					(xcb_key_press_event_t *)e;
				ie.type = vat7_KEY_DOWN;
				ie.key = get_engine_key_value(ke->detail);
				break;
			}
			case XCB_KEY_RELEASE:
			{
				xcb_key_release_event_t *ke =
					(xcb_key_release_event_t *)e;
				ie.type = vat7_KEY_UP;
				ie.key = get_engine_key_value(ke->detail);
				break;
			}
			case XCB_BUTTON_PRESS:
			{
				xcb_button_press_event_t *be =
					(xcb_button_press_event_t *)e;
				ie.type = vat7_MOUSE_DOWN;
				ie.key = get_engine_mouse_button_value(
								be->detail);
				break;
			}
			case XCB_BUTTON_RELEASE:
			{
				xcb_button_release_event_t *be =
					(xcb_button_release_event_t *)e;
				ie.type = vat7_MOUSE_UP;
				ie.key = get_engine_mouse_button_value(
								be->detail);
				break;
			}
			case XCB_MOTION_NOTIFY:
			{
				//TODO mouse coordinates
				xcb_motion_notify_event_t *me;
				me = (xcb_motion_notify_event_t *)e;
				wc->input_state.mouse_x = (uint16_t)me->event_x;
				wc->input_state.mouse_y = (uint16_t)me->event_y;
				break;
			}
			default:
			{
				ie.type = vat7_NULL_INPUT;
				ie.key = vat7_NULL_KEY;
				break;
			}
		}
		uint16_t bc = wc->input_state.buffer_count++;
		wc->input_state.buffer[bc] = ie;
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
