/*
 * cs_set_info.c
 *
 *  Created on: Jan 24, 2013
 *      Author: ts
 */

#include "carrier_sense.h"
#include "memcpy_wa.h"
#include "banal.h"
#include <lwip/ip_addr.h>
#include <stddef.h>
#include <string.h>

void cs_set_settings(csma_settings_t *s_set)
{
    uint32_t settings;
    uint16_t threshold = ntohs(s_set->threshold);
    uint32_t slottime  = get_uint32((char*)(&s_set->slottime));
    udp_debug_println("Processing Settings...");

    settings =  (s_set->readback_addr << 8) | 0; //s_set->samples;

    cs_settings->settings   = settings;
    cs_threshold->threshold = threshold;
    cs_slottime->slottime   = slottime;

    udp_debug_println("Done!");
}

void cs_set_rb_addr(cs_readback_data_t addr)
{
    udp_debug_println("Setting Readback Address...");

    uint32_t settings;

    settings = readback_mux->cs_settings_rb;
    settings &= 0xFFFF00FF;
    settings |= ((uint8_t)addr << 8);

    cs_settings->settings = settings;

    udp_debug_println("Done!");
}

