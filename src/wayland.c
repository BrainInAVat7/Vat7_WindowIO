/* Wayland vat7_win_io layer */

// Needed for memfd_create
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <linux/input-event-codes.h>
#include <unistd.h>

#include <wayland-client.h>
#include "../wayland_client/xdg-shell-client-protocol.h"

#include "../include/vat7_window.h"
#include "../include/vat7_input.h"
#include "../include/vat7_render_target.h"

//TODO update these
#include "../../../common/logging/vat7_logger.h"
#include "../../../common/profiling/vat7_profiler.h"


// define not const because type varies. This should really be 2 or 3,
// but in any case never negative or greater than 8 or things will break.
#define NUMBER_OF_PIXEL_BUFFERS 3



/* Wayland Registry Listener Declarations */
static void vat7_registry_handle_global (void *data,
	struct wl_registry *registry, uint32_t name, const char *interface,
	uint32_t version);

static void vat7_registry_handle_global_remove (void *data,
	struct wl_registry *registry, uint32_t name);

static const struct wl_registry_listener vat7_registry_listener =
{
	.global = vat7_registry_handle_global,
	.global_remove = vat7_registry_handle_global_remove
};


/* Wayland Buffer Listener Declarations */
static void vat7_set_buffer_not_busy (void *data, struct wl_buffer *wlb);

static const struct wl_buffer_listener vat7_buffer_listener =
{
	.release = vat7_set_buffer_not_busy
};


/* XDG Listener Declarations */
static void vat7_xdg_wm_base_ping (void *data, struct xdg_wm_base *xdg_wm_base,
	uint32_t serial);

static void vat7_xdg_surface_configure (void *data,
	struct xdg_surface *xdg_surface, uint32_t serial);

static void vat7_xdg_toplevel_configure (void *data, struct xdg_toplevel *xdgtl,
	int32_t width, int32_t height, struct wl_array *states);

static void vat7_xdg_toplevel_close (void *data, struct xdg_toplevel *xdgtl);

static void vat7_xdg_toplevel_configure_bounds (void *data,
	struct xdg_toplevel *xdgtl, int32_t width, int32_t height);

static void vat7_xdg_toplevel_wm_capabilities (void *data,
	struct xdg_toplevel *xdgtl, struct wl_array *capabilities);

static const struct xdg_wm_base_listener vat7_xdg_wm_base_listener =
{
	.ping = vat7_xdg_wm_base_ping,
};

static const struct xdg_surface_listener vat7_xdg_surface_listener =
{
	.configure = vat7_xdg_surface_configure,
};

static const struct xdg_toplevel_listener vat7_xdg_toplevel_listener =
{
	.configure = vat7_xdg_toplevel_configure,
	.close = vat7_xdg_toplevel_close,
	.configure_bounds = vat7_xdg_toplevel_configure_bounds,
	.wm_capabilities = vat7_xdg_toplevel_wm_capabilities,
};


/* Wayland frame callback listener Declarations */
static void vat7_compositor_ready (void *data, struct wl_callback *cb,
	uint32_t time);

static const struct wl_callback_listener
vat7_frame_listener =
{
	.done = vat7_compositor_ready
};


/* Wayland seat listener declarations */
static void vat7_wl_seat_capabilities (void *data, struct wl_seat *seat,
	uint32_t caps);

static void vat7_wl_seat_name (void *data, struct wl_seat *seat,
	const char * name);

static void vat7_wl_keyboard_keymap (void *data, struct wl_keyboard *keyboard,
	uint32_t format, int32_t fd, uint32_t size);

static void vat7_wl_keyboard_enter (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, struct wl_surface *surface, struct wl_array *keys);

static void vat7_wl_keyboard_leave (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, struct wl_surface *surface);

static void vat7_wl_keyboard_key (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, uint32_t time, uint32_t key, uint32_t state);

static void vat7_wl_keyboard_modifiers (void *data,
	struct wl_keyboard *keyboard, uint32_t serial,
	uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
	uint32_t group);

static void vat7_wl_keyboard_repeat_info (void *data,
	struct wl_keyboard *keyboard, int32_t rate, int32_t delay);

static void vat7_wl_pointer_enter (void *data, struct wl_pointer *pointer,
	uint32_t serial, struct wl_surface *surface,
	wl_fixed_t surface_x, wl_fixed_t surface_y);

static void vat7_wl_pointer_leave (void *data, struct wl_pointer *pointer,
	uint32_t serial, struct wl_surface *surface);

static void vat7_wl_pointer_motion (void *data, struct wl_pointer *pointer,
	uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);

static void vat7_wl_pointer_button (void *data, struct wl_pointer *pointer,
	uint32_t serial, uint32_t time, uint32_t button, uint32_t state);

static void vat7_wl_pointer_frame (void *data, struct wl_pointer *pointer);

static void vat7_wl_pointer_axis (void *data, struct wl_pointer *pointer,
	uint32_t time, uint32_t axis, wl_fixed_t value);

static void vat7_wl_pointer_axis_source (void *data, struct wl_pointer *pointer,
	uint32_t axis_source);

static void vat7_wl_pointer_axis_stop (void *data, struct wl_pointer *pointer,
	uint32_t time, uint32_t axis);

static void vat7_wl_pointer_axis_discrete (void *data,
	struct wl_pointer *pointer, uint32_t axis, int32_t discrete);

static const struct wl_seat_listener vat7_wl_seat_listener =
{
	.capabilities = vat7_wl_seat_capabilities,
	.name = vat7_wl_seat_name,
};

static const struct wl_keyboard_listener vat7_wl_keyboard_listener =
{
	.keymap = vat7_wl_keyboard_keymap,
	.enter = vat7_wl_keyboard_enter,
	.leave = vat7_wl_keyboard_leave,
	.key = vat7_wl_keyboard_key,
	.modifiers = vat7_wl_keyboard_modifiers,
	.repeat_info = vat7_wl_keyboard_repeat_info,
};

static const struct wl_pointer_listener vat7_wl_pointer_listener =
{
	.enter = vat7_wl_pointer_enter,
	.leave = vat7_wl_pointer_leave,
	.motion = vat7_wl_pointer_motion,
	.button = vat7_wl_pointer_button,
	.frame = vat7_wl_pointer_frame,
	.axis = vat7_wl_pointer_axis,
	.axis_source = vat7_wl_pointer_axis_source,
	.axis_stop = vat7_wl_pointer_axis_stop,
	.axis_discrete = vat7_wl_pointer_axis_discrete,
};


/* Platform Context Definitions */
//TODO convert bools to bit flags?
struct vat7_WindowContext
{
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_surface *surface;
	struct wl_shm *shm;
	struct wl_shm_pool *shm_pool;
	struct wl_buffer *buffers[NUMBER_OF_PIXEL_BUFFERS];
	struct xdg_wm_base *xdg_wm_base;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	struct wl_callback *frame_callback;
	struct wl_seat *seat;
	struct wl_keyboard *keyboard;
	struct wl_pointer *pointer;
	void *main_mem_ptr;
	size_t main_mem_size;
	uint8_t *pixel_buffer;
	int32_t pb_height;
	int32_t pb_width;
	bool buffers_busy[NUMBER_OF_PIXEL_BUFFERS];
	size_t render_buffer;

	vat7_InputState input_context;
};


/* Helper Functions */

/* Check whether two strings are equal */
static bool strings_equal (const char *str_1, const char *str_2)
{
	while (*str_1 == *str_2)
	{
		// Note, only get to this check if values already
		// tested to be equal, so if str_1 terminates here,
		// then so does str_2.
		if (*str_1 == '\0') return true;
		str_1++;
		str_2++;
	}
	return false;
}

/* Convert a linux mouse button code for a mouse click event into an
 * engine constant.
 */
static uint8_t get_engine_mouse_button_value (uint32_t mouse_button)
{
	uint8_t eng_key;
	switch (mouse_button)
	{
		case BTN_LEFT:
			eng_key = vat7_MSBTN_LEFT;
			break;
		case BTN_RIGHT:
			eng_key = vat7_MSBTN_RIGHT;
			break;
		case BTN_MIDDLE:
			eng_key = vat7_MSBTN_MIDDLE;
			break;
		default:
			eng_key = vat7_NULL_KEY;
			break;
	}
	return eng_key;
}


/* Convert a Linux keycode for a keyboard event into an engine constant. */
static uint8_t get_engine_key_value (uint32_t wayland_key)
{
	uint8_t eng_key;
	switch (wayland_key)
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


/* Wayland Registry Listener Function Definitions */
// TODO cap version to avoid breaking on updates
static void vat7_registry_handle_global (void *data,
	struct wl_registry *registry, uint32_t name, const char *interface,
	uint32_t version)
{
	vat7_WindowContext *wc = data;
	if (strings_equal(interface, wl_compositor_interface.name))
	{
		wc->compositor = wl_registry_bind(registry, name,
			&wl_compositor_interface, version);
	}
	if (strings_equal(interface, wl_shm_interface.name))
	{
		wc->shm = wl_registry_bind(registry, name,
			&wl_shm_interface, version);
	}
	if (strings_equal(interface, xdg_wm_base_interface.name))
	{
		wc->xdg_wm_base = wl_registry_bind(registry, name,
			&xdg_wm_base_interface, version);
		xdg_wm_base_add_listener(wc->xdg_wm_base,
			&vat7_xdg_wm_base_listener, wc);
	}
	if (strings_equal(interface, wl_seat_interface.name))
	{
		wc->seat = wl_registry_bind(registry, name,
			&wl_seat_interface, version);
		wl_seat_add_listener(wc->seat, &vat7_wl_seat_listener, wc);
	}
}


static void vat7_registry_handle_global_remove (void *data,
	struct wl_registry *registry, uint32_t name)
{
	// Do Nothing
	(void)data;
	(void)registry;
	(void)name;
}


/* Wayland Buffer Listener Function Definitions */
static void vat7_set_buffer_not_busy (void *data, struct wl_buffer *wlb)
{
	vat7_WindowContext *wc = data;
	for (size_t i = 0; i < NUMBER_OF_PIXEL_BUFFERS; i++)
	{
		if (wc->buffers[i] == wlb)
		{
			wc->buffers_busy[i] = false;
			VT7_LOG_TRACE("Buffer %zu marked free", i);
			break;
		}
		if (i == NUMBER_OF_PIXEL_BUFFERS - 1)
		{

			VT7_LOG_WARNING("Wayland buffer released but no buffer \
				set not busy.");
		}
	}
}


/* XDG Listener Function Definitions */
static void vat7_xdg_wm_base_ping (void *data, struct xdg_wm_base *xdg_wm_base,
	uint32_t serial)
{
	(void)data;
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static void vat7_xdg_surface_configure (void *data,
	struct xdg_surface *xdg_surface, uint32_t serial)
{
	vat7_WindowContext *wc = data;
	xdg_surface_ack_configure(xdg_surface, serial);
	if (wc->frame_callback != NULL)
	{
		wl_callback_destroy(wc->frame_callback);
	}
	wc->frame_callback = wl_surface_frame(wc->surface);
	wl_callback_add_listener(wc->frame_callback, &vat7_frame_listener, wc);
	wl_surface_commit(wc->surface);
}

static void vat7_xdg_toplevel_configure (void *data, struct xdg_toplevel *xdgtl,
	int32_t width, int32_t height, struct wl_array *states)
{
	// Do nothing
	(void)data;
	(void)xdgtl;
	(void)width;
	(void)height;
	(void)states;
}

static void vat7_xdg_toplevel_close (void *data, struct xdg_toplevel *xdgtl)
{
	// Do nothing
	(void)xdgtl;
	vat7_WindowContext *wc = data;
	size_t bc = wc->input_context.buffer_count;
	vat7_InputEvent ie;
	ie.type = vat7_QUIT;
	ie.key = vat7_NULL_KEY;
	wc->input_context.buffer[bc] = ie;
	wc->input_context.buffer_count++;
}


static void vat7_xdg_toplevel_configure_bounds (void *data,
	struct xdg_toplevel *xdgtl, int32_t width, int32_t height)
{
	// Do nothing
	(void)data;
	(void)xdgtl;
	(void)width;
	(void)height;
}


static void vat7_xdg_toplevel_wm_capabilities (void *data,
	struct xdg_toplevel *xdgtl, struct wl_array *capabilities)
{
	(void)data;
	(void)xdgtl;
	(void)capabilities;
}


/* Wayland Frame Callback Function Definitions */
static void vat7_compositor_ready (void *data, struct wl_callback *cb,
	uint32_t time)
{
	(void)time;
	VT7_LOG_TRACE("Callback time: %d", time);
	vat7_WindowContext *wc = data;
	wl_callback_destroy(cb);
	wc->frame_callback = NULL;
}


/* Wayland Seat Listener Function Definitions */
static void vat7_wl_seat_capabilities (void *data, struct wl_seat *seat,
	uint32_t capabilities)
{
	vat7_WindowContext *wc = data;
	if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
	{
		wc->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(wc->keyboard,
			&vat7_wl_keyboard_listener, wc);
	}
	else
	{
		wc->keyboard = NULL;
	}
	if (capabilities & WL_SEAT_CAPABILITY_POINTER)
	{
		wc->pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(wc->pointer,
			&vat7_wl_pointer_listener, wc);
	}
	else
	{
		wc->pointer = NULL;
	}
}


static void vat7_wl_seat_name (void *data, struct wl_seat *seat,
	const char * name)
{
	// do nothing
	(void)data;
	(void)seat;
	(void)name;
}


static void vat7_wl_keyboard_keymap (void *data, struct wl_keyboard *keyboard,
	uint32_t format, int32_t fd, uint32_t size)
{
	(void)data;
	(void)keyboard;
	(void)format;
	(void)fd;
	(void)size;
}


static void vat7_wl_keyboard_enter (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)surface;
	(void)keys;
}


static void vat7_wl_keyboard_leave (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, struct wl_surface *surface)
{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)surface;
}


static void vat7_wl_keyboard_key (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	(void)keyboard;
	(void)serial;
	(void)time;
	vat7_WindowContext *wc = data;

	vat7_InputEvent ie;
	// Note: state = 1 is press, state = 0 is released.
	if (state == 1) ie.type = vat7_KEY_DOWN;
	else if (state == 0) ie.type = vat7_KEY_UP;
	ie.key = get_engine_key_value(key);
	size_t bc = wc->input_context.buffer_count;
	wc->input_context.buffer[bc] = ie;
	wc->input_context.buffer_count++;
}


static void vat7_wl_keyboard_modifiers (void *data,
	struct wl_keyboard *keyboard, uint32_t serial,
	uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
	uint32_t group)
{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)mods_depressed;
	(void)mods_latched;
	(void)mods_locked;
	(void)group;
}


static void vat7_wl_keyboard_repeat_info (void *data,
	struct wl_keyboard *keyboard, int32_t rate, int32_t delay)
{
	(void)data;
	(void)keyboard;
	(void)rate;
	(void)delay;
}


static void vat7_wl_pointer_enter (void *data, struct wl_pointer *pointer,
	uint32_t serial, struct wl_surface *surface,
	wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	(void)data;
	(void)pointer;
	(void)serial;
	(void)surface;
	(void)surface_x;
	(void)surface_y;
}


static void vat7_wl_pointer_leave (void *data, struct wl_pointer *pointer,
	uint32_t serial, struct wl_surface *surface)
{
	(void)data;
	(void)pointer;
	(void)serial;
	(void)surface;
}


static void vat7_wl_pointer_motion (void *data, struct wl_pointer *pointer,
	uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	(void)pointer;
	(void)time;
	vat7_WindowContext *wc = data;

	// Can use this if I ever want to support subpixel precision.
	// double x = wl_fixed_to_double(surface_x);
	// double y = wl_fixed_to_double(surface_y);

	int x = wl_fixed_to_int(surface_x);
	int y = wl_fixed_to_int(surface_y);

	wc->input_context.mouse_x = (uint16_t)x;
	wc->input_context.mouse_y = (uint16_t)y;
}


static void vat7_wl_pointer_button (void *data, struct wl_pointer *pointer,
	uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	(void)pointer;
	(void)serial;
	(void)time;
	vat7_WindowContext *wc = data;

	vat7_InputEvent ie;
	// Note: state = 1 is press, state = 0 is released.
	if (state == 1) ie.type = vat7_MOUSE_DOWN;
	else if (state == 0) ie.type = vat7_MOUSE_UP;
	ie.key = get_engine_mouse_button_value(button);
	size_t bc = wc->input_context.buffer_count;
	wc->input_context.buffer[bc] = ie;
	wc->input_context.buffer_count++;
}


static void vat7_wl_pointer_frame (void *data, struct wl_pointer *pointer)
{
	(void)data;
	(void)pointer;
}


static void vat7_wl_pointer_axis (void *data, struct wl_pointer *pointer,
	uint32_t time, uint32_t axis, wl_fixed_t value)
{
	(void)data;
	(void)pointer;
	(void)time;
	(void)axis;
	(void)value;
}


static void vat7_wl_pointer_axis_source (void *data, struct wl_pointer *pointer,
	uint32_t axis_source)
{
	(void)data;
	(void)pointer;
	(void)axis_source;
}


static void vat7_wl_pointer_axis_stop (void *data, struct wl_pointer *pointer,
	uint32_t time, uint32_t axis)
{
	(void)data;
	(void)pointer;
	(void)time;
	(void)axis;
}


static void vat7_wl_pointer_axis_discrete (void *data,
	struct wl_pointer *pointer, uint32_t axis, int32_t discrete)
{
	(void)data;
	(void)pointer;
	(void)axis;
	(void)discrete;
}



/* Helper Functions */
static void vat7_pump_wayland_events (vat7_WindowContext *wc)
{
	while (wl_display_prepare_read(wc->display) != 0)
		wl_display_dispatch_pending(wc->display);

	wl_display_flush(wc->display);

	wl_display_read_events(wc->display);
	wl_display_dispatch_pending(wc->display);
}


/* API Functions */
void vat7_win_io_quit(vat7_WindowContext *wc)
{
	// Need in case shutdown happens while a callback is active
	if (wc->frame_callback != NULL)
	{
		wl_callback_destroy(wc->frame_callback);
		wc->frame_callback = NULL;
	}

	if (wc->keyboard != NULL)
	{
		wl_keyboard_destroy(wc->keyboard);
		wc->keyboard = NULL;
	}
	if (wc->pointer != NULL)
	{
		wl_pointer_destroy(wc->pointer);
		wc->pointer = NULL;
	}
	if (wc->seat != NULL)
	{
		wl_seat_destroy(wc->seat);
		wc->seat = NULL;
	}
	if (wc->xdg_toplevel != NULL)
	{
		xdg_toplevel_destroy(wc->xdg_toplevel);
		wc->xdg_toplevel = NULL;
	}
	if (wc->xdg_surface != NULL)
	{
		xdg_surface_destroy(wc->xdg_surface);
		wc->xdg_surface = NULL;
	}
	if (wc->xdg_wm_base != NULL)
	{
		xdg_wm_base_destroy(wc->xdg_wm_base);
		wc->xdg_surface = NULL;
	}
	for (size_t i = 0;
		i < sizeof(wc->buffers) / sizeof(wc->buffers[0]); i++)
	{
		if (wc->buffers[i] != NULL)
		{
			wl_buffer_destroy(wc->buffers[i]);
			wc->buffers[i] = NULL;
		}
	}
	if (wc->shm_pool != NULL)
	{
		wl_shm_pool_destroy(wc->shm_pool);
		wc->shm_pool = NULL;
	}
	if (wc->shm != NULL)
	{
		wl_shm_destroy(wc->shm);
		wc->shm = NULL;
	}
	if (wc->surface != NULL)
	{
		wl_surface_destroy(wc->surface);
		wc->surface = NULL;
	}
	if (wc->compositor != NULL)
	{
		wl_compositor_destroy(wc->compositor);
		wc->compositor = NULL;
	}
	if (wc->registry != NULL)
	{
		wl_registry_destroy(wc->registry);
		wc->registry = NULL;
	}
	if (wc->display != NULL)
	{
		wl_display_disconnect(wc->display);
		wc->display = NULL;
	}

	munmap(wc->main_mem_ptr, wc->main_mem_size);
	free(wc);
	VT7_LOG_INFO("Wayland display disconnected. Platform quit successful.");
}


vat7_WindowContext *vat7_win_io_init (void)
{
	// WINDOW CONTEXT
	vat7_WindowContext *wc =
		(vat7_WindowContext *)malloc(sizeof(vat7_WindowContext));

	// Pixel Buffer
	int32_t width = 960;
	wc->pb_width = width;
	int32_t height = 512;
	wc->pb_height = height;
	int32_t pixel_bytes = 4;
	int32_t stride = width * pixel_bytes;
	int32_t pixel_buffer_size = width * height * pixel_bytes;
	size_t buffers_mem =
		(size_t)(pixel_buffer_size * NUMBER_OF_PIXEL_BUFFERS);

	// MEMORY
	// TODO Add error checking for all of this
	// TODO Implement new allocator for this since not using the
	// one from the old project
	int fd = memfd_create("pg_mem_block", MFD_CLOEXEC);
	//TODO clean this up. For now just patching in to get new
	//architecture hooked up for proof of concept
	//In particular, rename!
	size_t main_mem_bytes = buffers_mem;
	ftruncate(fd, (off_t)main_mem_bytes);
	void *mem = mmap(NULL, main_mem_bytes,
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);


	// for freeing on quit
	wc->main_mem_ptr = mem;
	wc->main_mem_size = main_mem_bytes;

	// INPUT
	vat7_InputState ic;
	ic.buffer_count = 0u;
	ic.mouse_x = 0u;
	ic.mouse_y = 0u;
	wc->input_context = ic;

	// WAYLAND
	wc->display = wl_display_connect(NULL);
	if (!(wc->display))
	{
		VT7_LOG_CRITICAL("Wayland display failed to connect.");
		return NULL;
	}
	VT7_LOG_INFO("Wayland display connected.");
	// TODO error handling on get registry
	wc->registry = wl_display_get_registry(wc->display);
	wl_registry_add_listener(wc->registry, &vat7_registry_listener, wc);
	wl_display_roundtrip(wc->display);
	wc->surface = wl_compositor_create_surface(wc->compositor);
	assert(main_mem_bytes <= INT32_MAX);
	wc->shm_pool = wl_shm_create_pool(wc->shm, fd,
		(int32_t)main_mem_bytes);

	// TODO make buffer plural for clarity
	wc->pixel_buffer = (uint8_t *)mem;
	assert(wc->pixel_buffer != NULL);
	for (int32_t i = 0; i < NUMBER_OF_PIXEL_BUFFERS; i++)
	{
		wc->buffers[i] = wl_shm_pool_create_buffer(wc->shm_pool,
			i * (int32_t)pixel_buffer_size,
			width, height, stride, WL_SHM_FORMAT_XRGB8888);
		wc->buffers_busy[i] = false;
		wl_buffer_add_listener(wc->buffers[i], &vat7_buffer_listener,
			wc);
	}
	wc->render_buffer = 0u;

	// XDG
	wc->xdg_surface = xdg_wm_base_get_xdg_surface(wc->xdg_wm_base,
		wc->surface);
	xdg_surface_add_listener(wc->xdg_surface, &vat7_xdg_surface_listener,
		wc);
	wc->xdg_toplevel = xdg_surface_get_toplevel(wc->xdg_surface);
	xdg_toplevel_add_listener(wc->xdg_toplevel, &vat7_xdg_toplevel_listener,
		wc);
	xdg_toplevel_set_title(wc->xdg_toplevel, "Wayland Test Window");

	close(fd);

	return wc;
}


vat7_InputState *vat7_win_io_get_input_state (vat7_WindowContext *wc)
{
	vat7_pump_wayland_events(wc);
	return &(wc->input_context);
}


vat7_RenderTarget vat7_win_io_get_render_target (vat7_WindowContext *wc)
{
	for (size_t i = 0; i < NUMBER_OF_PIXEL_BUFFERS; i++)
	{
		if (!wc->buffers_busy[i])
		{
			wc->render_buffer = i;
			VT7_LOG_TRACE("Render Buffer set to %zu", \
				wc->render_buffer);
			wc->buffers_busy[wc->render_buffer] = true;
			size_t pb_size =
				(size_t)(wc->pb_width * 4 * wc->pb_height);
			vat7_RenderTarget target;
			target.pixel_buffer = wc->pixel_buffer
				+ pb_size * wc->render_buffer;
			target.width = (uint32_t)wc->pb_width;
			target.height = (uint32_t)wc->pb_height;
			target.pixel_bytes = 4u;
			target.flags = 0u;
			return target;
		}
	}
	// TODO carefully log this NULL return behavior
	VT7_LOG_WARNING("Returning NULL render target");
	return vat7_NULL_RENDER_TARGET;
}


void vat7_win_io_present (vat7_WindowContext *wc)
{
	// TODO if vsync, vat7_win_io_wait until compositor ready for buffer
	static int cm = 0;

	vat7_pump_wayland_events(wc);

	if (wc->frame_callback == NULL)
	{
		cm++;
		VT7_PROFILE_BEGIN(Wayland_Commit);
		wl_surface_attach(wc->surface, wc->buffers[wc->render_buffer],
			0, 0);
		wl_surface_damage(wc->surface, 0, 0, wc->pb_width,
			wc->pb_height);
		wc->frame_callback = wl_surface_frame(wc->surface);
		wl_callback_add_listener(wc->frame_callback,
			&vat7_frame_listener, wc);
		wl_surface_commit(wc->surface);
		wl_display_flush(wc->display);
		VT7_LOG_TRACE("Presented Buffer %zu", wc->render_buffer);
		VT7_LOG_TRACE("Commit Number: %d", cm);
		VT7_PROFILE_END();
	}
	else
	{
		// Free the buffer that wasn't committed to the
		// compositor since it never took ownership.
		wc->buffers_busy[wc->render_buffer] = false;
		VT7_LOG_TRACE("Buffer %zu marked free", wc->render_buffer);
	}

}
