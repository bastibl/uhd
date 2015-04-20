/*
 * cs_set_info.h
 *
 *  Created on: Jan 24, 2013
 *      Author: ts
 */

#ifndef CS_SET_INFO_H_
#define CS_SET_INFO_H_

#include "carrier_sense.h"
#include "memory_map.h"

void cs_set_settings(csma_settings_t *settings);
void cs_set_rb_addr(cs_readback_data_t addr);

#endif /* CS_SET_INFO_H_ */
