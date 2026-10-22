/* Header used internally for render target contract between
 * platform layer and renderer */

#ifndef VAT7_RENDER_TARGET_H
#define VAT7_RENDER_TARGET_H


#include "engine_libc/eng_stdint.h"
#include "engine_core/eng_render.h"


typedef struct
{
	uint32_t width;
	uint32_t height;
	uint8_t *pixel_buffer;
	uint8_t pixel_bytes;
	uint16_t flags;
} vat7_RenderTarget;


static const vat7_RenderTarget vat7_NULL_RENDER_TARGET =
{
	.width = 0,
	.height = 0,
	.pixel_buffer = NULL,
	.pixel_bytes = 0,
	.flags = 0,
};


void vat7_render (vat7_RenderTarget render_target, vat7_RenderCommandBuffer *rcb);


#endif  // header guard
