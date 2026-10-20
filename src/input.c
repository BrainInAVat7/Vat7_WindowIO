/* Defines input functions used by games */

#include <stdbool.h>
#include <stddef.h>

#include "vat7_input.h"  // TODO internal header, sort placement of this


vat7_MousePosition vat7_get_mouse_position (vat7_InputContext *ic)
{
	vat7_MousePosition mp = {ic->mouse_x, ic->mouse_y};
	return mp;
}


bool vat7_get_input_events (vat7_InputEvent *ie, vat7_InputContext *context)
{
	static size_t call_count = 0u;
	if (call_count >= context->buffer_count)
	{
		ie->type = vat7_NULL_INPUT;
		ie->key = vat7_NULL_KEY;
		call_count = 0u;
		context->buffer_count = 0u;
		return false;
	}
	*ie = context->buffer[call_count];
	call_count++;
	return true;
}
