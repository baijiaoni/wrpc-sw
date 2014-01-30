#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wrc.h>
#include "shell.h"


extern uint32_t uDelay=0;

static int cmd_delay(const char *args[])
{
  if (uDelay == 0)
  {
    uDelay = 1;
  }
  else
  {
  uDelay = 0;
  }
  return 0;
}

static int cmd_kill(const char *args[])
{
  uDelay = 0;

  return 0;
}

DEFINE_WRC_COMMAND(s) = {
	.name = "k",
	.exec = cmd_kill,
};

DEFINE_WRC_COMMAND(delay_send) = {
	.name = "d",
	.exec = cmd_delay,
};
