#include <string.h>

#include "endpoint.h"
#include "ptpd_netif.h"
#include "hw/memlayout.h"
#include "hw/etherbone-config.h"

#define TIME_TO_LIVE         10
#define PROTOCOL_ICMP        1

#ifndef htons
#define htons(x) x
#endif

static wr_socket_t *delay_socket;

void delay_init(const char *if_name)
{
	wr_sockaddr_t saddr;

	/* Configure socket filter */
	memset(&saddr, 0, sizeof(saddr));
	strcpy(saddr.if_name, if_name);
	get_mac_addr(&saddr.mac[0]);	/* Unicast */
	saddr.ethertype = htons(0x0CAFE);
	saddr.family = PTPD_SOCK_RAW_ETHERNET;

	delay_socket = ptpd_netif_create_socket(PTPD_SOCK_RAW_ETHERNET,
					       0, &saddr);
}


void delay_poll(void)
{
	uint8_t buf[400];
	wr_sockaddr_t addr;
	int len;

  len = ptpd_netif_recvfrom(delay_socket, &addr, buf, sizeof(buf), 0);

  if(len > 0)
    pp_printf("GOT DELAY \n");

}


void delay_send(void)
{
  static int cntr;
  int8_t buf[1600];
  wr_sockaddr_t addr;
  int len;


  if(cntr > 100000)
  {
    cntr = 0;

    pp_printf("sending\n");

    /* Prepare packet */
    len = 52;
    addr.ethertype = htons(0x0CAFE);	/* IPv4 */

    addr.mac[0] = 0x00;
    addr.mac[1] = 0x26;
    addr.mac[2] = 0x7b;
    addr.mac[3] = 0x00;
    addr.mac[4] = 0x02;
    addr.mac[5] = 0x03;

    //addr.mac[0] = 0xff;
    //addr.mac[1] = 0xff;
    //addr.mac[2] = 0xff;
    //addr.mac[3] = 0xff;
    //addr.mac[4] = 0xff;
    //addr.mac[5] = 0xff;


    /* Version, IHL, DSCP and ECN */
    buf[0] = 0x00; /* Version 4, Header length 20 bytes */
    buf[1] = 0x00;
    /* Total length */
    buf[2] = (len>>8)&0xff;
    buf[3] = (len)&0xff;
    /* Identification */
    buf[4] = 0x00;
    buf[5] = 0x00;
    /* Flags and fragment offset */
    buf[6] = 0x40; /* Don't fragment */
    buf[7] = 0x00;
    /* TTL */
    buf[8] = 0x0;
    /* Protocol */
    buf[9] = 0x0;
    /* Header checksum */
    buf[10] = 0x00;
    buf[11] = 0x00;
    /* Source Address */
    buf[12] = 192;
    buf[13] = 168;
    buf[14] = 1;
    buf[15] = 6; /* From exploder ... */
    /* Destination Address */
    buf[16] = 192;
    buf[17] = 168;
    buf[18] = 1;
    buf[19] = 3; /* ... to pexaria */
    /* Type */
    buf[20] = 13; /* Timestamp request */
    buf[21] = 0;  /* Timestamp request */
    /* ICMP checksum */
    buf[22] = 0xff;
    buf[23] = 0xff;

    ptpd_netif_sendto(delay_socket, &addr, buf, len, 0);
  }
  else
  {
    cntr += 1;
  }

}

