#include "agx_refusal.h"

#include <stdarg.h>
#include <stdio.h>

static char g_agx_refusal[AGX_REFUSAL_BYTES] = {0};

const char*
agx_refusal(void)
{
  return g_agx_refusal;
}

void
agx_refusal_clear(void)
{
  g_agx_refusal[0] = '\0';
}

bool
agx_refuse(const char* format, ...)
{
  va_list arguments;

  va_start(arguments, format);
  vsnprintf(g_agx_refusal, sizeof(g_agx_refusal), format, arguments);
  va_end(arguments);
  return false;
}
