/* Wayland platform layer */

// Needed for memfd_create
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "engine_libc/eng_stdbool.h"
#include "engine_libc/eng_stddef.h"
#include "engine_libc/eng_stdint.h"

#include <sys/mman.h>
#include <linux/input-event-codes.h>

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include "platform.h"
#include "audio_linux.h"
#include "render_target.h"
#include "audio_buffer.h"

#include "engine_core/eng_input.h"

#include "engine_tools/eng_memory.h"
#include "engine_tools/eng_assert.h"
#include "engine_tools/eng_logging.h"
#include "engine_tools/eng_profiling.h"


// define not const because type varies. This should really be 2 or 3,
// but in any case never negative or greater than 8 or things will break.
#define NUMBER_OF_PIXEL_BUFFERS 3


/* Manual declarations so unistd.h not required */
// Note: these are defined in linux's libc.
// Removing all libc dependency would require
// architecture specific definitions and assembly
typedef long off_t;

int close(int fd);

int ftruncate(int fd, off_t length);


/* Wayland Registry Listener Declarations */
static void ENG_registry_handle_global (void *data,
	struct wl_registry *registry, uint32_t name, const char *interface,
	uint32_t version);

static void ENG_registry_handle_global_remove (void *data,
	struct wl_registry *registry, uint32_t name);

static const struct wl_registry_listener ENG_registry_listener =
{
	.global = ENG_registry_handle_global,
	.global_remove = ENG_registry_handle_global_remove
};


/* Wayland Buffer Listener Declarations */
static void ENG_set_buffer_not_busy (void *data, struct wl_buffer *wlb);

static const struct wl_buffer_listener ENG_buffer_listener =
{
	.release = ENG_set_buffer_not_busy
};


/* XDG Listener Declarations */
static void ENG_xdg_wm_base_ping (void *data, struct xdg_wm_base *xdg_wm_base,
	uint32_t serial);

static void ENG_xdg_surface_configure (void *data,
	struct xdg_surface *xdg_surface, uint32_t serial);

static void ENG_xdg_toplevel_configure (void *data, struct xdg_toplevel *xdgtl,
	int32_t width, int32_t height, struct wl_array *states);

static void ENG_xdg_toplevel_close (void *data, struct xdg_toplevel *xdgtl);

static void ENG_xdg_toplevel_configure_bounds (void *data,
	struct xdg_toplevel *xdgtl, int32_t width, int32_t height);

static void ENG_xdg_toplevel_wm_capabilities (void *data,
	struct xdg_toplevel *xdgtl, struct wl_array *capabilities);

static const struct xdg_wm_base_listener ENG_xdg_wm_base_listener =
{
	.ping = ENG_xdg_wm_base_ping,
};

static const struct xdg_surface_listener ENG_xdg_surface_listener =
{
	.configure = ENG_xdg_surface_configure,
};

static const struct xdg_toplevel_listener ENG_xdg_toplevel_listener =
{
	.configure = ENG_xdg_toplevel_configure,
	.close = ENG_xdg_toplevel_close,
	.configure_bounds = ENG_xdg_toplevel_configure_bounds,
	.wm_capabilities = ENG_xdg_toplevel_wm_capabilities,
};


/* Wayland frame callback listener Declarations */
static void ENG_compositor_ready (void *data, struct wl_callback *cb,
	uint32_t time);

static const struct wl_callback_listener
ENG_frame_listener =
{
	.done = ENG_compositor_ready
};


/* Wayland seat listener declarations */
static void ENG_wl_seat_capabilities (void *data, struct wl_seat *seat,
	uint32_t caps);

static void ENG_wl_seat_name (void *data, struct wl_seat *seat,
	const char * name);

static void ENG_wl_keyboard_keymap (void *data, struct wl_keyboard *keyboard,
	uint32_t format, int32_t fd, uint32_t size);

static void ENG_wl_keyboard_enter (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, struct wl_surface *surface, struct wl_array *keys);

static void ENG_wl_keyboard_leave (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, struct wl_surface *surface);

static void ENG_wl_keyboard_key (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, uint32_t time, uint32_t key, uint32_t state);

static void ENG_wl_keyboard_modifiers (void *data,
	struct wl_keyboard *keyboard, uint32_t serial,
	uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
	uint32_t group);

static void ENG_wl_keyboard_repeat_info (void *data,
	struct wl_keyboard *keyboard, int32_t rate, int32_t delay);

static void ENG_wl_pointer_enter (void *data, struct wl_pointer *pointer,
	uint32_t serial, struct wl_surface *surface,
	wl_fixed_t surface_x, wl_fixed_t surface_y);

static void ENG_wl_pointer_leave (void *data, struct wl_pointer *pointer,
	uint32_t serial, struct wl_surface *surface);

static void ENG_wl_pointer_motion (void *data, struct wl_pointer *pointer,
	uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);

static void ENG_wl_pointer_button (void *data, struct wl_pointer *pointer,
	uint32_t serial, uint32_t time, uint32_t button, uint32_t state);

static void ENG_wl_pointer_frame (void *data, struct wl_pointer *pointer);

static void ENG_wl_pointer_axis (void *data, struct wl_pointer *pointer,
	uint32_t time, uint32_t axis, wl_fixed_t value);

static void ENG_wl_pointer_axis_source (void *data, struct wl_pointer *pointer,
	uint32_t axis_source);

static void ENG_wl_pointer_axis_stop (void *data, struct wl_pointer *pointer,
	uint32_t time, uint32_t axis);

static void ENG_wl_pointer_axis_discrete (void *data,
	struct wl_pointer *pointer, uint32_t axis, int32_t discrete);

static const struct wl_seat_listener ENG_wl_seat_listener =
{
	.capabilities = ENG_wl_seat_capabilities,
	.name = ENG_wl_seat_name,
};

static const struct wl_keyboard_listener ENG_wl_keyboard_listener =
{
	.keymap = ENG_wl_keyboard_keymap,
	.enter = ENG_wl_keyboard_enter,
	.leave = ENG_wl_keyboard_leave,
	.key = ENG_wl_keyboard_key,
	.modifiers = ENG_wl_keyboard_modifiers,
	.repeat_info = ENG_wl_keyboard_repeat_info,
};

static const struct wl_pointer_listener ENG_wl_pointer_listener =
{
	.enter = ENG_wl_pointer_enter,
	.leave = ENG_wl_pointer_leave,
	.motion = ENG_wl_pointer_motion,
	.button = ENG_wl_pointer_button,
	.frame = ENG_wl_pointer_frame,
	.axis = ENG_wl_pointer_axis,
	.axis_source = ENG_wl_pointer_axis_source,
	.axis_stop = ENG_wl_pointer_axis_stop,
	.axis_discrete = ENG_wl_pointer_axis_discrete,
};


/* Platform Context Definitions */
//TODO convert bools to bit flags
struct ENG_PlatformContext
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

	LinuxAudioContext *audio_context;
	ENG_InputContext input_context;
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
			eng_key = ENG_MSBTN_LEFT;
			break;
		case BTN_RIGHT:
			eng_key = ENG_MSBTN_RIGHT;
			break;
		case BTN_MIDDLE:
			eng_key = ENG_MSBTN_MIDDLE;
			break;
		default:
			eng_key = ENG_NULL_KEY;
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
			eng_key = ENG_K_ENTER;
			break;
		case KEY_ESC:
			eng_key = ENG_K_ESC;
			break;
		case KEY_BACKSPACE:
			eng_key = ENG_K_BSPACE;
			break;
		case KEY_TAB:
			eng_key = ENG_K_TAB;
			break;
		case KEY_SPACE:
			eng_key = ENG_K_SPACE;
			break;
		case KEY_EQUAL:
			eng_key = ENG_K_EQUAL;
			break;
		case KEY_COMMA:
			eng_key = ENG_K_COMMA;
			break;
		case KEY_MINUS:
			eng_key = ENG_K_HYPHEN;
			break;
		case KEY_DOT:
			eng_key = ENG_K_PERIOD;
			break;
		case KEY_SLASH:
			eng_key = ENG_K_FSLASH;
			break;
		case KEY_LEFTBRACE:
			eng_key = ENG_K_LBRACK;
			break;
		case KEY_RIGHTBRACE:
			eng_key = ENG_K_RBRACK;
			break;
		case KEY_SEMICOLON:
			eng_key = ENG_K_SEMCLN;
			break;
		case KEY_BACKSLASH:
			eng_key = ENG_K_BSLASH;
			break;
		case KEY_GRAVE:
			eng_key = ENG_K_GRAVE;
			break;
		case KEY_APOSTROPHE:
			eng_key = ENG_K_APOSTR;
			break;
		case KEY_LEFTCTRL:
			eng_key = ENG_K_LCTRL;
			break;
		case KEY_LEFTSHIFT:
			eng_key = ENG_K_LSHIFT;
			break;
		case KEY_LEFTALT:
			eng_key = ENG_K_LALT;
			break;
		case KEY_RIGHTSHIFT:
			eng_key = ENG_K_RSHIFT;
			break;
		case KEY_RIGHTCTRL:
			eng_key = ENG_K_RCTRL;
			break;
		case KEY_RIGHTALT:
			eng_key = ENG_K_RALT;
			break;
		case KEY_CAPSLOCK:
			eng_key = ENG_K_CAPSLK;
			break;
		case KEY_RIGHT:
			eng_key = ENG_K_RIGHT;
			break;
		case KEY_DOWN:
			eng_key = ENG_K_DOWN;
			break;
		case KEY_LEFT:
			eng_key = ENG_K_LEFT;
			break;
		case KEY_UP:
			eng_key = ENG_K_UP;
			break;
		case KEY_HOME:
			eng_key = ENG_K_HOME;
			break;
		case KEY_INSERT:
			eng_key = ENG_K_INSERT;
			break;
		case KEY_END:
			eng_key = ENG_K_END;
			break;
		case KEY_DELETE:
			eng_key = ENG_K_DELETE;
			break;
		case KEY_PAGEUP:
			eng_key = ENG_K_PGUP;
			break;
		case KEY_PAGEDOWN:
			eng_key = ENG_K_PGDOWN;
			break;
		case KEY_F1:
			eng_key = ENG_K_F1;
			break;
		case KEY_F2:
			eng_key = ENG_K_F2;
			break;
		case KEY_F3:
			eng_key = ENG_K_F3;
			break;
		case KEY_F4:
			eng_key = ENG_K_F4;
			break;
		case KEY_F5:
			eng_key = ENG_K_F5;
			break;
		case KEY_F6:
			eng_key = ENG_K_F6;
			break;
		case KEY_F7:
			eng_key = ENG_K_F7;
			break;
		case KEY_F8:
			eng_key = ENG_K_F8;
			break;
		case KEY_F9:
			eng_key = ENG_K_F9;
			break;
		case KEY_F10:
			eng_key = ENG_K_F10;
			break;
		case KEY_F11:
			eng_key = ENG_K_F11;
			break;
		case KEY_F12:
			eng_key = ENG_K_F12;
			break;
		case KEY_0:
			eng_key = ENG_K_0;
			break;
		case KEY_1:
			eng_key = ENG_K_1;
			break;
		case KEY_2:
			eng_key = ENG_K_2;
			break;
		case KEY_3:
			eng_key = ENG_K_3;
			break;
		case KEY_4:
			eng_key = ENG_K_4;
			break;
		case KEY_5:
			eng_key = ENG_K_5;
			break;
		case KEY_6:
			eng_key = ENG_K_6;
			break;
		case KEY_7:
			eng_key = ENG_K_7;
			break;
		case KEY_8:
			eng_key = ENG_K_8;
			break;
		case KEY_9:
			eng_key = ENG_K_9;
			break;
		case KEY_A:
			eng_key = ENG_K_a;
			break;
		case KEY_B:
			eng_key = ENG_K_b;
			break;
		case KEY_C:
			eng_key = ENG_K_c;
			break;
		case KEY_D:
			eng_key = ENG_K_d;
			break;
		case KEY_E:
			eng_key = ENG_K_e;
			break;
		case KEY_F:
			eng_key = ENG_K_f;
			break;
		case KEY_G:
			eng_key = ENG_K_g;
			break;
		case KEY_H:
			eng_key = ENG_K_h;
			break;
		case KEY_I:
			eng_key = ENG_K_i;
			break;
		case KEY_J:
			eng_key = ENG_K_j;
			break;
		case KEY_K:
			eng_key = ENG_K_k;
			break;
		case KEY_L:
			eng_key = ENG_K_l;
			break;
		case KEY_M:
			eng_key = ENG_K_m;
			break;
		case KEY_N:
			eng_key = ENG_K_n;
			break;
		case KEY_O:
			eng_key = ENG_K_o;
			break;
		case KEY_P:
			eng_key = ENG_K_p;
			break;
		case KEY_Q:
			eng_key = ENG_K_q;
			break;
		case KEY_R:
			eng_key = ENG_K_r;
			break;
		case KEY_S:
			eng_key = ENG_K_s;
			break;
		case KEY_T:
			eng_key = ENG_K_t;
			break;
		case KEY_U:
			eng_key = ENG_K_u;
			break;
		case KEY_V:
			eng_key = ENG_K_v;
			break;
		case KEY_W:
			eng_key = ENG_K_w;
			break;
		case KEY_X:
			eng_key = ENG_K_x;
			break;
		case KEY_Y:
			eng_key = ENG_K_y;
			break;
		case KEY_Z:
			eng_key = ENG_K_z;
			break;
		case KEY_KPSLASH:
			eng_key = ENG_K_NP_DIV;
			break;
		case KEY_KPASTERISK:
			eng_key = ENG_K_NP_MUL;
			break;
		case KEY_KPMINUS:
			eng_key = ENG_K_NP_MIN;
			break;
		case KEY_KPPLUS:
			eng_key = ENG_K_NP_PLS;
			break;
		case KEY_KPENTER:
			eng_key = ENG_K_NP_ENT;
			break;
		case KEY_KPDOT:
			eng_key = ENG_K_NP_PER;
			break;
		case KEY_KP0:
			eng_key = ENG_K_NP_0;
			break;
		case KEY_KP1:
			eng_key = ENG_K_NP_1;
			break;
		case KEY_KP2:
			eng_key = ENG_K_NP_2;
			break;
		case KEY_KP3:
			eng_key = ENG_K_NP_3;
			break;
		case KEY_KP4:
			eng_key = ENG_K_NP_4;
			break;
		case KEY_KP5:
			eng_key = ENG_K_NP_5;
			break;
		case KEY_KP6:
			eng_key = ENG_K_NP_6;
			break;
		case KEY_KP7:
			eng_key = ENG_K_NP_7;
			break;
		case KEY_KP8:
			eng_key = ENG_K_NP_8;
			break;
		case KEY_KP9:
			eng_key = ENG_K_NP_9;
			break;
		default:
			eng_key = ENG_NULL_KEY;
			break;
	}
	return eng_key;
}


/* Wayland Registry Listener Function Definitions */
// TODO cap version to avoid breaking on updates
static void ENG_registry_handle_global (void *data,
	struct wl_registry *registry, uint32_t name, const char *interface,
	uint32_t version)
{
	ENG_PlatformContext *pc = data;
	if (strings_equal(interface, wl_compositor_interface.name))
	{
		pc->compositor = wl_registry_bind(registry, name,
			&wl_compositor_interface, version);
	}
	if (strings_equal(interface, wl_shm_interface.name))
	{
		pc->shm = wl_registry_bind(registry, name,
			&wl_shm_interface, version);
	}
	if (strings_equal(interface, xdg_wm_base_interface.name))
	{
		pc->xdg_wm_base = wl_registry_bind(registry, name,
			&xdg_wm_base_interface, version);
		xdg_wm_base_add_listener(pc->xdg_wm_base,
			&ENG_xdg_wm_base_listener, pc);
	}
	if (strings_equal(interface, wl_seat_interface.name))
	{
		pc->seat = wl_registry_bind(registry, name,
			&wl_seat_interface, version);
		wl_seat_add_listener(pc->seat, &ENG_wl_seat_listener, pc);
	}
}


static void ENG_registry_handle_global_remove (void *data,
	struct wl_registry *registry, uint32_t name)
{
	// Do Nothing
	(void)data;
	(void)registry;
	(void)name;
}


/* Wayland Buffer Listener Function Definitions */
static void ENG_set_buffer_not_busy (void *data, struct wl_buffer *wlb)
{
	ENG_PlatformContext *pc = data;
	for (size_t i = 0; i < NUMBER_OF_PIXEL_BUFFERS; i++)
	{
		if (pc->buffers[i] == wlb)
		{
			pc->buffers_busy[i] = false;
			TRACE_LOG("Buffer %zu marked free", i);
			break;
		}
		if (i == NUMBER_OF_PIXEL_BUFFERS - 1)
		{

			WARNING_LOG("Wayland buffer released but no buffer \
				set not busy.");
		}
	}
}


/* XDG Listener Function Definitions */
static void ENG_xdg_wm_base_ping (void *data, struct xdg_wm_base *xdg_wm_base,
	uint32_t serial)
{
	(void)data;
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static void ENG_xdg_surface_configure (void *data,
	struct xdg_surface *xdg_surface, uint32_t serial)
{
	ENG_PlatformContext *pc = data;
	xdg_surface_ack_configure(xdg_surface, serial);
	if (pc->frame_callback != NULL)
	{
		wl_callback_destroy(pc->frame_callback);
	}
	pc->frame_callback = wl_surface_frame(pc->surface);
	wl_callback_add_listener(pc->frame_callback, &ENG_frame_listener, pc);
	wl_surface_commit(pc->surface);
}

static void ENG_xdg_toplevel_configure (void *data, struct xdg_toplevel *xdgtl,
	int32_t width, int32_t height, struct wl_array *states)
{
	// Do nothing
	(void)data;
	(void)xdgtl;
	(void)width;
	(void)height;
	(void)states;
}

static void ENG_xdg_toplevel_close (void *data, struct xdg_toplevel *xdgtl)
{
	// Do nothing
	(void)xdgtl;
	ENG_PlatformContext *pc = data;
	size_t bc = pc->input_context.buffer_count;
	ENG_InputEvent ie;
	ie.type = ENG_QUIT;
	ie.key = ENG_NULL_KEY;
	pc->input_context.buffer[bc] = ie;
	pc->input_context.buffer_count++;
}


static void ENG_xdg_toplevel_configure_bounds (void *data,
	struct xdg_toplevel *xdgtl, int32_t width, int32_t height)
{
	// Do nothing
	(void)data;
	(void)xdgtl;
	(void)width;
	(void)height;
}


static void ENG_xdg_toplevel_wm_capabilities (void *data,
	struct xdg_toplevel *xdgtl, struct wl_array *capabilities)
{
	(void)data;
	(void)xdgtl;
	(void)capabilities;
}


/* Wayland Frame Callback Function Definitions */
static void ENG_compositor_ready (void *data, struct wl_callback *cb,
	uint32_t time)
{
	(void)time;
	TRACE_LOG("Callback time: %d", time);
	ENG_PlatformContext *pc = data;
	wl_callback_destroy(cb);
	pc->frame_callback = NULL;
}


/* Wayland Seat Listener Function Definitions */
static void ENG_wl_seat_capabilities (void *data, struct wl_seat *seat,
	uint32_t capabilities)
{
	ENG_PlatformContext *pc = data;
	if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
	{
		pc->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(pc->keyboard,
			&ENG_wl_keyboard_listener, pc);
	}
	else
	{
		pc->keyboard = NULL;
	}
	if (capabilities & WL_SEAT_CAPABILITY_POINTER)
	{
		pc->pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(pc->pointer,
			&ENG_wl_pointer_listener, pc);
	}
	else
	{
		pc->pointer = NULL;
	}
}


static void ENG_wl_seat_name (void *data, struct wl_seat *seat,
	const char * name)
{
	// do nothing
	(void)data;
	(void)seat;
	(void)name;
}


static void ENG_wl_keyboard_keymap (void *data, struct wl_keyboard *keyboard,
	uint32_t format, int32_t fd, uint32_t size)
{
	(void)data;
	(void)keyboard;
	(void)format;
	(void)fd;
	(void)size;
}


static void ENG_wl_keyboard_enter (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)surface;
	(void)keys;
}


static void ENG_wl_keyboard_leave (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, struct wl_surface *surface)
{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)surface;
}


static void ENG_wl_keyboard_key (void *data, struct wl_keyboard *keyboard,
	uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	(void)keyboard;
	(void)serial;
	(void)time;
	ENG_PlatformContext *pc = data;

	ENG_InputEvent ie;
	// Note: state = 1 is press, state = 0 is released.
	if (state == 1) ie.type = ENG_KEY_DOWN;
	else if (state == 0) ie.type = ENG_KEY_UP;
	ie.key = get_engine_key_value(key);
	size_t bc = pc->input_context.buffer_count;
	pc->input_context.buffer[bc] = ie;
	pc->input_context.buffer_count++;
}


static void ENG_wl_keyboard_modifiers (void *data,
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


static void ENG_wl_keyboard_repeat_info (void *data,
	struct wl_keyboard *keyboard, int32_t rate, int32_t delay)
{
	(void)data;
	(void)keyboard;
	(void)rate;
	(void)delay;
}


static void ENG_wl_pointer_enter (void *data, struct wl_pointer *pointer,
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


static void ENG_wl_pointer_leave (void *data, struct wl_pointer *pointer,
	uint32_t serial, struct wl_surface *surface)
{
	(void)data;
	(void)pointer;
	(void)serial;
	(void)surface;
}


static void ENG_wl_pointer_motion (void *data, struct wl_pointer *pointer,
	uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	(void)pointer;
	(void)time;
	ENG_PlatformContext *pc = data;

	// Can use this if I ever want to support subpixel precision.
	// double x = wl_fixed_to_double(surface_x);
	// double y = wl_fixed_to_double(surface_y);

	int x = wl_fixed_to_int(surface_x);
	int y = wl_fixed_to_int(surface_y);

	pc->input_context.mouse_x = (uint16_t)x;
	pc->input_context.mouse_y = (uint16_t)y;
}


static void ENG_wl_pointer_button (void *data, struct wl_pointer *pointer,
	uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	(void)pointer;
	(void)serial;
	(void)time;
	ENG_PlatformContext *pc = data;

	ENG_InputEvent ie;
	// Note: state = 1 is press, state = 0 is released.
	if (state == 1) ie.type = ENG_MOUSE_DOWN;
	else if (state == 0) ie.type = ENG_MOUSE_UP;
	ie.key = get_engine_mouse_button_value(button);
	size_t bc = pc->input_context.buffer_count;
	pc->input_context.buffer[bc] = ie;
	pc->input_context.buffer_count++;
}


static void ENG_wl_pointer_frame (void *data, struct wl_pointer *pointer)
{
	(void)data;
	(void)pointer;
}


static void ENG_wl_pointer_axis (void *data, struct wl_pointer *pointer,
	uint32_t time, uint32_t axis, wl_fixed_t value)
{
	(void)data;
	(void)pointer;
	(void)time;
	(void)axis;
	(void)value;
}


static void ENG_wl_pointer_axis_source (void *data, struct wl_pointer *pointer,
	uint32_t axis_source)
{
	(void)data;
	(void)pointer;
	(void)axis_source;
}


static void ENG_wl_pointer_axis_stop (void *data, struct wl_pointer *pointer,
	uint32_t time, uint32_t axis)
{
	(void)data;
	(void)pointer;
	(void)time;
	(void)axis;
}


static void ENG_wl_pointer_axis_discrete (void *data,
	struct wl_pointer *pointer, uint32_t axis, int32_t discrete)
{
	(void)data;
	(void)pointer;
	(void)axis;
	(void)discrete;
}



/* Helper Functions */
static void ENG_pump_wayland_events (ENG_PlatformContext *pc)
{
	while (wl_display_prepare_read(pc->display) != 0)
		wl_display_dispatch_pending(pc->display);

	wl_display_flush(pc->display);

	wl_display_read_events(pc->display);
	wl_display_dispatch_pending(pc->display);
}


/* API Functions */
void platform_quit(ENG_PlatformContext *pc)
{
	if (pc->audio_context != NULL)
	{
		platform_shutdown_linux_audio(pc->audio_context);
		pc->audio_context = NULL;
	}

	// Need in case shutdown happens while a callback is active
	if (pc->frame_callback != NULL)
	{
		wl_callback_destroy(pc->frame_callback);
		pc->frame_callback = NULL;
	}

	if (pc->keyboard != NULL)
	{
		wl_keyboard_destroy(pc->keyboard);
		pc->keyboard = NULL;
	}
	if (pc->pointer != NULL)
	{
		wl_pointer_destroy(pc->pointer);
		pc->pointer = NULL;
	}
	if (pc->seat != NULL)
	{
		wl_seat_destroy(pc->seat);
		pc->seat = NULL;
	}
	if (pc->xdg_toplevel != NULL)
	{
		xdg_toplevel_destroy(pc->xdg_toplevel);
		pc->xdg_toplevel = NULL;
	}
	if (pc->xdg_surface != NULL)
	{
		xdg_surface_destroy(pc->xdg_surface);
		pc->xdg_surface = NULL;
	}
	if (pc->xdg_wm_base != NULL)
	{
		xdg_wm_base_destroy(pc->xdg_wm_base);
		pc->xdg_surface = NULL;
	}
	for (size_t i = 0;
		i < sizeof(pc->buffers) / sizeof(pc->buffers[0]); i++)
	{
		if (pc->buffers[i] != NULL)
		{
			wl_buffer_destroy(pc->buffers[i]);
			pc->buffers[i] = NULL;
		}
	}
	if (pc->shm_pool != NULL)
	{
		wl_shm_pool_destroy(pc->shm_pool);
		pc->shm_pool = NULL;
	}
	if (pc->shm != NULL)
	{
		wl_shm_destroy(pc->shm);
		pc->shm = NULL;
	}
	if (pc->surface != NULL)
	{
		wl_surface_destroy(pc->surface);
		pc->surface = NULL;
	}
	if (pc->compositor != NULL)
	{
		wl_compositor_destroy(pc->compositor);
		pc->compositor = NULL;
	}
	if (pc->registry != NULL)
	{
		wl_registry_destroy(pc->registry);
		pc->registry = NULL;
	}
	if (pc->display != NULL)
	{
		wl_display_disconnect(pc->display);
		pc->display = NULL;
	}

	ENG_shutdown_main_memory();
	munmap(pc->main_mem_ptr, pc->main_mem_size);
	INFO_LOG("Wayland display disconnected. Platform quit successful.");
}


ENG_PlatformContext *platform_initialize (memory_init_fn init_memory,
	timing_init_fn init_timing)
{
	// Initialize Timing
	init_timing(platform_get_timing_counter,
		platform_get_timing_frequency);

	// MEMORY
	// TODO Add error checking for all of this
	int fd = memfd_create("pg_mem_block", MFD_CLOEXEC);
	size_t main_mem_bytes = 536870912u;
	ftruncate(fd, (off_t)main_mem_bytes);
	void *mem = mmap(NULL, main_mem_bytes,
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	init_memory(mem, main_mem_bytes);

	// PLATFORM CONTEXT
	ENG_PlatformContext *pc =
		(ENG_PlatformContext *)ENG_alloc(sizeof(ENG_PlatformContext));
	// for freeing on quit
	pc->main_mem_ptr = mem;
	pc->main_mem_size = main_mem_bytes;

	// INPUT
	ENG_InputContext ic;
	ic.buffer_count = 0u;
	ic.mouse_x = 0u;
	ic.mouse_y = 0u;
	pc->input_context = ic;

	// WAYLAND
	pc->display = wl_display_connect(NULL);
	if (!(pc->display))
	{
		CRITICAL_LOG("Wayland display failed to connect.");
		return NULL;
	}
	INFO_LOG("Wayland display connected.");
	// TODO error handling on get registry
	pc->registry = wl_display_get_registry(pc->display);
	wl_registry_add_listener(pc->registry, &ENG_registry_listener, pc);
	wl_display_roundtrip(pc->display);
	pc->surface = wl_compositor_create_surface(pc->compositor);
	ENG_ASSERT(main_mem_bytes <= INT32_MAX);
	pc->shm_pool = wl_shm_create_pool(pc->shm, fd,
		(int32_t)main_mem_bytes);

	// Pixel Buffer
	int32_t width = 960;
	pc->pb_width = width;
	int32_t height = 512;
	pc->pb_height = height;
	int32_t pixel_bytes = 4;
	int32_t stride = width * pixel_bytes;
	int32_t pixel_buffer_size = width * height * pixel_bytes;
	size_t buffers_mem =
		(size_t)(pixel_buffer_size * NUMBER_OF_PIXEL_BUFFERS);
	// TODO make buffer plural for clarity
	pc->pixel_buffer = (uint8_t *)ENG_alloc(buffers_mem);
	ENG_ASSERT(pc->pixel_buffer != NULL);
	if ((uintptr_t)(pc->pixel_buffer) < (uintptr_t)mem)
	{
		CRITICAL_LOG("pixel buffer pointer less than mem pointer");
		platform_quit(pc);
		return NULL;
	}
	uintptr_t offset = (uintptr_t)pc->pixel_buffer - (uintptr_t)mem;
	if (offset > INT32_MAX)
	{
		CRITICAL_LOG("Wayland buffer offset exceeds max size");
		platform_quit(pc);
		return NULL;
	}
	for (int32_t i = 0; i < NUMBER_OF_PIXEL_BUFFERS; i++)
	{
		pc->buffers[i] = wl_shm_pool_create_buffer(pc->shm_pool,
			(int32_t)offset + i * (int32_t)pixel_buffer_size,
			width, height, stride, WL_SHM_FORMAT_XRGB8888);
		pc->buffers_busy[i] = false;
		wl_buffer_add_listener(pc->buffers[i], &ENG_buffer_listener,
			pc);
	}
	pc->render_buffer = 0u;

	// XDG (This step especially is black magic)
	pc->xdg_surface = xdg_wm_base_get_xdg_surface(pc->xdg_wm_base,
		pc->surface);
	xdg_surface_add_listener(pc->xdg_surface, &ENG_xdg_surface_listener,
		pc);
	pc->xdg_toplevel = xdg_surface_get_toplevel(pc->xdg_surface);
	xdg_toplevel_add_listener(pc->xdg_toplevel, &ENG_xdg_toplevel_listener,
		pc);
	xdg_toplevel_set_title(pc->xdg_toplevel, "Wayland Test Window");

	close(fd);

	// Audio
	pc->audio_context = platform_init_linux_audio();

	return pc;
}


ENG_InputContext *platform_get_input (ENG_PlatformContext *pc)
{
	ENG_pump_wayland_events(pc);
	return &(pc->input_context);
}


ENG_RenderTarget platform_get_render_target (ENG_PlatformContext *pc)
{
	for (size_t i = 0; i < NUMBER_OF_PIXEL_BUFFERS; i++)
	{
		if (!pc->buffers_busy[i])
		{
			pc->render_buffer = i;
			TRACE_LOG("Render Buffer set to %zu", \
				pc->render_buffer);
			pc->buffers_busy[pc->render_buffer] = true;
			size_t pb_size =
				(size_t)(pc->pb_width * 4 * pc->pb_height);
			ENG_RenderTarget target;
			target.pixel_buffer = pc->pixel_buffer
				+ pb_size * pc->render_buffer;
			target.width = (uint32_t)pc->pb_width;
			target.height = (uint32_t)pc->pb_height;
			target.pixel_bytes = 4u;
			target.flags = 0u;
			return target;
		}
	}
	// TODO carefully log this NULL return behavior
	WARNING_LOG("Returning NULL render target");
	return ENG_NULL_RENDER_TARGET;
}


void platform_present (ENG_PlatformContext *pc)
{
	// TODO if vsync, platform_wait until compositor ready for buffer
	static int cm = 0;

	ENG_pump_wayland_events(pc);

	if (pc->frame_callback == NULL)
	{
		cm++;
		PROFILE_BEGIN(Wayland_Commit);
		wl_surface_attach(pc->surface, pc->buffers[pc->render_buffer],
			0, 0);
		wl_surface_damage(pc->surface, 0, 0, pc->pb_width,
			pc->pb_height);
		pc->frame_callback = wl_surface_frame(pc->surface);
		wl_callback_add_listener(pc->frame_callback,
			&ENG_frame_listener, pc);
		wl_surface_commit(pc->surface);
		wl_display_flush(pc->display);
		TRACE_LOG("Presented Buffer %zu", pc->render_buffer);
		TRACE_LOG("Commit Number: %d", cm);
		PROFILE_END();
	}
	else
	{
		// Free the buffer that wasn't committed to the
		// compositor since it never took ownership.
		pc->buffers_busy[pc->render_buffer] = false;
		TRACE_LOG("Buffer %zu marked free", pc->render_buffer);
	}

}


ENG_AudioBuffer *platform_get_audio_buffer (ENG_PlatformContext *pc)
{
	return platform_get_linux_audio_buffer(pc->audio_context);
}
