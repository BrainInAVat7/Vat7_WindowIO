/* Header file for platform.c that defines neutral API for input/output */

#ifndef VAT7_WINDOW_H
#define VAT7_WINDOW_H

#include "vat7_render_target.h"
#include "vat7_input.h"  // For vat7_InputState


// Defined in platform source code
typedef struct vat7_WindowContext vat7_WindowContext;


// Functions
vat7_WindowContext *vat7_win_io_init (void);

vat7_InputState *vat7_win_io_get_input_state (vat7_WindowContext *);

vat7_RenderTarget vat7_win_io_get_render_target (vat7_WindowContext *);

void vat7_win_io_present (vat7_WindowContext *);

void vat7_win_io_quit (vat7_WindowContext *);


#endif  // header guard
