/* Defines engine input events for interface between game and platform */

// TODO Add Controller constants

#ifndef ENGINE_INPUT_EVENTS_H
#define ENGINE_INPUT_EVENTS_H


#include "engine_libc/eng_stdbool.h"  //#include <stdbool.h>
#include "engine_libc/eng_stdint.h"  //#include <stdint.h>


typedef struct
{
	uint8_t type;
	uint8_t key;
} ENG_InputEvent;


typedef struct
{
	ENG_InputEvent buffer[UINT16_MAX];  // Matches SDL max event count
	uint16_t buffer_count;
	uint16_t mouse_x;
	uint16_t mouse_y;
} ENG_InputContext;


typedef struct
{
	uint16_t x;
	uint16_t y;
} ENG_MousePosition;


ENG_MousePosition ENG_get_mouse_position (ENG_InputContext *);
bool ENG_get_input_events (ENG_InputEvent *, ENG_InputContext *);


// NOTE: using static const not enum for constants to use specific int types.

/* EVENT TYPE CONSTANTS */
static const uint8_t ENG_NULL_INPUT   = 0u;
static const uint8_t ENG_QUIT         = 1u;
static const uint8_t ENG_KEY_DOWN     = 2u;
static const uint8_t ENG_KEY_UP       = 3u;
static const uint8_t ENG_MOUSE_DOWN   = 4u;
static const uint8_t ENG_MOUSE_UP     = 5u;

/* KEY CONSTANTS */
/* MOUSE BUTTONS */
static const uint8_t ENG_MSBTN_LEFT =   1u;
static const uint8_t ENG_MSBTN_MIDDLE = 2u;
static const uint8_t ENG_MSBTN_RIGHT =  3u;
// NOTE: For other mouse buttons, just use raw numeral.

/* KEYBOARD KEYS */
static const uint8_t ENG_NULL_KEY =  0u;
static const uint8_t ENG_K_ESC    =  1u;
static const uint8_t ENG_K_SPACE  =  2u;
static const uint8_t ENG_K_ENTER  =  3u;
static const uint8_t ENG_K_LSHIFT =  4u;
static const uint8_t ENG_K_RSHIFT =  5u;
static const uint8_t ENG_K_TAB    =  6u;
static const uint8_t ENG_K_LCTRL  =  7u;
static const uint8_t ENG_K_RCTRL  =  8u;
static const uint8_t ENG_K_LALT   =  9u;
static const uint8_t ENG_K_RALT   = 10u;
static const uint8_t ENG_K_LEFT   = 11u;
static const uint8_t ENG_K_RIGHT  = 12u;
static const uint8_t ENG_K_UP     = 13u;
static const uint8_t ENG_K_DOWN   = 14u;
static const uint8_t ENG_K_BSPACE = 15u;
static const uint8_t ENG_K_GRAVE  = 16u;
static const uint8_t ENG_K_HYPHEN = 17u;
static const uint8_t ENG_K_EQUAL  = 18u;
static const uint8_t ENG_K_FSLASH = 19u;
static const uint8_t ENG_K_BSLASH = 20u;
static const uint8_t ENG_K_LBRACK = 21u;
static const uint8_t ENG_K_RBRACK = 22u;
static const uint8_t ENG_K_PERIOD = 23u;
static const uint8_t ENG_K_COMMA  = 24u;
static const uint8_t ENG_K_SEMCLN = 25u;
static const uint8_t ENG_K_APOSTR = 26u;
static const uint8_t ENG_K_CAPSLK = 27u;
static const uint8_t ENG_K_PGUP   = 28u;
static const uint8_t ENG_K_PGDOWN = 29u;
static const uint8_t ENG_K_HOME   = 30u;
static const uint8_t ENG_K_END    = 31u;
static const uint8_t ENG_K_INSERT = 32u;
static const uint8_t ENG_K_DELETE = 33u;
static const uint8_t ENG_K_F1     = 34u;
static const uint8_t ENG_K_F2     = 35u;
static const uint8_t ENG_K_F3     = 36u;
static const uint8_t ENG_K_F4     = 37u;
static const uint8_t ENG_K_F5     = 38u;
static const uint8_t ENG_K_F6     = 39u;
static const uint8_t ENG_K_F7     = 40u;
static const uint8_t ENG_K_F8     = 41u;
static const uint8_t ENG_K_F9     = 42u;
static const uint8_t ENG_K_F10    = 43u;
static const uint8_t ENG_K_F11    = 44u;
static const uint8_t ENG_K_F12    = 45u;
static const uint8_t ENG_K_1      = 46u;
static const uint8_t ENG_K_2      = 47u;
static const uint8_t ENG_K_3      = 48u;
static const uint8_t ENG_K_4      = 49u;
static const uint8_t ENG_K_5      = 50u;
static const uint8_t ENG_K_6      = 51u;
static const uint8_t ENG_K_7      = 52u;
static const uint8_t ENG_K_8      = 53u;
static const uint8_t ENG_K_9      = 54u;
static const uint8_t ENG_K_0      = 55u;
static const uint8_t ENG_K_q      = 56u;
static const uint8_t ENG_K_w      = 57u;
static const uint8_t ENG_K_e      = 58u;
static const uint8_t ENG_K_r      = 59u;
static const uint8_t ENG_K_t      = 60u;
static const uint8_t ENG_K_y      = 61u;
static const uint8_t ENG_K_u      = 62u;
static const uint8_t ENG_K_i      = 63u;
static const uint8_t ENG_K_o      = 64u;
static const uint8_t ENG_K_p      = 65u;
static const uint8_t ENG_K_a      = 66u;
static const uint8_t ENG_K_s      = 67u;
static const uint8_t ENG_K_d      = 68u;
static const uint8_t ENG_K_f      = 69u;
static const uint8_t ENG_K_g      = 70u;
static const uint8_t ENG_K_h      = 71u;
static const uint8_t ENG_K_j      = 72u;
static const uint8_t ENG_K_k      = 73u;
static const uint8_t ENG_K_l      = 74u;
static const uint8_t ENG_K_z      = 75u;
static const uint8_t ENG_K_x      = 76u;
static const uint8_t ENG_K_c      = 77u;
static const uint8_t ENG_K_v      = 78u;
static const uint8_t ENG_K_b      = 79u;
static const uint8_t ENG_K_n      = 80u;
static const uint8_t ENG_K_m      = 81u;
static const uint8_t ENG_K_NP_DIV = 82u;
static const uint8_t ENG_K_NP_MUL = 83u;
static const uint8_t ENG_K_NP_MIN = 84u;
static const uint8_t ENG_K_NP_PLS = 85u;
static const uint8_t ENG_K_NP_ENT = 86u;
static const uint8_t ENG_K_NP_PER = 87u;
static const uint8_t ENG_K_NP_0   = 88u;
static const uint8_t ENG_K_NP_1   = 89u;
static const uint8_t ENG_K_NP_2   = 90u;
static const uint8_t ENG_K_NP_3   = 91u;
static const uint8_t ENG_K_NP_4   = 92u;
static const uint8_t ENG_K_NP_5   = 93u;
static const uint8_t ENG_K_NP_6   = 94u;
static const uint8_t ENG_K_NP_7   = 95u;
static const uint8_t ENG_K_NP_8   = 96u;
static const uint8_t ENG_K_NP_9   = 97u;


#endif  // header guard
