/*
 * cs_debug.h
 *
 *  Created on: Jan 24, 2013
 *      Author: ts
 */

#ifndef CS_DEBUG_H_
#define CS_DEBUG_H_

#include "net/socket_address.h"

struct socket_addr;

void udp_debug_println(char *s);
void udp_debug_sendData(void *d, int len);
void udp_debug_set_addr(struct socket_address addr);

#endif /* CS_DEBUG_H_ */
