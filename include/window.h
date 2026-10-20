/* Header file for platform.c that defines neutral API for input/output */

#ifndef PLATFORM_H
#define PLATFORM_H

#include "render_target.h"
#include "audio_buffer.h"
#include "engine_libc/eng_stdint.h"
#include "engine_core/eng_input.h"  // For ENG_InputContext


// Defined in platform source code
typedef struct ENG_PlatformContext ENG_PlatformContext;

typedef void (*memory_init_fn)(void *, size_t);
typedef void (*timing_init_fn)(uint64_t (*)(void), uint64_t (*)(void));


// Functions
ENG_PlatformContext *platform_initialize (memory_init_fn, timing_init_fn);

ENG_InputContext *platform_get_input (ENG_PlatformContext *);

ENG_RenderTarget platform_get_render_target (ENG_PlatformContext *);

ENG_AudioBuffer *platform_get_audio_buffer (ENG_PlatformContext *pc);

void platform_present (ENG_PlatformContext *);

void platform_quit (ENG_PlatformContext *);

bool platform_timing_is_initialized (void);

uint64_t platform_get_timing_start_count (void);

uint64_t platform_get_timing_counter (void);

uint64_t platform_get_timing_frequency (void);

void platform_wait (double);


#endif  // header guard
