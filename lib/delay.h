#ifndef DELAY_H
#define DELAY_H

#include <inttypes.h>


void delay_init(const char *if_name);
void delay_poll(void);
void delay_send(void);



#endif
