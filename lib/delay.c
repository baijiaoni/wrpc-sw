#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "endpoint.h"
#include "ptpd_netif.h"
#include "hw/memlayout.h"
#include "hw/etherbone-config.h"

#define TIME_TO_LIVE         10
#define PROTOCOL_ICMP        1

#ifndef htons
#define htons(x) x
#endif
#define IDLE_TIME_NEXT_STAMP 50000
#define TIME_TO_LIVE         10
#define PROTOCOL_ICMP        0
#define DELAY_REQUESTS       1
#define SLEEP_US             100
#define PRIO_DELAY_REQ       6 /* DELAY_REQUESTS times counter message */
#define PRIO_DELAY_RESP      5 /* Message with timestamps for DELAY_REQUESTS packets */


	static uint8_t buf[400];
	static wr_sockaddr_t addr;
  static int len;
  static uint32_t uCycleCnt = 0;
  bool fBlockForward = false;
  static wr_timestamp_t wr_ts;

  /* Delay send */
  static uint32_t uDelayCycleCounterHigh = 0;
  static uint32_t uDelayCycleCounterLow = 0;
  static uint64_t uDelayCycleCounter = 0;
  static uint16_t uPacketSize = 56;
  static uint32_t uDelayPacketsSend = 0;
  static uint32_t uSendDelayCycleTimestampNs[DELAY_REQUESTS];
  static uint32_t uSendDelayCycleTimestampSec[DELAY_REQUESTS];

  /* Delay receive */
  static uint32_t uDelayPacketsReceived = 0;
  static uint32_t uLastDelayCycleTimestampNs[DELAY_REQUESTS];
  static uint32_t uLastDelayCycleTimestampSec[DELAY_REQUESTS];
  static uint32_t uDelayCycleCounterRevcHigh[DELAY_REQUESTS];
  static uint32_t uDelayCycleCounterRevcLow[DELAY_REQUESTS];
  static uint32_t uCompareDelayCycleTimestampNs[DELAY_REQUESTS];
  static uint32_t uCompareDelayCycleTimestampSec[DELAY_REQUESTS];
static wr_socket_t *delay_socket;

unsigned int delay_checksum(unsigned short *buf, int shorts)
{
  int i;
  unsigned int sum;

  sum = 0;
  for (i = 0; i < shorts; ++i)
    sum += buf[i];

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

  return (~sum & 0xffff);
}


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

/* Check for new packets */
  if ((len = ptpd_netif_recvfrom(delay_socket, &addr, buf, sizeof(buf), &wr_ts)) > 0)
  {
  //if(len > 0)
   // pp_printf("GOT DELAY ----------------\n");
      /* Inspect packet */
      /* Is this an ICMP timestamp request? */
     if ((buf[9] == PROTOCOL_ICMP) && (buf[20] == 13) && (buf[21] == 0))
      {
        pp_printf("11 ----------------\n");
        /* Block this frame for other functions and checkers */
        fBlockForward = true;
        /* Extract the counter ID from frame */
        uDelayCycleCounterRevcHigh[uDelayPacketsReceived] = ((buf[24]<<24)&0xff000000) | ((buf[25]<<16)&0x00ff0000) | ((buf[26]<<8)&0x0000ff00) | (buf[27]&0x000000ff);
        uDelayCycleCounterRevcLow[uDelayPacketsReceived] = ((buf[28]<<24)&0xff000000) | ((buf[29]<<16)&0x00ff0000) | ((buf[30]<<8)&0x0000ff00) | (buf[31]&0x000000ff);
        /* Get the receiving timestamp of this frame */
        uCompareDelayCycleTimestampSec[uDelayPacketsReceived] = (uint32_t)(wr_ts.sec&0xffffffff);
        uCompareDelayCycleTimestampNs[uDelayPacketsReceived] = wr_ts.nsec;
        /* Ready for next packet */
        uDelayPacketsReceived++;

      }
      /* Is this an ICMP timestamp replay? */
      else if ((buf[9] == PROTOCOL_ICMP) && (buf[20] == 14) && (buf[21] == 0))
      {
        pp_printf("22 ----------------\n");
        /* Block this frame for other functions and checkers */
        fBlockForward = true;
        /* Get each time stamp */
        for (uDelayPacketsReceived=0; uDelayPacketsReceived<DELAY_REQUESTS; uDelayPacketsReceived++)
        {
          uLastDelayCycleTimestampSec[uDelayPacketsReceived] = (uint32_t) ((buf[(24+(uDelayPacketsReceived*8))]<<24)&0xff000000)
                                                             | (uint32_t) ((buf[(25+(uDelayPacketsReceived*8))]<<16)&0x00ff0000)
                                                             | (uint32_t) ((buf[(26+(uDelayPacketsReceived*8))]<<8)&0x0000ff00)
                                                             | (uint32_t) ((buf[(27+(uDelayPacketsReceived*8))])&0x000000ff);
          uLastDelayCycleTimestampNs[uDelayPacketsReceived] =  (uint32_t) ((buf[(28+(uDelayPacketsReceived*8))]<<24)&0xff000000)
                                                             | (uint32_t) ((buf[(29+(uDelayPacketsReceived*8))]<<16)&0x00ff0000)
                                                             | (uint32_t) ((buf[(30+(uDelayPacketsReceived*8))]<<8)&0x0000ff00)
                                                             | (uint32_t) ((buf[(31+(uDelayPacketsReceived*8))])&0x000000ff);
          mprintf("ipv4_poll: uLastDelayCycleTimestampSec[uDelayPacketsReceived] 0x%08x\n",uLastDelayCycleTimestampSec[uDelayPacketsReceived]);
          mprintf("ipv4_poll: uLastDelayCycleTimestampNs[uDelayPacketsReceived] 0x%08x\n",uLastDelayCycleTimestampNs[uDelayPacketsReceived]);
        }

      }
        /* Print measurement */
        for (uDelayPacketsReceived=0; uDelayPacketsReceived<DELAY_REQUESTS; uDelayPacketsReceived++)
        {
          mprintf("ipv4_poll: uCompareDelayCycleTimestampSec[uDelayPacketsReceived] 0x%08x\n",uCompareDelayCycleTimestampSec[uDelayPacketsReceived]);
          mprintf("ipv4_poll: uCompareDelayCycleTimestampNs[uDelayPacketsReceived] 0x%08x\n",uCompareDelayCycleTimestampNs[uDelayPacketsReceived]);
          if(uCompareDelayCycleTimestampSec[uDelayPacketsReceived]==uLastDelayCycleTimestampSec[uDelayPacketsReceived])
          {
            mprintf("ipv4_poll: RX ID:               0x%08x:0x%08x\n", uDelayCycleCounterRevcHigh[uDelayPacketsReceived], uDelayCycleCounterRevcLow[uDelayPacketsReceived]);
            mprintf("ipv4_poll: RX time stamp sec:   0x%08x:0x%08x\n", uCompareDelayCycleTimestampSec[uDelayPacketsReceived], uCompareDelayCycleTimestampNs[uDelayPacketsReceived]);
            mprintf("ipv4_poll: RX size:             %d bytes\n", len);
            mprintf("ipv4_poll: Delay                %dns\n", (uCompareDelayCycleTimestampNs[uDelayPacketsReceived]-uLastDelayCycleTimestampNs[uDelayPacketsReceived]));
          }
        }
        /* Reset counters */
        uDelayPacketsReceived = 0;
      }
      /* Don't care */
      else
      {
        /* Forward packet to other functions and checkers */
        fBlockForward = false;
      }
} /* if(uRecvDelay) */


void delay_send(void)
{
  if(uCycleCnt==IDLE_TIME_NEXT_STAMP)
  {
  /* Prepare packet */
    len = uPacketSize;
    addr.ethertype = htons(0x0CAFE);	/* IPv4 */


  //if(cntr > 100000)
  //{
   // cntr = 0;

    //pp_printf("sending\n");

    /* Prepare packet */
    //len = 1280;
    //addr.ethertype = htons(0x0CAFE);	/* IPv4 */
    //addr.ethertype = htons(0x0CAFE);	/* IPv4 */

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

      /* Payload => Send counters */
      for (uDelayPacketsSend=0; uDelayPacketsSend<DELAY_REQUESTS; uDelayPacketsSend++)
      {
        uDelayCycleCounterHigh = (uint32_t) (uDelayCycleCounter>>32);
        uDelayCycleCounterLow = (uint32_t) (uDelayCycleCounter&0xffffffff);
        buf[24] = (uint8_t) ((uDelayCycleCounterHigh>>24)&0xff);
        buf[25] = (uint8_t) ((uDelayCycleCounterHigh>>16)&0xff);
        buf[26] = (uint8_t) ((uDelayCycleCounterHigh>>8)&0xff);
        buf[27] = (uint8_t) (uDelayCycleCounterHigh&0xff);
        buf[28] = (uint8_t) ((uDelayCycleCounterLow>>24)&0xff);
        buf[29] = (uint8_t) ((uDelayCycleCounterLow>>16)&0xff);
        buf[30] = (uint8_t) ((uDelayCycleCounterLow>>8)&0xff);
        buf[31] = (uint8_t) (uDelayCycleCounterLow&0xff);
        /* Send packet */
        usleep(SLEEP_US);
        ptpd_netif_sendto(delay_socket, &addr, buf, len, &wr_ts);
        mprintf("ipv4_poll: TX ID:               0x%08x:0x%08x\n", uDelayCycleCounterHigh, uDelayCycleCounterLow);
        mprintf("ipv4_poll: TX time stamp sec:   0x%08x:0x%08x\n", (uint32_t)(wr_ts.sec&0xffffffff), wr_ts.nsec);
        mprintf("ipv4_poll: TX size:             %d bytes\n", len);
        /* Save timestamp */
        uSendDelayCycleTimestampSec[uDelayPacketsSend]= (uint32_t)(wr_ts.sec&0xffffffff);
        uSendDelayCycleTimestampNs[uDelayPacketsSend]= wr_ts.nsec;
        /* Increase cycle counter */
        uDelayCycleCounter++;
      }

      /* Payload => Send timestamps */
      buf[20] = 14; /* Timestamp Reply */
      buf[21] = 0;  /* Timestamp Reply */
      for (uDelayPacketsSend=0; uDelayPacketsSend<DELAY_REQUESTS; uDelayPacketsSend++)
      {
        buf[24+(uDelayPacketsSend*8)] = (uint8_t) (((uint32_t)uSendDelayCycleTimestampSec[uDelayPacketsSend]>>24)&0xff);
        buf[25+(uDelayPacketsSend*8)] = (uint8_t) (((uint32_t)uSendDelayCycleTimestampSec[uDelayPacketsSend]>>16)&0xff);
        buf[26+(uDelayPacketsSend*8)] = (uint8_t) (((uint32_t)uSendDelayCycleTimestampSec[uDelayPacketsSend]>>8)&0xff);
        buf[27+(uDelayPacketsSend*8)] = (uint8_t) (((uint32_t)uSendDelayCycleTimestampSec[uDelayPacketsSend]&0xff));
        buf[28+(uDelayPacketsSend*8)] = (uint8_t) (((uint32_t)uSendDelayCycleTimestampNs[uDelayPacketsSend]>>24)&0xff);
        buf[29+(uDelayPacketsSend*8)] = (uint8_t) (((uint32_t)uSendDelayCycleTimestampNs[uDelayPacketsSend]>>16)&0xff);
        buf[30+(uDelayPacketsSend*8)] = (uint8_t) (((uint32_t)uSendDelayCycleTimestampNs[uDelayPacketsSend]>>8)&0xff);
        buf[31+(uDelayPacketsSend*8)] = (uint8_t) (((uint32_t)uSendDelayCycleTimestampNs[uDelayPacketsSend]&0xff));
      }
      usleep(SLEEP_US);
      ptpd_netif_sendto(delay_socket, &addr, buf, len, 0);

      /* Control Counters */
      uCycleCnt=0;
    }
    else
    {
    usleep(1000000);
    uCycleCnt = 50000;
    }
  } /* if(uSendDelay) */

