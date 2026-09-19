#pragma once

#include <stdbool.h>

#define AGX_REFUSAL_BYTES 256u

const char*
agx_refusal(void);
void
agx_refusal_clear(void);

bool
agx_refuse(const char* format, ...);
