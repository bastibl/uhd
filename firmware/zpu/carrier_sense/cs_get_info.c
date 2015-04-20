/*
 * cs_get_info.c
 *
 *  Created on: Jan 24, 2013
 *      Author: ts
 */

#include "carrier_sense.h"

#include "memory_map.h"
#include "memcpy_wa.h"

#include <stddef.h>
#include <string.h>

#include <lwip/ip_addr.h>

void cs_get_settings()
{
    csma_frame_t oframe;
    uint32_t sets;
    csma_settings_t *g_set;

    udp_debug_println("Processing Settings Request...");
    oframe.type = GET_SETTINGS;
    oframe.payload_len = sizeof(csma_settings_t);
    memset(oframe.payload,0,CSMA_PAYLOAD_LENGTH);

    sets = (readback_mux->cs_settings_rb);

    g_set = (csma_settings_t*)oframe.payload;
    //g_set->samples       = (sets & 0x00000007);
    g_set->readback_addr = (sets & 0x0000FF00) >> 8;
    cs_set_rb_addr(THRESHOLD);
    g_set->threshold     = readback_mux->cs_readback;

    memcpy(oframe.payload, g_set, sizeof(csma_settings_t));
    udp_debug_sendData(&oframe, sizeof(csma_frame_t));
}

void cs_get_backoff()
{
    csma_frame_t oframe;
    cs_backoff_t g_bo;

    udp_debug_println("Processing Backoff Request...");
    oframe.type = GET_BACKOFF;
    oframe.payload_len = sizeof(cs_backoff_t);
    memset(oframe.payload,0,CSMA_PAYLOAD_LENGTH);

    cs_set_rb_addr(BACKOFF0);
    g_bo.bo_rnd0 = (readback_mux->cs_readback);
    cs_set_rb_addr(BACKOFF1);
    g_bo.bo_rnd1 = (readback_mux->cs_readback);
    cs_set_rb_addr(BACKOFF2);
    g_bo.bo_rnd2 = (readback_mux->cs_readback);
    cs_set_rb_addr(BACKOFF3);
    g_bo.bo_rnd3 = (readback_mux->cs_readback);
    cs_set_rb_addr(BACKOFF4);
    g_bo.bo_rnd4 = (readback_mux->cs_readback);
    cs_set_rb_addr(BACKOFF5);
    g_bo.bo_rnd5 = (readback_mux->cs_readback);

    memcpy(oframe.payload,&g_bo,sizeof(cs_backoff_t));
    udp_debug_sendData(&oframe,sizeof(csma_frame_t));
}

void cs_get_status()
{
    csma_frame_t oframe;
    csma_state_t g_stat;

    udp_debug_println("Processing Status Request...");
    oframe.type = GET_STATUS;
    oframe.payload_len = sizeof(csma_state_t);
    memset(oframe.payload,0,CSMA_PAYLOAD_LENGTH);

    uint32_t status = readback_mux->cs_status;
    g_stat.run              = (status & 0x00000001);        // 0000|0000|0000|0000|0000|0000|0000|0001
    g_stat.error            = (status & 0x00000002) >>  1;  // 0000|0000|0000|0000|0000|0000|0000|0010
    g_stat.state            = (status & 0x0000003C) >>  2;  // 0000|0000|0000|0000|0000|0000|0011|1100
    g_stat.ena              = (status & 0x00000040) >>  6;  // 0000|0000|0000|0000|0000|0000|0100|0000
    g_stat.free             = (status & 0x00000080) >>  7;  // 0000|0000|0000|0000|0000|0000|1000|0000
    g_stat.round            = (status & 0x00000700) >>  8;  // 0000|0000|0000|0000|0000|0111|0000|0000
    g_stat.ready            = (status & 0x00000800) >> 11;  // 0000|0000|0000|0000|0000|1000|0000|0000
    g_stat.reserved_4       = (status & 0x0000F000) >> 12;  // 0000|0000|0000|0000|1111|0000|0000|0000
    g_stat.reserved_16      = (status & 0xFFFF0000) >> 16;  // 1111|1111|1111|1111|0000|0000|0000|0000

    memcpy(oframe.payload,&g_stat,sizeof(csma_state_t));
    udp_debug_sendData(&oframe,sizeof(csma_frame_t));
}

void cs_get_readback()
{
    csma_frame_t oframe;
    oframe.type = GET_READBACK;
    oframe.payload_len = sizeof(uint32_t);
    memset(oframe.payload,0,CSMA_PAYLOAD_LENGTH);

    memcpy(oframe.payload,(char*)&(readback_mux->cs_readback),sizeof(uint32_t));

    udp_debug_sendData(&oframe,sizeof(csma_frame_t));
}

void cs_get_rssi()
{
    csma_frame_t oframe;
    oframe.type = GET_RSSI;
    oframe.payload_len = sizeof(uint32_t);
    memset(oframe.payload,0,CSMA_PAYLOAD_LENGTH);

    memcpy(oframe.payload,(char*)&(readback_mux->cs_avg_rssi),sizeof(uint32_t));

    udp_debug_sendData(&oframe,sizeof(csma_frame_t));
}
