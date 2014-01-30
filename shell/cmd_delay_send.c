#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wrc.h>
#include "shell.h"

uint32_t uSendDelay = 0;

static int cmd_delay_send(const char *args[])
{
  if (uSendDelay == 0)
  {
    uSendDelay = 1;
  }
  else
  {
  uSendDelay = 0;
  }
  return 0;
}

static int cmd_stop(const char *args[])
{
  uSendDelay = 0;
  
  return 0;
}

DEFINE_WRC_COMMAND(s) = {
	.name = "s",
	.exec = cmd_stop,
};

DEFINE_WRC_COMMAND(delay_send) = {
	.name = "delay_send",
	.exec = cmd_delay_send,
};

