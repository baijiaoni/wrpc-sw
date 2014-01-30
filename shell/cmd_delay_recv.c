#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wrc.h>
#include "shell.h"

uint32_t uRecvDelay = 0;

static int cmd_delay_recv(const char *args[])
{
  if(uRecvDelay==0)
  { 
    uRecvDelay = 1;
  }
  else
  {
    uRecvDelay = 0;
  }
  return 0;
}

static int cmd_stop1(const char *args[])
{
  uRecvDelay = 0;
  
  return 0;
}

DEFINE_WRC_COMMAND(s) = {
	.name = "t",
	.exec = cmd_stop1,
};

DEFINE_WRC_COMMAND(delay_recv) = {
	.name = "delay_recv",
	.exec = cmd_delay_recv,
};
