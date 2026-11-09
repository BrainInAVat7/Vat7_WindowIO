/* Placeholder for windows window_io version */

#include <windows.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "../include/vat7_window.h"
#include "../include/vat7_input.h"
#include "../include/vat7_render_target.h"


/* Windows Boilerplate */
static LRESULT CALLBACK WindowProc(HWND hwnd,
				UINT uMsg,
				WPARAM wParam,
				LPARAM lParam)
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}




//temporary placholder
struct vat7_WindowContext
{
	HWND hwnd;
	vat7_InputState input_state;
};

/* API Functions */
void vat7_win_io_quit(vat7_WindowContext *wc)
{
	free(wc);
}


vat7_WindowContext *vat7_win_io_init (uint16_t width, uint16_t height)
{
	(void)width;
	(void)height;

	/* Memory for Window Context */
	vat7_WindowContext *wc =
		(vat7_WindowContext *)malloc(sizeof(vat7_WindowContext));

	/* Input State */
	wc->input_state.buffer_count = 0u;
	wc->input_state.mouse_x = 0u;
	wc->input_state.mouse_y = 0u;

	/* Create Windows Window */
	HINSTANCE hInstance = GetModuleHandle(NULL);
	const char *class_name = "Vat7 Window";
	WNDCLASS wndclass = {0};
	wndclass.lpfnWndProc = WindowProc;
	wndclass.hInstance = hInstance;
	wndclass.lpszClassName = class_name;
	ATOM register_status = RegisterClassA(&wndclass);
	//TODO real error checking here
	assert(register_status != 0);

	HWND hwnd = CreateWindowEx(0,
			class_name,
			"",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			NULL,
			NULL,
			hInstance,
			NULL);
	//TODO error checking here and replace assert with logging
	assert(hwnd != NULL);
	ShowWindow(hwnd, SW_SHOW);

	wc->hwnd = hwnd;

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
