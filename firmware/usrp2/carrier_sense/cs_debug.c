/*
 * carrier_sense.c
 *
 *  Created on: Jan 24, 2013
 *      Author: ts
 */

#include "carrier_sense.h"

#include "net_common.h"

#include <string.h>
#include <stddef.h>

struct socket_address _debug_addr;

void udp_debug_set_addr(struct socket_address addr)
{
    _debug_addr.addr.addr = addr.addr.addr;
    _debug_addr.port = USRP2_UDP_CSMA_PORT;
}

void udp_debug_println(char *s)
{
    return;
    // this does not work, the packet blocks the actual response
    csma_frame_t frame;
    frame.type = TEXT_MESSAGE;
    frame.payload_len = strlen(s)+1;
    memset(frame.payload,0,CSMA_PAYLOAD_LENGTH);
    memcpy(frame.payload,s,strlen(s)+1);
    udp_debug_sendData(&frame,sizeof(csma_frame_t));
}

void udp_debug_sendData(void *d, int len)
{
    send_udp_pkt(USRP2_UDP_CSMA_PORT, _debug_addr, d, len);
}

