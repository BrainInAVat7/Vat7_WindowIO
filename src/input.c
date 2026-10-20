/* Defines input functions used by games */

#include "engine_libc/eng_stdbool.h"
#include "engine_libc/eng_stddef.h"
#include "engine_core/eng_input.h"


ENG_MousePosition ENG_get_mouse_position (ENG_InputContext *ic)
{
	ENG_MousePosition mp = {ic->mouse_x, ic->mouse_y};
	return mp;
}


bool ENG_get_input_events (ENG_InputEvent *ie, ENG_InputContext *context)
{
	static size_t call_count = 0u;
	if (call_count >= context->buffer_count)
	{
		ie->type = ENG_NULL_INPUT;
		ie->key = ENG_NULL_KEY;
		call_count = 0u;
		context->buffer_count = 0u;
		return false;
	}
	*ie = context->buffer[call_count];
	call_count++;
	return true;
}
