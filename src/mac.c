/* Placeholder for mac window_io version */

#include <stdlib.h>

#include "../include/vat7_window.h"
#include "../include/vat7_input.h"
#include "../include/vat7_render_target.h"

//temporary placholder
struct vat7_WindowContext
{
	vat7_InputState input_state;
};

/* API Functions */
void vat7_win_io_quit(vat7_WindowContext *wc)
{
	free(wc);
}


vat7_WindowContext *vat7_win_io_init (void)
{
	vat7_WindowContext *wc =
		(vat7_WindowContext *)malloc(sizeof(vat7_WindowContext));

	wc->input_state.buffer_count = 0u;
	wc->input_state.mouse_x = 0u;
	wc->input_state.mouse_y = 0u;

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
